//
// Created by xiaoq on 3/26/19.
//

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "timer.h"
#include "gzip.h"
#include "ws.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using namespace std;

int nSessions = 0;
vector<shared_ptr<session>> sessions;

vector<shared_ptr<session>> get_sessions() {
    sessions.erase(remove_if(sessions.begin(), sessions.end(), [](auto s) {return !s->alive;}), sessions.end());
    return sessions;
}

listener::listener(asio::io_context &ioc, const tcp::endpoint &endpoint) : acceptor(ioc), socket(ioc), ioc(ioc) {
    acceptor.open(endpoint.protocol());
    acceptor.set_option(asio::socket_base::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen(asio::socket_base::max_listen_connections);
}

void listener::run() {
    cout << "accept loop" <<  endl;
    acceptor.async_accept(socket, [self = shared_from_this()](beast::error_code ec) {
        self->on_accept(ec);
    });
}

void listener::on_accept(beast::error_code &ec) {
    auto s = make_shared<session>(move(socket));
    sessions.push_back(s);
    s->run();
    run();
}

session::session(tcp::socket &&socket) :
        ws(move(socket)),
        alive(true),
        id(nSessions++),
        last_message_time_point(chrono::steady_clock::now()) {
//        socket.get_io_context();
}

void session::fail(beast::error_code ec, const string &where) {
    if (ec == asio::error::operation_aborted || ec == websocket::error::closed)
        return;

    alive = false;
    cerr << "error: " << where << ": " << ec.message() << endl;
}

void session::run() {
    // timer poll(1, ioc, [&]() {on_poll();});
    if (!check_alive()) return;
    ws.async_accept([self = shared_from_this()](auto ec) {
        self->on_accept(ec);
    });
}

bool session::check_alive() {
    auto now = chrono::steady_clock::now();
    if (chrono::duration_cast<chrono::duration<double>>(now - last_message_time_point).count() > 30) {
        cout << id << " is inactive for 30 seconds" << endl;
        alive = false;
    }
    return alive;
}

void session::on_accept(beast::error_code ec) {
    if (ec) return fail(ec, "accept");
    if (!check_alive()) return;
    readloop();
}

void session::readloop() {
    if (!check_alive()) return;
    cout << "readloop" << endl;
    ws.async_read(buffer, [self = shared_from_this()](auto ec, auto size) {
        self->on_read(ec, size);
    });
}

void session::on_read(beast::error_code ec, size_t size) {
    if (ec) return fail(ec, "read");
    last_message_time_point = chrono::steady_clock::now();
    if (!check_alive()) return;

    string data = beast::buffers_to_string(buffer.data());
    cout << id << ": " << "received " << data << endl;
    // ws.text(ws.got_text());
    // ws.async_write(asio::buffer(data), [self = shared_from_this()](beast::error_code, std::size_t) {
    //     self->on_accept();
    // });
    readloop();
    buffer.consume(size);
}

bool session::write(const string &data) {
    if (!check_alive()) return false;
    if (is_writing) {
        cerr << id << " in writing, message ignored" << endl;
        return false;
    }
    cout << id << ": " << "sending " << data.length() << endl;

    is_writing = true;
    data_sending = data;
    ws.text(true);
    ws.async_write(asio::buffer(data_sending), [self = shared_from_this()](beast::error_code ec, std::size_t) {
        self->is_writing = false;
        if (ec) return self->fail(ec, "write");
    });
    return true;
}

//
// Created by xiaoq on 3/26/19.
//

#ifndef LEMOND_WS_H
#define LEMOND_WS_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <chrono>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using namespace std;

class session : public enable_shared_from_this<session> {
    beast::flat_buffer buffer;
    websocket::stream<tcp::socket> ws;
    string data_sending;
    bool is_writing = false;
    int id;
public:
    bool alive;
    chrono::steady_clock::time_point last_message_time_point;

    session(tcp::socket &&socket);
    void fail(beast::error_code ec, const string &where);
    void run();
    bool check_alive();
    void on_accept(beast::error_code ec);
    void readloop();
    void on_read(beast::error_code ec, size_t size);
    bool write(const string &data);
};

class listener : public enable_shared_from_this<listener> {
    tcp::acceptor acceptor;
    tcp::socket socket;
    asio::io_context &ioc;
public:
    listener(asio::io_context &ioc, const tcp::endpoint& endpoint);
    void run();
    void on_accept(beast::error_code &ec);
};

vector<shared_ptr<session>> get_sessions();

#endif //LEMOND_WS_H

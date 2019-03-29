//
// Created by xiaoq on 3/26/19.
//

#ifndef LEMOND_SPAWN_H
#define LEMOND_SPAWN_H

#include <string>
#include <unistd.h>
#include <iostream>
#include <boost/process.hpp>
#include <boost/process/async.hpp>


namespace bp = boost::process;
namespace asio = boost::asio;

using namespace std;
class spawn_process {
private:
    bp::async_pipe output;
    bp::child child;
    asio::streambuf buffer;
    function<void(istream &)> handler;
public:
    void readloop() {
        asio::async_read_until(output, buffer, '\n', [&](boost::system::error_code ec, size_t transferred) {
            if (transferred) {
                std::istream is(&buffer);
                handler(is);
            }
            if (ec) {
                std::cerr << "Output pipe: " << ec.message() << std::endl;
            } else {
                readloop();
            }
        });
    }

    spawn_process(const string &cmd, function<void(istream &)> handler, asio::io_service &ios) :
            output(ios), child(cmd, bp::std_out > output, bp::std_err > bp::null, ios), handler(handler) {
        readloop();
    }
};

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

#endif //LEMOND_SPAWN_H

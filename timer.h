//
// Created by xiaoq on 3/26/19.
//

#ifndef LEMOND_TIMER_H
#define LEMOND_TIMER_H
#include <boost/asio/steady_timer.hpp>
namespace asio = boost::asio;
using namespace std;
class timer {
    asio::steady_timer t;
    function<void()> func_;
    int second_;
public:
    timer(int second, asio::io_context &io, function<void()> func): t(io, chrono::seconds(second)), func_(func), second_(second) {
        t.async_wait([&](auto &ec) {
            func_();
            loop();
        });
    }

    void loop() {
        t.expires_at(t.expires_at() + chrono::seconds(second_));
        t.async_wait([&](auto &ec) {
            func_();
            loop();
        });
    }
};
#endif //LEMOND_TIMER_H

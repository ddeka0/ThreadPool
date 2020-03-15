#pragma once
#include <bits/stdc++.h>
#include "notification_queue.hpp"

template<typename F>
concept Invocable = std::is_invocable_v<F>;

class task_system {
    private:
    const unsigned              _count{std::thread::hardware_concurrency()};
    std::vector<std::thread>    _threads;
    notification_queue          _q;
    std::atomic_bool            _done;
    std::mutex                  _m;

    void run(unsigned i) {
        thread_local int work_count = 0;
        while(!_done) {
            std::function<void()> f;
            _q.pop(f);
            if(f != nullptr and !_done) {
                f();
                work_count++;
            }
        }
        lock_t lock{_m};
        _total_work_count += work_count;
    }

    public:
        unsigned                    _total_work_count;
        
        task_system() {
            for(unsigned n = 0;n != _count ;++n) {
                _threads.emplace_back([&,n]() {
                    run(n);
                });
            }
        }
        ~task_system() {}
        void destruct() {
            _q.done();      // wake up the threads
            _done = true;   // stop the looping
            for(auto& e : _threads) {
                if(e.joinable()) {
                    e.join();
                }
            }
        }
        template<Invocable F>
        void submit(F &&f) {
            _q.push(std::forward<F>(f));
        }
};
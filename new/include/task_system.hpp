#pragma once
#include <bits/stdc++.h>
#include "notification_queue.hpp"

template<typename F>
concept Invocable = std::is_invocable_v<F>;

class task_system {
	private:
	const unsigned							_count{std::thread::hardware_concurrency()};
	std::vector<std::thread>				_threads;
	std::vector<notification_queue>			_q{_count};
	std::mutex								_m;
	std::atomic<unsigned>					_index{0};

	void run(unsigned i) {
		thread_local int work_count = 0;
		while(true) {
			std::function<void()> f;
			for(unsigned n = 0;n != _count; ++n) {
				if(_q[(i + n) % _count].try_pop(f)) break;
			}
			if(!f && !_q[i].pop(f)) break;
			f();
			work_count++;
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
			for(auto&e : _q) e.done(); // wake up threads
			for(auto& e : _threads) {
				if(e.joinable()) {
					e.join();
				}
			}
		}
		template<Invocable F>
		void submit(F &&f) {
			auto i = _index++;
			for(unsigned n = 0; n!= _count*4;++n) {
				if(_q[(i + n) % _count].try_push(std::forward<F>(f))) return;
			}
			_q[i % _count].push(std::forward<F>(f));
		}
};
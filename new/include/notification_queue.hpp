#pragma once
#include <bits/stdc++.h>
using lock_t = std::unique_lock<std::mutex>;
class notification_queue {
private:
	std::deque<std::function<void()>>   _q;
	bool                                _done{false};
	std::mutex                          _mutex;
	std::condition_variable             _ready;
public:
	void done() {
		{
			lock_t lock{_mutex};
			_done = true;
		}
		_ready.notify_all();
	}
	bool pop(std::function<void()>& x) {
		lock_t lock{_mutex};
		while(_q.empty() && !_done) _ready.wait(lock);
		_ready.wait(lock,[this]() {
			return !_q.empty() || _done;
		});
		x = nullptr;
		if(!_q.empty()) {
			x = std::move(_q.front());
			_q.pop_front();
		}
		return true;
	}
	template<typename F>
	void push(F&& f) {
		{
			lock_t lock{_mutex};
			_q.emplace_back(std::forward<F>(f));
		}
		_ready.notify_one();
	}
	// Test purpose ...
	std::size_t get_size() {
		lock_t lock{_mutex};
		return _q.size();
	}

};
#pragma once
#include <atomic>
#include <bits/stdc++.h>
#include <chrono>
#include <utility>

long long GetCurrentTimeInSec() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

/**
 * @brief
 * Invocable is concept to accept only callable work items in the submit method
 * @tparam F
 */
template <typename F>
concept Invocable = std::is_invocable_v<F>;
/**
 * @brief
 * task_system is the scheduler
 */
class task_system {
  using lock_t = std::unique_lock<std::mutex>;

 private:
  /**
   * @brief
   * work_object holds, actual function, an unique ID, and the time of
   * execution
   */
  struct work_object {
    std::function<void()> work;
    long long id;
    long long time;
  };
  /**
   * @brief
   * _count = number of concurrent threads supported
   */
  const unsigned _count{std::thread::hardware_concurrency()};
  /**
   * @brief
   * vector to store all the worker threads
   */
  std::vector<std::thread> _threads;
  /**
   * @brief
   * a queue to store all the ready work items
   */
  std::deque<work_object> _ready_q;
  /**
   * @brief
   * a mutex to protect the following things
   * _ready_q , _done , _future_work
   */
  std::mutex _mutex;
  /**
   * @brief
   * A set to contain all the future work items (separate from ready_q)
   */
  std::set<work_object,
           decltype([](const work_object& a, const work_object& b) {
             return a.time < b.time;
           })>
      _future_work;
  /**
   * @brief
   * _done = true, means task_system is shutting down.
   */
  bool _done = false;
  /**
   * @brief
   * _ready is used to wait or signal threads.
   */
  std::condition_variable _ready;
  /**
   * @brief
   * work_id_generator is used to give one unique Id to each work
   */
  std::atomic<long long> work_id_generator;

  //----------------------------------------------------------------------------
  /**
   * @brief
   * worker_loop is the main execution loop for a worker thread
   * @param i (i the thread index, not used however)
   * It does the following things in sequence:
   * 1. Try to get new ready work form ready_q
   * 2. Execute if some work found
   * 3. process future work items and add them to ready_q
   */
  void worker_loop(unsigned i) {
    auto execute_work = [](const std::function<void()>& work) {
      if (work) work();
    };
    while (true) {
      std::function<void()> work;
      if (!get_work(work)) break;
      execute_work(work);
      process_future_work();
    }
  }

  //----------------------------------------------------------------------------

  /**
   * @brief
   * process_future_work iterates over the sorted future work items
   * If any work is ready to execute now, adds it to the ready_q
   */
  void process_future_work() {
    std::vector<work_object> ready_works;

    lock_t lock{_mutex};
    if (_future_work.empty()) return;
    auto now_sec = GetCurrentTimeInSec();
    for (auto work_iter = std::begin(_future_work);
         work_iter != std::end(_future_work);) {
      if (now_sec < work_iter->time) {
        break;
      }
      ready_works.push_back(std::move(*work_iter));
      work_iter = _future_work.erase(work_iter);
    }
    lock.unlock();

    if (!ready_works.empty()) {
      for (auto&& work : ready_works) {
        add_to_ready_q(std::move(work));
      }
    }
  }
  //----------------------------------------------------------------------------

  /**
   * @brief
   * This function provides new work from the ready_q
   * @param work
   * @return true
   * @return false - when sysem is shutting down
   */
  bool get_work(std::function<void()>& work) {
    lock_t lock{_mutex};
    // when a threads wakes up, it checkes this predicate to decide if it needs
    // to sleep again. It this returns false, then it sleep again.
    // Please check condition_variable::wait_for(...) API arguments.
    auto stop_waiting = [this]() {
      return !_ready_q.empty() || _done || !_future_work.empty();
    };

    // If we have some future work items,
    // then sleep untill next future work item
    if (!_future_work.empty()) {
      long long sec_till_next_future_work =
          _future_work.begin()->time - GetCurrentTimeInSec();
      if (sec_till_next_future_work > 0) {
        _ready.wait_for(lock,
                        std::chrono::seconds(sec_till_next_future_work),
                        std::move(stop_waiting));
      }
    } else {
      _ready.wait(lock, std::move(stop_waiting));
    }

    // If the system is shutting down, return false.
    if (_done) {
      return false;
    }

    // If ready_q has something, extract it.
    if (!_ready_q.empty()) {
      work = std::move(_ready_q.front().work);
      _ready_q.pop_front();
    }
    return true;
  }

  //----------------------------------------------------------------------------

  /**
   * @brief
   * Add ready work items to ready_q and notifies one thread
   * @tparam F
   * @param f
   */
  template <typename F>
  void add_to_ready_q(F&& work) {
    {
      lock_t lock{_mutex};
      _ready_q.emplace_back(std::forward<F>(work));
    }
    //
    _ready.notify_one();
  }
  //----------------------------------------------------------------------------

  /**
   * @brief
   * notify all thread to wake up because the system is shutting down.
   */
  void done() {
    {
      lock_t lock{_mutex};
      _done = true;
    }
    _ready.notify_all();
  }

  //----------------------------------------------------------------------------

 public:
  /**
   * @brief Construct a new task system object
   * Create N threads and give them the worker_loop method.
   */
  task_system() {
    for (unsigned n = 0; n != _count; ++n) {
      _threads.emplace_back([&, n]() { worker_loop(n); });
    }
  }

  //----------------------------------------------------------------------------

  /**
   * @brief Destroy the task system object
   * Nothing to clean, all automatic cleanup.
   */
  ~task_system() {}

  //----------------------------------------------------------------------------

  /**
   * @brief
   * sets _done = true and joins all worker threads.
   */
  void destruct() {
    done();  // wake up the threads
    for (auto& e : _threads) {
      if (e.joinable()) {
        e.join();
      }
    }
  }

  //----------------------------------------------------------------------------

  /**
   * @brief
   * submits one callable work item to the system. If run_after_sec > 0
   * then the work will be executes after run_after_sec from current time.
   * @tparam F
   * @param f
   * @param run_after_sec
   */
  template <Invocable F>
  void submit(F&& f, long long run_after_sec = 0) {
    work_object wrk;
    auto current_id = work_id_generator.fetch_add(1);
    wrk.id = current_id;
    wrk.work = std::move(f);
    if (run_after_sec > 0) {
      wrk.time = GetCurrentTimeInSec() + run_after_sec;
      lock_t lock{_mutex};
      _future_work.insert(std::move(wrk));
      if (_future_work.begin()->id == current_id) {
        _ready.notify_one();
      }
      return;
    }
    wrk.time = GetCurrentTimeInSec();
    add_to_ready_q(std::move(wrk));
  }

  //----------------------------------------------------------------------------
};

#include <bits/stdc++.h>
#include <variant>
#include <time.h>
#include <sys/time.h>
#include <thread>
#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <functional>
#include <future>
#include <cmath>
#include <thread>
using namespace std;
using namespace std::chrono;

#define THREAD_POOL_SIZE 200
std::map<thread::id,std::string> tidToTname;

// template <class T>
// constexpr std::string_view type_name() {
//     using namespace std;
// #ifdef __clang__
//     string_view p = __PRETTY_FUNCTION__;
//     return string_view(p.data() + 34, p.size() - 34 - 1);
// #elif defined(__GNUC__)
//     string_view p = __PRETTY_FUNCTION__;
// #  if __cplusplus < 201402
//     return string_view(p.data() + 36, p.size() - 36 - 1);
// #  else
//     return string_view(p.data() + 49, p.find(';', 49) - 49);
// #  endif
// #elif defined(_MSC_VER)
//     string_view p = __FUNCSIG__;
//     return string_view(p.data() + 84, p.size() - 84 - 7);
// #endif
// }

class function_wrapper {
    struct impl_base {
        virtual void call() = 0;    // pure virtual function
        virtual ~impl_base() {}
    };
    std::unique_ptr<impl_base> impl;
    template<typename F>
    struct impl_type: impl_base {
        F f;
        impl_type(F && f_) : f(std::move(f_)) {}
        void call() {f();}
    };
public:
    template<typename F>
    function_wrapper(F &&f):
        impl(new impl_type<F>(std::move(f))) {}
    void operator()() {
        impl->call();
    }
    function_wrapper() = default;
    function_wrapper(function_wrapper && other):
        impl(std::move(other.impl))
    {}
    function_wrapper& operator=(function_wrapper && other) {
        impl = std::move(other.impl);
        return *this;
    }
    function_wrapper(const function_wrapper&) = delete;
    function_wrapper(function_wrapper&) = delete;
    function_wrapper& operator=(const function_wrapper&)=delete;
};

template<typename T>
class threadSafeQueue {
    private:
        mutable std::mutex mut;
        std::queue<std::shared_ptr<T>> taskQueue; /* 	task as pushed here and 
							task are processed in FIFO
							style */
        std::condition_variable dataCond;		/* used to protect the queue */
    public:
        threadSafeQueue(){}
        void waitAndPop(T& value);		/* wait untill task is not available in 
										   the queue */
        std::shared_ptr<T> waitAndPop();/* same but returns a shared pointer */
        bool tryPop(T& value);			/* does not block */
        std::shared_ptr<T> tryPop();    /* does not block and returns a pointer*/
        void Push(T newData);
        bool IsEmpty() const;				/* check if queue is empty or not */
		void notifyAllThreads();		/* notify all the waiting threads
										   used in Thpool decallocation	*/
};

template<typename T>
void threadSafeQueue<T>::notifyAllThreads() {
    dataCond.notify_all();
}
template<typename T>
void threadSafeQueue<T>::waitAndPop(T& value) {
    std::unique_lock<std::mutex> lk(mut);
    dataCond.wait(lk,[this](){return !taskQueue.empty();});
    value = std::move(*taskQueue.front());
    taskQueue.pop();
}
template<typename T>
std::shared_ptr<T> threadSafeQueue<T>::waitAndPop() {
    std::unique_lock<std::mutex> lk(mut);
    dataCond.wait(lk,[this](){return !taskQueue.empty();});
    std::shared_ptr<T> res = taskQueue.front();
    taskQueue.pop();
    return res;
}

template<typename T>
bool threadSafeQueue<T>::tryPop(T& value) {
    std::lock_guard<std::mutex> lk(mut);
    if(taskQueue.empty())
        return false;
    value = std::move(*taskQueue.front());
    taskQueue.pop();
    return true;
}   
template<typename T>
std::shared_ptr<T> threadSafeQueue<T>::tryPop() {
    std::lock_guard<std::mutex> lk(mut);
    if(taskQueue.empty())
        return std::shared_ptr<T>(); /* return nullptr */
    std::shared_ptr<T> res = taskQueue.front();
    taskQueue.pop();
    return res;
}
template<typename T>
void threadSafeQueue<T>::Push(T newData) { /* TODO: size check before pushing */
    // std::shared_ptr<T> data(std::make_shared<T>(std::move(newData)));
    auto data = std::make_shared<T>(std::move(newData));
										   /* construct the object before lock*/
    std::lock_guard<std::mutex> lk(mut);
    taskQueue.push(std::move(data));
    dataCond.notify_one();
}
template<typename T>
bool threadSafeQueue<T>::IsEmpty() const {
    std::lock_guard<std::mutex> lk(mut);
    return taskQueue.empty();
}

class threadJoiner { /* could be used to automatic thread deallocation 
						not used for now, TODO use it in the exception case */
    private:
        std::vector<std::thread> & threads;
    public:
        explicit threadJoiner(std::vector<std::thread>& thread_):threads(thread_){}
        ~threadJoiner() {
            cout <<"getting called, threadJoiner destructor" << endl;
            cout << "thread_vector_size = " << threads.size() << endl;
            for(unsigned long i = 0;i<threads.size();i++) {
                if(threads[i].joinable()) {
                    cout <<"i = "<< i << endl;
                    threads[i].join();
                }
            }
            cout <<"threadJoiner destructor exit" << endl;
        }
};

int xy() {
    cout <<"xy() function gets called" << endl;
    return 1;
}
int xz(int x) {
    cout <<"xz() function gets called" << endl;
    return 3;
}

class TaskA {
    public:
        using ReturnType = int;
        using Function = std::function<ReturnType(void)>;
        typedef ReturnType (CallSignature)();
        
        TaskA(Function &f) : func{f} {}
        
        ReturnType operator()() {
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            return func();
        }

    private:
        Function func;
        
};

class TaskB {
    public:
        using ReturnType = int;
        using Function = std::function<ReturnType(int)>;
        typedef ReturnType (CallSignature)(int);

        TaskB(Function &f , int x) : func{f} {
            this->x = x;
        }
        ReturnType operator()() {
            std::this_thread::sleep_for(std::chrono::milliseconds(4000));
            return func(x);
        }
    private:
        int x;
        Function func;
};

template <typename T>
struct function_signature
    : public function_signature<decltype(&T::operator())>
{};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_signature<ReturnType(ClassType::*)(Args...)> {
    using type = ReturnType(Args...);
};

class Thpool {
    std::atomic_bool done;
    threadSafeQueue<function_wrapper> workQ;
    std::vector<std::thread> threads;
    threadJoiner joiner;
    void workerThread() {

    }
    public:
        Thpool(): done(false) ,joiner(threads) {
	        unsigned const maxThreadCount = THREAD_POOL_SIZE;
	        printf("ThreadPool Size = %d\n",maxThreadCount);
            try {
                for(unsigned int i = 0;i<maxThreadCount;i++) {
                    threads.emplace_back(
                        [this]()
                        {
                            while(!done) {
                                auto task = workQ.waitAndPop();
                                if(task != nullptr and !done) {
                                    (*task)();
                                }
                            }
                        }
                    );
                }
            }catch(...) {
                done = true;
                throw;
            }

        }
        ~Thpool() {
            cout <<"getting called, Thpool destructor" << endl;
            done = true;
        }
        
        template<typename TaskType>
        auto submit(TaskType func) {
            using signature = typename function_signature<TaskType>::type;
            auto task  = std::packaged_task<signature>(std::move(func));
            auto future = task.get_future();
            workQ.Push(std::move(task));
            return std::move(future);
        }
        void deAllocatePool() {
            done = true;
            workQ.notifyAllThreads();
            unsigned const maxThreadCount = THREAD_POOL_SIZE;
	    	for(unsigned int i = 0;i<maxThreadCount;) {
                if(threads[i].joinable()) {
                    threads[i].join();
                    i++;
                }else {
                	workQ.notifyAllThreads();
                }
            }
        }
};

int main() {

    cout << "Hello World !" << endl;
    Thpool pool;
    std::function<int(void)> func1{xy};
    TaskA task1{func1};

    std::function<int(int)> func2{xz};
    TaskB task2(func2,4);

    auto x = pool.submit(task1);
    auto y = pool.submit(task2);

    cout <<"result of xy = "<< x.get() << endl;
    cout <<"result of xz = "<< y.get() << endl;

    getchar();
    printf("Exit of main function!\n");
}

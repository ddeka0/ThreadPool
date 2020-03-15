#include "../include/notification_queue.hpp"
void test_queue_basic() {
    notification_queue q;
    q.push([]() {
        std::cout <<"TaskA" << std::endl;
    });
    q.push([]() {
        std::cout <<"TaskB" << std::endl;
    });
    std::cout << q.get_size() << std::endl;
    std::cout <<"test_queue_basic complete" << std::endl;
}
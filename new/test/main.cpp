#include <bits/stdc++.h>
#include "queue_test_basic.hpp"
#include "../include/task_system.hpp"
#include <chrono>
int main(int argc,char *argv[]) {
    if(argc < 2) {
        std::cout<<"usage : ./a.out <simulation_time_in_seconds>" << std::endl;
        std::exit(1);
    }
    int _seconds = std::atoi(argv[1]);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 2); // total 3 tasks: 0,1,2

    std::map<int,std::function<void()>> Tasks = 
    {
        {0,[]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
         }},
        {1,[]() {
            for(unsigned int i = 0;i<=100000000;i++) {
                int x = i + i / 2;
                int y = x + i;
                int z = x + y;
            }
        }},
        {2,[]() {
            for(unsigned int i = 0;i<=50000000;i++) {
                int x = i + i / 2;
                int y = x + i;
                int z = x + y;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            for(unsigned int i = 0;i<=50000000;i++) {
                int x = i + i / 2;
                int y = x + i;
                int z = x + y;
            }
        }}
    };

    task_system pool;
    for(unsigned j = 0;j<=10000;j++) {
        pool.submit(Tasks[dis(gen)]);
    }
    std::this_thread::sleep_for(std::chrono::seconds(_seconds));
    pool.destruct();
    std::cout <<"No. of task completed in 10 Seconds = "<< pool._total_work_count << std::endl;
    
}
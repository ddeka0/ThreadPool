#include "task_system.hpp"
#include <chrono>
int main(int argc, char* argv[]) {
  task_system pool;
  // pool.submit([](){
  //   std::cout <<"work-8" << std::endl;
  // },8);
  // pool.submit([](){
  //   std::cout <<"work-6" << std::endl;
  // },6);
  // pool.submit([](){
  //   std::cout <<"work-4" << std::endl;
  // },4);
  // pool.submit([](){
  //   std::cout <<"work-3" << std::endl;
  // },3);
  // pool.submit([](){
  //   std::cout <<"work-0" << std::endl;
  // });
  // pool.submit([](){
  //   std::cout <<"work-0-0" << std::endl;
  // });
  // pool.submit([](){
  //   std::cout <<"work-5" << std::endl;
  // },5);
  std::this_thread::sleep_for(std::chrono::seconds(1));

  pool.submit(
      []() {
        std::cout << "work-10" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::cout << "work-10 done!" << std::endl;
      },
      10);
  pool.submit([]() { std::cout << "work-11" << std::endl; }, 11);

  std::this_thread::sleep_for(std::chrono::seconds(30));
  pool.destruct();
}

// TaskQueueTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <thread>
#include "base/task_queue_win.h"
int main()
{
  auto task_queue_win_factory = webrtc::CreateTaskQueueWinFactory();
  auto task_queue_win = task_queue_win_factory->CreateTaskQueue("test", webrtc::TaskQueueFactory::Priority::HIGH);
  task_queue_win->PostDelayedHighPrecisionTask([] {
    std::cout << "Hello World111!\n";
    },5000);
  std::this_thread::sleep_for(std::chrono::seconds(10));
  std::cout << "Done1!\n";
  task_queue_win->PostTask([] {
    std::cout << "Hello World22!\n";
    });
  std::this_thread::sleep_for(std::chrono::seconds(5));
  std::cout << "Done2!\n";
  task_queue_win->PostTask([] {
    std::cout << "Hello World33!\n";
    });
  std::this_thread::sleep_for(std::chrono::seconds(5));
  std::cout << "Done3!\n";
  std::this_thread::sleep_for(std::chrono::seconds(10));
  return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

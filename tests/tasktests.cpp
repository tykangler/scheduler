#include <iostream>
#include "../src/Parallel/Task.hpp"

int main() {
   using namespace Parallel;
   TaskInfo taskinfo{Executor::make_closure([]() { std::cout << "TaskInfo\n"; })};
   taskinfo();
   taskinfo.mname = "taskinfo";
   taskinfo();
   auto exec = Executor::make_closure([]() { std::cout << "Executor\n"; });
   exec();
   exec();
   auto movedexec = std::move(exec);
   movedexec();
   TaskInfo movedtaskinfo = std::move(taskinfo);
   std::cout << movedtaskinfo.mname << '\n';
   movedtaskinfo();
}
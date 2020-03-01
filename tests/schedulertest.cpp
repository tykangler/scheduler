#include <iostream>
#include <windows.h>
#include "../src/Parallel/Scheduler.hpp"
#include "../src/Cipher/PBKDF2.hpp"

void complex() {
   Parallel::Scheduler scheduler{};
   auto a = scheduler.silent_add([]() { std::cout << "Task a\n"; });
   auto b = scheduler.silent_add([]() { std::cout << "Task b\n"; });
   auto c = scheduler.silent_add([]() { std::cout << "Task c\n"; });
   auto d = scheduler.silent_add([]() { std::cout << "Task d\n"; });
   auto e = scheduler.silent_add([]() { std::cout << "Task e\n"; });
   auto f = scheduler.silent_add([]() { std::cout << "Task f\n"; });
   auto g = scheduler.silent_add([]() { std::cout << "Task g\n"; });
   auto h = scheduler.silent_add([]() { std::cout << "Task h\n"; });
   auto i = scheduler.silent_add([]() { std::cout << "Task i\n"; });
   auto j = scheduler.silent_add([]() { std::cout << "Task j\n"; });
   scheduler.linearize(a, f, g, h, i, j);
   scheduler.direct(b, g);
   scheduler.direct(c, h);
   scheduler.direct(d, i);
   scheduler.direct(e, j);
   scheduler.execute();
   scheduler.wait();
}

int main(int argc, char **argv) {
   using namespace std::chrono;
   HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
   complex();
}
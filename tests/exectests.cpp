#include <iostream>
#include "../src/Parallel/Executor.hpp"

void print() {
   std::cout << 5 << "a\n";
}

int main() {
   auto test = Parallel::Executor::make_closure([]() { print(); });
   std::cout << "executor:\n";
   test();
   std::cout << "print():\n";
   print();
   std::cout << "now moving\n";
   auto moved = std::move(test);
   moved();
}
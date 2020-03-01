#include <string>
#include <cstdio>
#include <iostream>

#include "../src/ThreadPool/Scheduler.hpp"
// #include "../src/ThreadPool/OrderedGraph.hpp"

void blah1(int x, int y);
void blah2(int x, int y, double p);
void blah3(char x);
double blah4(int x, double y);

int main(int argc, char **argv) {
   Parallel::Scheduler schedule;

   /* adding and executing callbacks using silent_add(...) */

   auto d = schedule.silent_add(blah3, 'a');
   auto a = schedule.silent_add([]() {
      printf("%d\n", 2 * 4);
   });
   auto c = schedule.silent_add(blah2, 8, 9, 2.2);
   auto b = schedule.silent_add(blah1, 4, 7);

   std::cout << "silent_add(...): \n";
   Parallel::Task task_vec[] {std::move(a), std::move(b), std::move(c), std::move(d)};
   for (int i = 0; i < 4; i++) {
      task_vec[i]();
   }
   std::cout << '\n';

   /* adding and executing callbacks using add(...) */

   std::cout << "add(...): \n";
   auto [e, efu] = schedule.add(blah4, 4, 2.2);
   e();
   std::cout << efu.get() << '\n';
   auto g = schedule.silent_add([]() {});
   auto f = schedule.silent_add([]() {});
   auto i = schedule.silent_add([]() {});
   auto h = schedule.silent_add([]() {});
   auto j = schedule.silent_add([]() {});
   std::cout << '\n';

   /* giving names to nodes */

   a.name("a");
   b.name("b");
   c.name("c");
   d.name("d");
   e.name("e");
   f.name("f");
   g.name("g");
   h.name("h");
   i.name("i");
   j.name("j");

   /* drawing edges between nodes [a, ..., j] */

   schedule.linearize(a, f, g, h, i, j);
   schedule.direct(b, g);
   schedule.direct(c, h);
   schedule.direct(d, i);
   schedule.direct(e, j);


   /* print graph in unsorted order */

   std::cout << "Unsorted order: \n";
   schedule.print();
   std::cout << '\n';

   /* print graph in sorted order */

   std::cout << "topologically sorted order: \n";
   auto sorted = schedule.execute();
   for (auto &level : sorted) {
      for (auto node : level) {
         std::cout << node->mname << " at depth " << node->depth << '\n';
      }
   }
   std::cout << '\n';

   return 0;
}

void blah1(int x, int y){
   printf("%d\n", x * y);
}

void blah2(int x, int y, double p){
   printf("%f\n", x * y + p);
}

void blah3(char x){
   printf("%c\n", x);
}

double blah4(int x, double y) {
   return x * y;
}

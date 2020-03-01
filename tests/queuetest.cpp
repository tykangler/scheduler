#include <iostream>
#include <thread>
#include <string>
#include <initializer_list>
#include <condition_variable>
#include <mutex>
#include <cmath>
#include <windows.h>
#include <atomic>

#include "../src/Parallel/WorkStealingQueue.hpp"

using namespace Parallel;

namespace Test {

using Comparator = std::initializer_list<std::string>;

static constexpr int max_bufsize = Parallel::bs;

HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
std::condition_variable cv;
std::mutex mu;
std::atomic<bool> flag = false;
std::array<std::atomic<int>, 7> counter;

struct Worker {
   
   int own_id;
   std::thread worker;
   WorkStealingQueue jobs;
   Worker *sibling;

   Worker(int id) : own_id{id}, worker{multi_pop_steal, this, id} {}

   bool joinable() {
      return worker.joinable();
   }

   void join() {
      worker.join();
   }

   /* tests concurrent pops and steals. */
   void multi_pop_steal(int mid) {
      std::unique_lock lck{mu};
      if (!flag.load()) {
         std::cout << "Processor " << mid << " sleeping...\n";
         cv.wait(lck, []() { return flag.load(); });
         std::cout << "Processor " << mid << " awake...\n"; 
      }
      int popcount = 0;
      int stealcount = 0;
      lck.unlock();
      TaskInfo holder;
      while (!jobs.empty()) {
         if (jobs.pop(holder)) {
            holder();
            popcount++;
         }
      }
      while (!sibling->jobs.empty()) {
         if (sibling->jobs.steal(holder)) {
            holder();
            stealcount++;
         } 
      }
      lck.lock();
      std::cout << "Processor " << mid << " done...\n";
      std::cout << "   popcount = " << popcount << " tasks\n";
      std::cout << "   stealcount = " << stealcount << " tasks\n";
      lck.unlock();
   }
};

/* Tests whether given size is equal to size of queue, throws error if not */
void assert_size(int size, WorkStealingQueue &queue, const char *caller) {
   if (size != queue.size()) {
      SetConsoleTextAttribute(console, 12);
      std::cerr << "ERROR - " << caller << "()\n";
      std::cerr << "-----------------------------\n";
      SetConsoleTextAttribute(console, 15);
      std::cerr << "expected size = " << size << '\n';
      std::cerr << "actual size = " << queue.size() << '\n';
      exit(1);
   } 
   SetConsoleTextAttribute(console, 10);
   std::cout << caller << "() [assert_size] PASSED\n";
   SetConsoleTextAttribute(console, 15);
}

/* Tests whether the given list and the contents of the queue are equal, throws error if not.
If successful, the queue will be left empty */
void assert_equals(Comparator names, WorkStealingQueue &queue, const char *caller) {
   TaskInfo holder;
   int index = names.size();
   Comparator::iterator iter = names.end();
   while (iter != names.begin()) {
      iter--;
      index--;
      queue.pop(holder);
      if (holder.mname != *iter) {
         SetConsoleTextAttribute(console, 12);
         std::cerr << "ERROR - " << caller << "()\n";
         std::cerr << "-----------------------------\n";
         SetConsoleTextAttribute(console, 15);
         std::cerr << "expected at index " << index << ": \"" << *iter << "\"\n";
         std::cerr << "actual at index " << index << ": \"" << holder.mname << "\"\n";
         exit(1);
      }
   }
   SetConsoleTextAttribute(console, 10);
   std::cout << caller << "() [assert_equals] PASSED\n";
   SetConsoleTextAttribute(console, 15);
}

/* Tests whether the given queue is empty, throws error if not */
void assert_empty(WorkStealingQueue &queue, const char *caller) {
   if (!queue.empty()) {
      SetConsoleTextAttribute(console, 12);
      std::cerr << "ERROR - " << caller << "()\n";
      std::cerr << "-----------------------------\n";
      SetConsoleTextAttribute(console, 15);
      std::cerr << "queue should be empty but size is " << queue.size() << '\n';
      exit(1);
   }
   SetConsoleTextAttribute(console, 10);
   std::cout << caller << "() [assert_empty] PASSED\n";
   SetConsoleTextAttribute(console, 15);
}

/* pushes the given amount of elements allowed into the queue */
void push_many(int num, int id, WorkStealingQueue &queue) {
   num = std::min(num, max_bufsize);
   for (int i = 0; i < num; i++) {
      queue.push(TaskInfo{Executor::make_closure(
         [id]() {
            counter[id].fetch_add(1, std::memory_order_acq_rel);
         })
      });
   }
}

/* pushes the maximum amount of elements allowed into the queue, then pops all elements off */
void push_then_pop_all(WorkStealingQueue &queue) {
   while (queue.push(TaskInfo{})) {}
   assert_size(max_bufsize, queue, "push_then_pop_all");
   TaskInfo holder;
   while (queue.pop(holder)) {}
   assert_empty(queue, "push_then_pop_all");
}

/* pushes the maximum amount of elements allowed into the queue, then steals all elements */
void push_then_steal_all(WorkStealingQueue &queue) {
   while (queue.push(TaskInfo{})) {}
   assert_size(max_bufsize, queue, "push_then_steal_all");
   TaskInfo holder;
   while (queue.steal(holder)) {}
   assert_empty(queue, "push_then_steal_all");
}

/* tests whether the queue will pop an invalid element. If so, then throws an error */
void invalid_pop(WorkStealingQueue &queue) {
   TaskInfo a;
   a.num_deps = 1;
   queue.push(std::move(a));
   TaskInfo holder;
   if (queue.pop(holder)) {
      SetConsoleTextAttribute(console, 12);
      std::cerr << "ERROR - invalid_pop()\n";
      std::cerr << "-----------------------------\n";
      SetConsoleTextAttribute(console, 15);
      std::cerr << "queue popped but indegree was 1\n";
      exit(1);
   }
   assert_size(1, queue, "invalid_pop");
   queue.clear();
}

/* tests whether the queue will steal an invalid element. If so, then throws an error */
void invalid_steal(WorkStealingQueue &queue) {
   TaskInfo a;
   a.num_deps = 1;
   queue.push(std::move(a));
   TaskInfo holder;
   if (queue.steal(holder)) {
      SetConsoleTextAttribute(console, 12);
      std::cerr << "ERROR - invalid_pop()\n";
      std::cerr << "-----------------------------\n";
      SetConsoleTextAttribute(console, 15);
      std::cerr << "queue stolen from but indegree was " << a.num_deps << '\n';
      exit(1);
   }
   assert_size(1, queue, "invalid_steal");
   queue.clear();
}

/* initializes queue with three elements, each with names "a", "b", and "c" */
void init_elems(WorkStealingQueue &queue) {
   TaskInfo a, b, c;
   a.mname = "a";
   b.mname = "b";
   c.mname = "c";
   queue.push(std::move(a));
   queue.push(std::move(b));
   queue.push(std::move(c));
}

/* initializes 3 elements in the queue */
void basic_push(WorkStealingQueue &queue) {
   init_elems(queue);
   assert_size(3, queue, "basic_push");
   assert_equals({"a", "b", "c"}, queue, "basic_push");
   assert_empty(queue, "basic_push");
}

/* initializes 3 elements in the queue, then pops one element */
void basic_pop(WorkStealingQueue &queue) {
   init_elems(queue);
   TaskInfo c;
   queue.pop(c);
   assert_size(2, queue, "basic_pop");
   assert_equals({"a", "b"}, queue, "basic_pop");
   assert_empty(queue, "basic_pop");
}

/* initializes 3 elements in the queue, then steals one element */
void basic_steal(WorkStealingQueue &queue) {
   init_elems(queue);
   TaskInfo a;
   queue.steal(a);
   assert_size(2, queue, "basic_steal");
   assert_equals({"b", "c"}, queue, "basic_steal");
   assert_empty(queue, "basic_steal");
}

int main() {
   {
      WorkStealingQueue queue{};

      /* Basic Single-Threaded Tests */
      std::cout << "single-threaded...\n\n";
      basic_push(queue);
      basic_pop(queue);
      basic_steal(queue);
      push_then_pop_all(queue);
      push_then_steal_all(queue);
      invalid_pop(queue);
      invalid_steal(queue);
   }
   std::array<int, 7> expected;
   {
      /* Initializing Threads */
      std::cout << "\ninitializing processors...\n" << std::endl;
      std::vector<Worker> threads;
      int num_threads = std::thread::hardware_concurrency() - 1;
      threads.reserve(num_threads);
      threads.emplace_back(0);
      push_many(max_bufsize / 7, 0, threads[0].jobs);
      expected[0] = max_bufsize / 7;
      for (int i = 1; i < num_threads; i++) {
         threads.emplace_back(i);
         expected[i] = max_bufsize * (i + 1) / 7;
         push_many(max_bufsize * (i + 1) / 7, i, threads[i].jobs);
         threads[i - 1].sibling = &threads[i];
      }
      threads[num_threads - 1].sibling = &threads[0];

      /* Wait for input for threads to start popping/stealing */
      if (std::cin.get()) {
         flag = true;
         cv.notify_all();
      }
      /* wait for threads to join */
      for (auto &t : threads) {
         if (t.joinable()) {
            t.join();
         }
      }
   }

   /* print counter values */
   std::cout << "result values: \n";
   std::cout << "[" << counter[0];
   for (int i = 1; i < counter.size(); i++) {
      std::cout << ", " << counter[i];
   }
   std::cout << "]\n\n";

   /* print expected values */
   std::cout << "expected values: \n";
   std::cout << "[" << expected[0];
   for (int i = 1; i < expected.size(); i++) {
      std::cout << ", " << expected[i];
   }
   std::cout << "]\n\n";
   
   int errorcnt = 0;
   for (int i = 0; i < expected.size(); i++) {
      if (counter[i] != expected[i]) {
         if (errorcnt <= 0) {
            SetConsoleTextAttribute(console, 12);
            std::cerr << "ERROR - values don't match!\n";
            std::cerr << "-----------------------------\n";
            SetConsoleTextAttribute(console, 15);
         }
         SetConsoleTextAttribute(console, 14);
         std::cerr << "at index " << i << ":\n";
         SetConsoleTextAttribute(console, 15);
         std::cerr << "   actual   <" << counter[i] << "> " << '\n';
         std::cerr << "   expected <" << expected[i] << "> " << '\n';
         errorcnt++;
      }
   }
   if (errorcnt <= 0) {
      SetConsoleTextAttribute(console, 10);
      std::cout << "counter values match!\n";
      SetConsoleTextAttribute(console, 15);
   }

   return 0;
}

}

int main() {
   Test::main();
}
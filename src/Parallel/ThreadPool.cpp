#include "ThreadPool.hpp"
#include <iostream>

namespace Parallel {

Worker::Worker(ThreadPool *parent) : 
 employer{parent}, thread{Worker::work, this, parent} {}

Worker::~Worker() {
   join();
}

void Worker::assign(TaskInfo *task) {
   jobs.push(task);
}

void Worker::join() {
   if (thread.joinable()) {
      thread.join();
   }
}

void Worker::work(ThreadPool *parent) {
   while (!parent->running.load(std::memory_order_acquire)) {
      std::unique_lock locker{parent->lck_dev};
      parent->cond.wait(locker, [this, parent]() { 
         return parent->running.load() || parent->done.load(); 
      });
   }
   TaskInfo *task = nullptr;
   while (parent->running.load() && !parent->done.load()) {
      if (!jobs.empty()) {
         pop_run(task);
      } else {
         steal_run(task);
      }
   }
}

void Worker::pop_run(TaskInfo *task) {
   task = jobs.pop();
   if (task != nullptr) {
      if (task->num_deps != 0) {
         std::unique_lock locker{employer->lck_dev};
         employer->cond.wait(locker, [task]() { return task->num_deps == 0; });
      }
      (*task)();
      for (auto dep : task->dests) { 
         dep->num_deps.fetch_sub(1, std::memory_order_release);
      }
      employer->cond.notify_all();
   } 
}

void Worker::steal_run(TaskInfo *task) {
   Worker *victim = sibling;
   while (victim != this) {
      if (!victim->jobs.empty()) {
         task = victim->jobs.steal();
         if (task != nullptr) {
            if (task->num_deps != 0) { // impl a peek method, and maybe an empty check for all methods
               std::unique_lock locker{employer->lck_dev};
               employer->cond.wait(locker, [task]() { return task->num_deps == 0; });
            }
            (*task)();
            for (auto dep : task->dests) { 
               dep->num_deps.fetch_sub(1, std::memory_order_release);
            }
            employer->cond.notify_all();
         } 
      } else {
         std::this_thread::yield();
         victim = victim->sibling;
      }
   }
}

ThreadPool::ThreadPool(const int numthreads) : running{false}, done{false} {
   workers.reserve(numthreads);
   workers.emplace_back(this);
   for (int i = 1; i < numthreads; i++) {
      workers.emplace_back(this);
      workers[i - 1].set_sibling(&workers[i]);
   }
   workers[numthreads - 1].set_sibling(&workers[0]);
}

ThreadPool::~ThreadPool() {
   done.store(true, std::memory_order_release);
   running.store(false, std::memory_order_release);
   cond.notify_all();
}

void ThreadPool::dispatch(Topology &topology) {
   int processor{};
   int number_threads = workers.size();
   for (int i = topology.size() - 1; i >= 0; i--) {
      for (int j = 0; j < topology[i].size(); j++) {
         workers[processor % number_threads].assign(topology[i][j]);
         processor++;
      }
   }
   running.store(true, std::memory_order_release);
   cond.notify_all();
}

void ThreadPool::wait_for_all() {
   for (int i = 0; i < workers.size(); i++) {
      workers[i].join();
   }
}

}
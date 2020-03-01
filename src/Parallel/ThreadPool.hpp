#ifndef THREADPOOLHPP
#define THREADPOOLHPP

#include <thread>
#include <condition_variable>
#include <mutex>
#include <cstdio>
#include <atomic>

#include "Task.hpp"
#include "WorkStealingQueue.hpp"

namespace Parallel {

class Worker;
using Topology = std::vector<std::vector<TaskInfo*>>;

class ThreadPool {
   friend class Worker; 
   public:
      ThreadPool(const int numthreads = std::thread::hardware_concurrency() - 1);
      ~ThreadPool();
      void dispatch(Topology &topology);
      void wait_for_all();
   private:
      std::vector<Worker> workers; 
      std::condition_variable cond;
      std::mutex lck_dev;
      std::atomic<bool> running;
      std::atomic<bool> done;
};

class Worker {
   public:
      Worker(ThreadPool*);
      Worker(Worker&) =delete;
      Worker(Worker&&) =default;
      ~Worker();
      void set_sibling(Worker *next) { sibling = next; }
      void assign(TaskInfo *task);
      void join();
   private:
      void work(ThreadPool*);
      void pop_run(TaskInfo*);
      void steal_run(TaskInfo*);
      std::thread thread;
      WorkStealingQueue jobs;
      ThreadPool *employer;
      Worker *sibling;
};

}

#endif
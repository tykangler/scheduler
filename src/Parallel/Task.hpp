#ifndef TASKHPP
#define TASKHPP

#include <string>
#include <vector>
#include <unordered_set>
#include <atomic>

#include "Executor.hpp"

namespace Parallel {

struct TaskInfo {
   TaskInfo() : depth{0}, num_deps{0}, visiting{false} {}
   TaskInfo(Executor &&exec) : exec{std::move(exec)}, depth{0}, num_deps{0}, visiting{false} {}
   ~TaskInfo() {}
   TaskInfo(const TaskInfo&) =delete;
   TaskInfo &operator=(const TaskInfo&) =delete;
   TaskInfo(TaskInfo &&other) noexcept : 
      exec{std::move(other.exec)}, depth{other.depth}, dests{std::move(other.dests)}, 
      num_deps{other.num_deps.load()}, visiting{other.visiting} {}

   TaskInfo &operator=(TaskInfo &&other)  {
      exec = std::move(other.exec);
      dests = std::move(other.dests);
      depth = other.depth;
      num_deps = other.num_deps.load();
      visiting = other.visiting;
      return *this;
   }

   void operator()() { exec(); }

   void add_dep(TaskInfo *next) { 
      auto valid = dests.insert(next); 
      if (valid.second) {
         next->num_deps++;
      } 
   }

   Executor exec; 
   std::unordered_set<TaskInfo*> dests;
   int depth;
   std::atomic<int> num_deps;
   bool visiting;
};

/* Wrapper for vertex node, public facing */
class Task {
   public:
      Task();
      Task(TaskInfo &carry) : node{&carry} {} 
      Task(const Task&) =delete;
      Task &operator=(const Task&) =delete;
      Task(Task &&other) : node{other.node} {}
      Task &operator=(Task &&other) { this->node = other.node; return *this; }

      void operator()() { (*node)(); }

   private:
      TaskInfo *node;
      friend class Scheduler;
};

}

#endif

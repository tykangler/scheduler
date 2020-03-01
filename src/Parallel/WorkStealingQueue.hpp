#ifndef MPMCQUEUEHPP
#define MPMCQUEUEHPP

#include <atomic>
#include <memory>

#include "Errors.hpp"
#include "Task.hpp"

namespace Parallel {

constexpr static int cache_size = 64;
constexpr static int bs = 32768;

class WorkStealingQueue {
   public: 
      WorkStealingQueue(const int buf_size = 32);
      ~WorkStealingQueue();
      WorkStealingQueue(WorkStealingQueue&) =delete;
      WorkStealingQueue &operator=(WorkStealingQueue&) =delete;
      WorkStealingQueue(WorkStealingQueue&&);
      WorkStealingQueue &operator=(WorkStealingQueue&&);

      bool push(TaskInfo *val);
      TaskInfo *pop();
      TaskInfo *steal();
      TaskInfo *peek_front();
      void clear();
      bool empty() const;
      bool full() const;
      int size();
      const int size() const;

   private:
      alignas(cache_size) TaskInfo **buffer;
      int mask;
      std::atomic<size_t> front;
      std::atomic<size_t> back;
};

}

#endif

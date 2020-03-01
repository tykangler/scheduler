#include "WorkStealingQueue.hpp"

#define COMPILER_BARRIER asm volatile("" ::: "memory")
#define MEMORY_BARRIER asm volatile("mfence" ::: "memory")

namespace Parallel {

WorkStealingQueue::WorkStealingQueue(const int buf_size) : 
buffer{new TaskInfo*[buf_size]}, mask{buf_size - 1}, front{0}, back{0} {
   if (buf_size & mask != 0) {
      throw std::logic_error{"WorkStealingQueue buffer size must be a power of 2"};
   }
}

WorkStealingQueue::~WorkStealingQueue() {
   delete[] buffer;
}

WorkStealingQueue::WorkStealingQueue(WorkStealingQueue &&other) : 
buffer{other.buffer}, mask{other.mask}, front{other.front.load()}, 
back{other.back.load()} {}

WorkStealingQueue &WorkStealingQueue::operator=(WorkStealingQueue &&other) {
   if (&other != this) { 
      buffer = other.buffer;
      mask = other.mask;
      front.store(other.front.load());
      back.store(other.back.load());
   }
   return *this;
}

bool WorkStealingQueue::push(TaskInfo *val) {
   if (!full()) {
      size_t b = back.load(std::memory_order_acquire);
      buffer[b & mask] = val;
      COMPILER_BARRIER;
      back.store(b + 1, std::memory_order_release);
      return true;
   }
   return false;
}

TaskInfo *WorkStealingQueue::pop() {
   size_t b = back.load(std::memory_order_acquire) - 1;
   back.store(b, std::memory_order_release);
   MEMORY_BARRIER;
   size_t f = front.load(std::memory_order_acquire);
   if (f <= b) {
      TaskInfo *task = buffer[b & mask];
      if (f != b) {
         return task;
      } 
      if (!front.compare_exchange_strong(f, f + 1)) {
         task = nullptr;
      }
      back.store(f + 1, std::memory_order_release);
      return task;
   } else {
      back.store(f, std::memory_order_release);
      return nullptr;
   }
} 

TaskInfo *WorkStealingQueue::steal() {
   size_t f = front.load(std::memory_order_acquire);
   COMPILER_BARRIER;
   if (f < back.load(std::memory_order_acquire)) {
      TaskInfo *task = buffer[f & mask];
      if (front.compare_exchange_strong(f, f + 1)) {
         return task;
      }
   }
   return nullptr;
}

TaskInfo *WorkStealingQueue::peek_front() {
   if (!empty()) {
      return buffer[front.load(std::memory_order_acquire) & mask];
   }
   return nullptr;
}

void WorkStealingQueue::clear() {
   front.store(0);
   back.store(0);
}

bool WorkStealingQueue::empty() const {
   return front >= back;
}

bool WorkStealingQueue::full() const {
   return back - front == mask + 1;
}

int WorkStealingQueue::size() {
   return back - front;
}

const int WorkStealingQueue::size() const {
   return back - front;
}

}
#ifndef SCHEDULERHPP
#define SCHEDULERHPP 

#include <future>
#include <utility>
#include <list>
#include <iostream>
#include <vector>

#include "Task.hpp"
#include "ThreadPool.hpp"

namespace Parallel {

/* non-copy constructible/assignable task dependency graph,
directed and acyclic, handles submission and direction of tasks */
class Scheduler {
   public:
      Scheduler() : vertices{} {}
      ~Scheduler() {}
      Scheduler(Scheduler&) =delete;
      Scheduler &operator=(Scheduler&) =delete;
      Scheduler(Scheduler&&) noexcept =default;
      Scheduler &operator=(Scheduler&&) noexcept =default;

      /* adds a void-returning task to the graph, returns a handle to the task */
      template<typename Func>
      Task silent_add(Func &&task);
      template<typename Func, typename... Args>
      Task silent_add(Func &&task, Args&&... args);

      /* adds a non-void returning task to the graph, returns a handle to the task */
      template<typename Func>
      auto add(Func &&task) 
         -> std::pair<Task, std::future<decltype(task())>>;
      template<typename Func, typename... Args>
      auto add(Func &&task, Args&&... args)
         -> std::pair<Task, std::future<decltype(task(args...))>>;

      /* creates separate dependencies from root to all listed targets */
      template<typename... T>
      void direct(Task &root, T&... targets);

      /* creates linear dependencies from root to last target listed */
      template<typename T>
      void linearize(Task &root, T &last);
      template<typename T, typename... Tp>
      void linearize(Task &root, T &first, Tp&... rest);

      /* debug: passes off to threadpool, executes task graph */
      void execute();

      /* wait for threads to finish executing */
      void wait();      
   private:
      std::list<TaskInfo> vertices;
      ThreadPool threads;
};

/* Implementation */

template<typename Func>
Task Scheduler::silent_add(Func &&task) {
   vertices.emplace_back(Executor::make_closure(std::forward<Func>(task)));
   return Task{vertices.back()};
}

template<typename Func, typename... Args>
Task Scheduler::silent_add(Func &&task, Args&&... args) {
   vertices.emplace_back(Executor::make_closure(
      [task = std::forward<Func>(task), 
      args_tup = std::make_tuple(std::forward<Args>(args)...)]() {
         std::apply([&task](auto&... args) {
            task(std::forward<decltype(args)>(args)...);
         }, args_tup);
      })
   );
   return Task{vertices.back()};
}

template<typename Func>
auto Scheduler::add(Func &&task) 
-> std::pair<Task, std::future<decltype(task())>> {
   using RetType = decltype(task());
   std::promise<RetType> ret_promise;
   std::future<RetType> ret = ret_promise.get_future();
   auto closure = Executor::make_closure(
      [ret_promise = std::move(ret_promise),
      task = std::forward<Func>(task)]() mutable {
         ret_promise.set_value(task());
      }
   );
   vertices.emplace_back(std::move(closure));
   return std::make_pair(Task{vertices.back()}, std::move(ret));
}

template<typename Func, typename... Args>
auto Scheduler::add(Func &&task, Args&&... args) 
-> std::pair<Task, std::future<decltype(task(args...))>> {
   using RetType = decltype(task(args...));
   std::promise<RetType> ret_promise;
   std::future<RetType> ret = ret_promise.get_future();
   auto closure = Executor::make_closure(
      [ret_promise = std::move(ret_promise),
      task = std::forward<Func>(task),
      args_tup = std::make_tuple(std::forward<Args>(args)...)]() mutable {
         ret_promise.set_value(std::apply(
            [&task](auto&... args) -> RetType {
               return task(std::forward<decltype(args)>(args)...);
            }, args_tup));
      }
   ); 
   vertices.emplace_back(std::move(closure));
   return std::make_pair(Task{vertices.back()}, std::move(ret));
}

template<typename... T>
void Scheduler::direct(Task &root, T&... targets) {
   static_assert(sizeof...(targets) > 0, "root must direct targets");
   static_assert((std::is_same_v<Task, T> && ...), "only Tasks may direct");
   (root.node->add_dep(targets.node), ...);
}

template<typename T>
void Scheduler::linearize(Task &root, T &last) {
   static_assert(std::is_same_v<Task, T>, "only Tasks may linearize");
   root.node->add_dep(last.node);
}

template<typename T, typename... Tp>
void Scheduler::linearize(Task &root, T &first, Tp&... rest) {
   static_assert(std::is_same_v<Task, T> && (std::is_same_v<Task, Tp> && ...), 
      "only Tasks may linearize");
   root.node->add_dep(first.node);
   linearize(first, rest...);
}

}

#endif
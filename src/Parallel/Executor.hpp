#ifndef EXECUTORHPP
#define EXECUTORHPP

#include <new>
#include <array>
#include <utility>
#include <type_traits>

namespace Parallel {

class alignas(64) Executor {
   public:
      Executor() { sbstore.fill(0); }
      Executor(Executor &&other) noexcept {
         sbstore = std::move(other.sbstore);
         other.sbstore.fill(0);
      }

      Executor &operator=(Executor &&other) noexcept {
         sbstore = std::move(other.sbstore);
         other.sbstore.fill(0);
         return *this;
      }
      
      template<typename Func>
      static Executor make_closure(Func &&callable);

      void operator()() { 
         get_task().execute();
      }
   private:
      struct TaskBase {
         TaskBase() {}
         TaskBase(const TaskBase&) =delete;
         TaskBase &operator=(const TaskBase&) =delete;
         virtual ~TaskBase() {}
         virtual void execute() =0;
      };

      template<typename Func>
      struct Closure final : public TaskBase {
         Closure() {}
         ~Closure() {}
         Closure(Func &&f) : task{std::forward<Func>(f)} {}
         Closure(const Closure&) =delete;
         Closure &operator=(const Closure&) =delete;
         Closure(Closure &&t) noexcept : task{std::move(t.task)} {}
         Closure &operator=(Closure &&t) noexcept { 
            task = std::move(t.task); 
            return *this; 
         }
         void execute() override final { 
            task(); 
         }

         Func task;
      };
      using Storage = std::array<unsigned long, 16>;
   private: 
      Executor(const Executor&) =delete;
      Executor &operator=(const Executor&) =delete;

      void *data()                     { return static_cast<void*>(sbstore.data()); }
      const void *data() const         { return static_cast<const void*>(sbstore.data()); }
      TaskBase &get_task()             { return *static_cast<TaskBase*>(data()); }
      const TaskBase &get_task() const { return *static_cast<const TaskBase*>(data()); }
   private:
      Storage sbstore;
};

/* Implementation */

template<typename Func>
Executor Executor::make_closure(Func &&callable) {
   static_assert(alignof(callable) <= alignof(Executor), 
      "alignment of callable must not exceed that of Executor class");
   static_assert(sizeof(callable) <= sizeof(sbstore), 
      "size of callable may not exceed 64 bytes");
   Executor init{};
   new(init.sbstore.data()) Closure<Func>{std::forward<Func>(callable)};
   return std::move(init);
}

}

#endif
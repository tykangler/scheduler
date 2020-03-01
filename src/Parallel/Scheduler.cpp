#include "Scheduler.hpp"

namespace Parallel {

namespace {

int assign_depth(int depth, TaskInfo &node) {
   if (node.visiting) {
      throw std::logic_error{"task dependencies must be acyclic"};
   }
   node.visiting = true;
   node.depth = (node.depth > depth) ? node.depth : depth;
   int max_depth = node.depth;
   for (auto *edge : node.dests) {
      max_depth = assign_depth(depth + 1, *edge);
   }
   node.visiting = false; 
   return max_depth;
}

}

void Scheduler::execute() {
   if (vertices.empty()) {
      throw std::logic_error{"execute() must execute tasks"};
   }
   int max_depth{};
   for (auto &node : vertices) {
      int curr_depth = 0;
      if (node.num_deps == 0) {
         curr_depth = assign_depth(0, node);
      }
      max_depth = (max_depth > curr_depth) ? max_depth : curr_depth;
   }
   Topology sorted;
   sorted.resize(max_depth + 1);
   for (auto &node : vertices) {
      sorted[node.depth].push_back(&node);
   }
   threads.dispatch(sorted);
}

void Scheduler::wait() {
   threads.wait_for_all();
}

}
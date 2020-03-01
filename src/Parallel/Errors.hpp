#ifndef PARRERRORSHPP
#define PARRERRORSHPP

#include <stdexcept>

namespace Parallel {

struct cache_error : public std::logic_error {
   public:
      const char *whatmessage;
      cache_error(const char* message) : std::logic_error{message}, whatmessage{message} {}
      const char *what() const noexcept { return whatmessage; }
};

}

#endif


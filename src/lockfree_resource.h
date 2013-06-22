#ifndef LOCKFREE_RESOURCE_H_
#define LOCKFREE_RESOURCE_H_

#include <array>
#include <atomic>

namespace midiaud {

template <typename T, size_t Size = 3>
class LockfreeResource {
 public:
  LockfreeResource();

  template <typename... Args> bool Emplace(Args &&... args);
  T *Fetch();

 private:
  T data_[Size];
  std::atomic<ptrdiff_t> read_offset_;
  std::atomic<ptrdiff_t> write_offset_;
};

}

#endif // LOCKFREE_RESOURCE_H_

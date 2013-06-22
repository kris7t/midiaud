#ifndef LOCKFREE_RESOURCE_INL_H_
#define LOCKFREE_RESOURCE_INL_H_

namespace midiaud {

template <typename T, size_t Size>
LockfreeResource<T, Size>::LockfreeResource()
    : read_offset_(Size - 1), write_offset_(0) {
}

template <typename T, size_t Size>
template <typename... Args>
bool LockfreeResource<T, Size>::Emplace(Args &&... args) {
  ptrdiff_t read_offset = read_offset_.load(std::memory_order_acquire);
  ptrdiff_t write_offset = write_offset_.load(std::memory_order_relaxed);
  ptrdiff_t next_write_offset = (write_offset + 1) % Size;
  if (next_write_offset == read_offset) return false;
  data_[write_offset] = T(std::forward<Args>(args)...);
  write_offset_.store(next_write_offset, std::memory_order_release);
  return true;
}

template <typename T, size_t Size>
T *LockfreeResource<T, Size>::Fetch() {
  ptrdiff_t write_offset = write_offset_.load(std::memory_order_acquire);
  ptrdiff_t read_offset = (write_offset + Size - 1) % Size;
  read_offset_.store(read_offset, std::memory_order_release);
  return &data_[read_offset];
}

}

#endif // LOCKFREE_RESOURCE_INL_H_

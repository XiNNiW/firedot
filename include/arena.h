#pragma once

#include <cstddef>
#include <cstdlib>
class Arena {
private:
  char *data;
  size_t position = 0;
  size_t size = 0;

public:
  Arena(size_t _size) : size(_size), position(0), data((char *)malloc(_size)) {}
  template <typename T> bool canAlloc() const {
    return (position + sizeof(T)) < size;
  }
  template <typename T> bool canAllocArray(size_t length) const {
    size_t desiredSize = (position + (sizeof(T) * length));
    return desiredSize < size;
  }
  template <typename T> T *push() {
    size_t size = sizeof(T);
    void *ptr = data + position;
    position += size;
    return (T *)ptr;
  }
  template <typename T> T *pushArray(size_t length) {
    size_t size = sizeof(T) * length;
    void *ptr = data + position;
    position += size;
    return (T *)ptr;
  }
  template <typename T> void pop() { position -= sizeof(T); }
  inline void clear() { position = 0; }
  inline size_t getPosition() { return position; }
  ~Arena() { free(data); }
};

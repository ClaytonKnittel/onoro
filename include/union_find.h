#pragma once

#include <stdint.h>

template <typename T>
class UnionFind {
 public:
  UnionFind(uint32_t capacity);
  ~UnionFind();

  UnionFind(const UnionFind&) = delete;
  UnionFind(UnionFind&&) = default;

  uint32_t GetNumGroups() const;

  uint32_t GetRoot(uint32_t idx);

  // Returns the index of the parent of this element's set.
  uint32_t Find(uint32_t idx);

  // Unions the two elements, returning the new parent of both.
  uint32_t Union(uint32_t a, uint32_t b);

 private:
  T* buffer_;
  uint32_t buffer_size_;
  uint32_t n_groups_;
};

template <typename T>
UnionFind<T>::UnionFind(uint32_t capacity)
    : buffer_size_(capacity), n_groups_(capacity) {
  buffer_ = new T[capacity];
  for (uint32_t i = 0; i < capacity; i++) {
    buffer_[i] = i;
  }
}

template <typename T>
UnionFind<T>::~UnionFind() {
  delete[] buffer_;
}

template <typename T>
uint32_t UnionFind<T>::GetNumGroups() const {
  return n_groups_;
}

template <typename T>
uint32_t UnionFind<T>::GetRoot(uint32_t idx) {
  uint32_t parent = buffer_[idx];

  // TODO see if this actually helps.
  // The common case, after the tree has been flattened.
  if (buffer_[parent] == parent) {
    return parent;
  }

  while (parent != idx) {
    uint32_t pp = buffer_[parent];
    buffer_[idx] = pp;

    idx = parent;
    parent = pp;
  }

  return idx;
}

template <typename T>
uint32_t UnionFind<T>::Find(uint32_t idx) {
  return GetRoot(idx);
}

template <typename T>
uint32_t UnionFind<T>::Union(uint32_t a, uint32_t b) {
  uint32_t ra = GetRoot(a);
  uint32_t rb = GetRoot(b);

  if (ra != rb) {
    buffer_[rb] = ra;
    n_groups_--;
  }

  return ra;
}

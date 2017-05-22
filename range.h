#pragma once

#include <assert.h>

namespace DynoGraph {

// Provides access to a contiguous array of data
// The goal of this class is to provide an interface similar to a std::vector<T>,
// while allowing flexibility in where the data are actually stored.
template<typename T>
class Range
{
public:
    // Returns a pointer to the start of the range
    T* begin() const { return begin_iter; }
    // Returns a pointer to one past the end of the range
    T* end() const { return end_iter; }
    // Constructs an empty range
    Range() : begin_iter(0), end_iter(0) {}
    // Constructs a range from a start and end iterator
    template<typename iterator>
    Range(iterator begin, iterator end)
    : begin_iter(&*begin), end_iter(&*end) {}
    // Constructs a range from a container like std::vector<T>
    template<typename Container>
    Range(Container& container)
    : Range(&*container.begin(), &*container.end()) {}
    // Allows array-like indexing
    T& operator[] (size_t i) {
        assert(begin_iter + i < end_iter);
        return *(begin_iter + i);
    }
    // Allows array-like indexing (const version)
    const T& operator[] (size_t i) const {
        assert(begin_iter + i < end_iter);
        return *(begin_iter + i);
    }
    // Returns the size of the range
    size_t size() const { return end_iter - begin_iter; }
    // Virtual destructor
    virtual ~Range() = default;
protected:
    // Pointer to the start of the range
    T* begin_iter;
    // Pointer to one past the end of the range
    T* end_iter;
};

} // end namespace DynoGraph
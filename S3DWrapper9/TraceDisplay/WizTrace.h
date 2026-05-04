// WizTrace.h
// Clean-room implementation of a generic circular trace buffer for wiz3D.
// See if necissary, and either remove if not or implement properly if needed. This is a placeholder for now.

#ifndef WIZ_TRACE_H
#define WIZ_TRACE_H

#include <vector>
#include <string>
#include <algorithm>

template <class T>
class WizTrace {
private:
    std::vector<T> _data;
    size_t _capacity;
    size_t _head;
    bool _is_full;
    T _max_val;
    T _min_val;
    std::string _name;

public:
    WizTrace(size_t capacity = 1000, const std::string& name = "")
        : _capacity(capacity), _head(0), _is_full(false),
        _max_val(T()), _min_val(T()), _name(name) {
    }

    ~WizTrace() {}

    void resize() {
        _data.resize(_capacity);
    }

    void clear() {
        _data.clear();
        _head = 0;
        _is_full = false;
    }

    void insert(const T item) {
        if (_head == 0 && !_is_full) {
            _max_val = item;
            _min_val = item;
        }
        else {
            if (item > _max_val) _max_val = item;
            if (item < _min_val) _min_val = item;
        }

        if (_data.size() < _capacity) {
            _data.resize(_capacity); // Ensure vector is sized before writing
        }

        _data[_head] = item;
        _head++;

        if (_head >= _capacity) {
            _head = 0;
            _is_full = true;
        }
    }

    size_t capacity() const { return _capacity; }
    void capacity(size_t c) { _capacity = c; }
    size_t size() const { return _is_full ? _capacity : _head; }

    T maxVal() const { return _max_val; }
    void maxVal(T m) { _max_val = m; }
    T minVal() const { return _min_val; }
    void minVal(T m) { _min_val = m; }

    std::string name() const { return _name; }
    void name(const std::string& s) { _name = s; }

    // Raw index access
    T operator[](size_t i) const { return _data[i]; }

    // Circular chronological access
    T operator()(size_t i) const {
        return _data[(i + (_is_full ? _head : 0)) % _capacity];
    }

    T last() const {
        size_t idx = (_head == 0) ? _capacity - 1 : _head - 1;
        return _data[idx];
    }

    T secondToLast() const {
        size_t idx = (_head < 2) ? _capacity - (2 - _head) : _head - 2;
        return _data[idx];
    }
};

#endif // WIZ_TRACE_H
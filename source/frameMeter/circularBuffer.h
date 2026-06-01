#pragma once

#include <vector>

template<typename T>
class CircularBuffer {
public:
    CircularBuffer(int size) : _buffer(size) { }

    int size() { return _buffer.size(); }
    int index() { return _index; }
    int maxIndex() { return std::max(_index, size() - 1); }
    int minIndex() { return std::min(_index - size() - 1, 0); }

    void UpdateFrameIndex(int offset) {
        _index += offset;
    }
    void Rollback(int offset) {
        _index -= offset;
    }
    void Add(T&& element) {
        _buffer[WrapIndex(_index++)] = element;
    }
    T Get(int offset) {
        return _buffer[WrapIndex(_index + offset)];
    }

private:
    std::vector<T> _buffer;
    int _index = 0;

    inline int WrapIndex(int index) {
        int r = index % size();
        return r < 0 ? r + size() : r;
    }
};

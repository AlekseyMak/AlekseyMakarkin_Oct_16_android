//
// Created by Andrey on 5/19/18.
//

#ifndef ALANVOICE_BUFFER_H
#define ALANVOICE_BUFFER_H

#include "Utils.h"
#include "Logger.h"
#include "Exception.h"

namespace sky {

template<typename T_>
class Buffer {
  public:
    const T_ * read() {
        return _result.get() + _readIdx;
    }

    size_t size() const {
        return _writeIdx - _readIdx;
    }

    constexpr unsigned int elementSize() const {
        return sizeof(T_);
    }

    void consume(size_t s) {
        if (s > size()) {
            Err<<"invalid consume: "<<s<<", readIdx: "<<_readIdx
               <<", writeIdx: "<<_writeIdx<<", capacity: "<<_capacity
               <<eol;
            throw Exception EX("exceed size");
        }
        _readIdx += s;
        if (_readIdx == _writeIdx) {
            _readIdx = _writeIdx = 0;
        }
    }

    T_ * write(size_t s) {
        if (!_result) {
            _result.reset(new T_[s]);
            _readIdx = _writeIdx = 0;
            _capacity = s;
        } else if (s > _capacity - size()) {
            allocate(s + size());
        } else if (_capacity - _writeIdx < s) {
            std::copy(_result.get() + _readIdx, _result.get() + _readIdx + size(), _result.get());
            _writeIdx = size();
            _readIdx = 0;
        }
        return _result.get() + _writeIdx;
    }

    void write(const T_ * data, size_t size) {
        std::copy(data, data + size, write(size));
        commit(size);
    }

    void commit(size_t s) {
        if (s > _capacity - _writeIdx) {
            Err<<"invalid commit: "<<s<<", readIdx: "<<_readIdx
               <<", writeIdx: "<<_writeIdx<<", capacity: "<<_capacity
               <<eol;
            throw Exception EX("commit exceed");
        }
        _writeIdx += s;
    }

  private:
    void allocate(size_t cap) {
        T_ *n = new T_[cap];
        auto s = size();
        memcpy(n, _result.get() + _readIdx, s * sizeof(T_));
        _readIdx = 0;
        _writeIdx = s;
        _capacity = cap;
        _result.reset(n);
    }

    size_t _readIdx{0};
    size_t _writeIdx{0};
    size_t _capacity{0};
    std::unique_ptr<T_[]> _result;
};

}

#endif //ALANVOICE_BUFFER_H

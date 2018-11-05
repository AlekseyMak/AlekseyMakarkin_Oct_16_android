//
// Created by Andrey on 5/1/18.
//

#ifndef ALANVOICE_ALANFRAME_H
#define ALANVOICE_ALANFRAME_H

#include "Utils.h"
#include "Exception.h"

namespace sky {

class AlanFrame;

class FieldBase {
  public:
    bool           isSet() const;
    uint8_t        type() const;
    virtual size_t writeSize() const = 0;
    virtual void   write(uint8_t *) = 0;
    virtual void   parse(const uint8_t *) = 0;

    FieldBase(AlanFrame * alan, uint8_t type);
  protected:
    friend class AlanFrame;

    const uint8_t _type;
    bool          _set;
};

template<typename T_>
class FrameField : public FieldBase {
  public:
    size_t writeSize() const override {
        return sizeof(T_);
    }

    void write(uint8_t * output) override {
        assert(isSet());
        *reinterpret_cast<T_*>(output) = _value;
    }

    void parse(const uint8_t * input) override {
        set(*reinterpret_cast<const T_*>(input));
    }

    void set(const T_ & value) {
        _set = true;
        _value = value;
    }

    const T_ & get() const {
        return _value;
    }

    operator T_ () const {
        return _value;
    }

    FrameField & operator = (const T_ & v) {
        set(v);
        return * this;
    }

    using FieldBase::FieldBase;

  private:
    T_ _value;
};

template<>
class FrameField<std::vector<uint8_t>> : public FieldBase {
    typedef uint32_t SizeType;
  public:
    size_t writeSize() const override {
        return sizeof(SizeType) + _value.size();
    }

    void write(uint8_t * output) override {
        *reinterpret_cast<SizeType*>(output) = SizeType(_value.size());
        memcpy(&((SizeType*)output)[1], &_value[0], _value.size());
    }

    void parse(const uint8_t * input) override {
        uint32_t size = *reinterpret_cast<const uint32_t*>(input);
        if (size > 1024 * 1024) {
            throw Exception EX("binary field is too long");
        }
        set(&input[sizeof(SizeType)], size);
    }

    const std::vector<uint8_t> & get() const {
        assert(isSet());
        return _value;
    }

    void set(const void * v, size_t size) {
        _set = true;
        auto ptr = (const uint8_t*)v;
        _value.assign(ptr, ptr + size);
    }

    template<typename V_>
    void set(const std::vector<V_> & v) {
        set(&v[0], v.size() * sizeof(V_));
    }

    template<typename V_>
    void set(std::vector<V_> && v) {
        _set = true;
        _value = move(v);
    }

    using FieldBase::FieldBase;
  private:
    std::vector<uint8_t> _value;
};

template<>
class FrameField<std::string> : public FieldBase {
    typedef uint32_t SizeType;
  public:
    size_t writeSize() const override {
        return sizeof(SizeType) + _value.size();
    }

    void write(uint8_t * output) override {
        *reinterpret_cast<SizeType*>(output) = SizeType(_value.size());
        memcpy(&((SizeType*)output)[1], &_value[0], _value.size());
    }

    void parse(const uint8_t * input) override {
        uint32_t size = *reinterpret_cast<const uint32_t*>(input);
        if (size > 1024 * 1024) {
            throw Exception EX("binary field is too long");
        }
        set(&input[sizeof(SizeType)], size);
    }

    const std::string & get() const {
        assert(isSet());
        return _value;
    }

    void set(const void * v, size_t size) {
        _set = true;
        auto ptr = (const char*)v;
        _value.assign(ptr, ptr + size);
    }

    void set(const std::string & v) {
        set(&v[0], v.size());
    }

    void set(std::string && v) {
        _set = true;
        _value = move(v);
    }

    using FieldBase::FieldBase;
  private:
    std::string _value;
};


using FrameFieldBinary = FrameField<std::vector<uint8_t>>;
using FrameFieldString = FrameField<std::string>;

class AlanFrame {
  private:
    friend class FieldBase;
    std::vector<FieldBase*> _fields;
  public:
    static const uint8_t SENT_TS   = 1;
    static const uint8_t DIFF_TS   = 2;
    static const uint8_t TIMESTAMP = 3;
    static const uint8_t DATA      = 4;
    static const uint8_t JSON      = 5;

    uint8_t version = 1;

    FrameField<int64_t> sentTs;
    FrameField<int64_t> diffTs;
    FrameField<int64_t> timestamp;
    FrameFieldBinary    data;
    FrameFieldString    json;

    std::vector<uint8_t> write();
    void parse(const std::vector<uint8_t> &);
    void parse(const void * data, size_t);

    AlanFrame();
};


}





#endif //ALANVOICE_ALANFRAME_H

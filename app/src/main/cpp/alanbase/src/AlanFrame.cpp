//
// Created by Andrey on 5/1/18.
//

#include "AlanFrame.h"
#include "Exception.h"
#include "Utils.h"

using namespace std;

namespace sky {

FieldBase::FieldBase(AlanFrame * alan, uint8_t type) : _type(type) {
    alan->_fields.push_back(this);
}

bool FieldBase::isSet() const {
    return _set;
}

uint8_t FieldBase::type() const {
    return _type;
}

AlanFrame::AlanFrame() : _fields(0),
    sentTs{this, SENT_TS},
    diffTs{this, DIFF_TS},
    timestamp{this, TIMESTAMP},
    data{this, DATA},
    json{this, JSON} {
}

vector<uint8_t> AlanFrame::write() {
    vector<uint8_t> buffer;
    size_t size = 1;
    for (auto f : _fields) {
        if (f->isSet()) {
            size += 1 + f->writeSize();
        }
    }
    buffer.resize(size);
    auto ptr = &buffer[0];
    *ptr++ = version;
    for (auto f : _fields) {
        if (f->isSet()) {
            *ptr++ = f->type();
            f->write(ptr);
            ptr+= f->writeSize();
        }
    }
    assert(buffer.size() == (ptr - &buffer[0]));
    return std::move(buffer);
}

void AlanFrame::parse(const std::vector<uint8_t> & buffer) {
    parse(&buffer[0], buffer.size());
}

void AlanFrame::parse(const void * buffer, size_t size) {
    auto ptr = (const uint8_t *)(buffer);
    auto end = ptr + size;
    version = *ptr++;
    while (ptr < end) {
        auto type = *ptr++;
        auto it = find_if(_fields.begin(), _fields.end(), [=](FieldBase * f) {
            return f->type() == type;
        });
        if (it == _fields.end()) {
            throw Exception EX(toStr("unsupported field type: ") + toStr(type));
        }
        (*it)->parse(ptr);
        ptr += (*it)->writeSize();
    }
}

}

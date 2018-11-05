//
//  Utils.cpp
//  skyBase
//
//  Created by Andrey Ryabov on 16/02/15.
//  Copyright (c) 2015 SkylineInc.Net. All rights reserved.
//

#include "Utils.h"
#include "Logger.h"
#include <cxxabi.h>
//#include <execinfo.h>
#include <random>
#include <iostream>
#include <boost/filesystem.hpp>

namespace sky {

using namespace std;
using namespace boost;


static string demangled(const string & sym) {
    string result(10 * 1024, ' ');
    auto size = result.size() - 1;
    int err = 0;
    abi::__cxa_demangle(sym.c_str(), &result[0], &size, &err);
    if (err) {
        return sym;
    }
    result.resize(size);
    return result;
}

string stackTrace() {
//    stringstream result;
//    void *array[10 * 1024];
//    int size = ::backtrace(array, 10 * 1024);
//    char ** symbols = backtrace_symbols(array, size);
//
//    for (int i = 1; i < size; i++ ) {
//        string line = symbols[i];
//        stringstream ss(line);
//        string tmp;
//        ss>>tmp>>tmp>>tmp>>tmp;
//        if (ss && tmp != "0x0") {
//            string dem = demangled(tmp);
//            size_t pos = line.find(tmp);
//            if (pos != string::npos) {
//                line.replace(pos, tmp.size(), dem);
//            }
//        }
//        result<<endl<<line;
//    }
//    free(symbols);
//    return result.str();
    return "No stacktrace available :(";
}

string fileFromPath(const string & path) {
    size_t idx = path.find_last_of("/\\");
    if (idx == string::npos) {
        return path;
    }
    return path.substr(idx + 1);
}

string timeNowStr() {
    using namespace std::chrono;

    auto now = system_clock::now();
    time_t tt = system_clock::to_time_t(now);

    tm * tmp = localtime(&tt);
    string res(128, ' ');
    size_t len = std::strftime(const_cast<char*>(res.c_str()),
        int(res.size()), "%Y-%m-%d %H:%M:%S.", tmp);
    res.resize(len);
    auto m = now.time_since_epoch() % milliseconds(1000);
    string msec = toStr(m.count() % 1000);
    while (msec.length() < 3) {
        msec = '0' + msec;
    }
    return res + msec;
}

//------------- byte array printing functions -----------------------------------------
string toHex(void * ptr) {
    stringstream ss;
    ss<<"0x"<<hex<< reinterpret_cast<uint64_t>(ptr);
    return ss.str();
}

string toHex(int64_t v) {
    stringstream ss;
    ss<<hex<<v;
    return ss.str();
}

string toHexStr(char byte) {
    return toHexStr((unsigned char) (byte));
}

string toHexStr(unsigned char byte) {
    stringstream s;
    s << hex << (unsigned int) (byte);
    string result = s.str();
    return (result.length() == 1) ? ("0" + result) : (result);
}

string toHexStr(const vector<char> & buf, size_t wrap) {
    return toHexStr(&buf[0], buf.size(), wrap);
}

string toHexStr(const void * ptr, size_t size, size_t wrap) {
    const unsigned char * src = static_cast<const unsigned char *> (ptr);
    stringstream s;
    for (size_t i = 0; i < size; i++) {
        if (i) {
            s<<" ";
            if (wrap && !(i % wrap)) {
                s<<endl;
            }
        }
        s<<"0x"<<toHexStr(src[i]);
    }
    return s.str();
}

int64_t epochMs() {
    using namespace std::chrono;
    auto tp = system_clock::now().time_since_epoch();
    return duration_cast<milliseconds>(tp).count();
}

int64_t nowMs() {
    return epochMs();
}

static const char _hex[]   = "abcdef0123456789";
static const char _chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static default_random_engine             _randomEngine((unsigned int)nowMs());
static uniform_int_distribution<int64_t> _uniform{0, numeric_limits<int64_t>::max()};
static normal_distribution<double>       _normal{0, 1};


double randNormal() {
    return _normal(_randomEngine);
}

int64_t randInt64() {
    return _uniform(_randomEngine);
}

int randInt(int from, int to) {
    return int(from + abs(_uniform(_randomEngine)) % (from - to));
}

string randStr(size_t size) {
    string res(size, '0');
    for (size_t i = 0; i < size; i++ ) {
        res[i] = _chars[randInt(0, sizeof(_chars) - 1)];
    }
    return res;
}

string randStrHex(size_t size) {
    string res(size, '0');
    for (size_t i = 0; i < size; i++ ) {
        res[i] = _hex[randInt(0, sizeof(_hex) - 1)];
    }
    return res;
}

BlobPtr makeBlob(const string & b) {
    return make_shared<const string>(b);
}

void toLower(string & s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
}

string toLower(const string & s) {
    auto res = s;
    toLower(res);
    return res;
}

bool makeDir(const string & name) {
    using namespace filesystem;
    path p(name);
    if (is_directory(p)) {
        return true;
    }
    system::error_code ec;
    return create_directories(p, ec);
}


//--------- class ThreadChecker ------------------------------------------------------------
ThreadChecker::ThreadChecker(const char * name) : _checker(name) {
}

static string getPThreadName(pthread_t pId) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
//    auto res = pthread_getname_np(pId, buffer, sizeof(buffer) - 1);
//    return res == 0 ? "" : buffer;
    return "No getname available";
}

void ThreadChecker::operator()() const {
    static_assert(sizeof(int64_t) >= sizeof(pthread_t), "size of int64_t must be greater then pthread_t");
    union {
        pthread_t p;
        int64_t   i;
    } id;
    memset(&id, 0, sizeof(id));
    id.p = pthread_self();
    int64_t xi = -1;
    if (_id.compare_exchange_weak(xi, id.i)) {
        _name = getPThreadName(id.p);
        Log<<"thread checker: "<<_checker<<",  initialized: "<<toHex(_id.load())<<", "<<_name<<eol;
    } else {
        if (_id.load() != id.i) {
            stringstream ss;
            ss<<" thread check failed: "<<_checker
              <<", "<<toHex(_id.load())<<"("<<_name<<") != "<<toHex(id.i)<<"("<<getPThreadName(id.p)<<")\n "
              <<stackTrace();

            string msg = ss.str();
            Err<<msg<<eol;
            cerr<<msg<<endl;
            assert(false);
        }
    }
}

int16_t floatToInt16(float v) {
    float maxS = 0.99;
    float s = v;
    if (s > maxS) {
        s = maxS;
    } else if (s < -maxS) {
        s = -maxS;
    }
    return int16_t((numeric_limits<int16_t>::max() - 3) * s);
}

void floatToInt16(const vector<float> & voice, vector<int16_t> & result) {
    floatToInt16(&voice[0], voice.size(), result);
}

void floatToInt16(const float * voice, size_t samples, vector<int16_t> & result) {
    result.resize(samples);
    floatToInt16(voice, samples, &result[0]);
}

void floatToInt16(const float * voice, size_t samples, int16_t * result) {
    for (int i = 0; i < samples; i++ ) {
        result[i] = floatToInt16(voice[i]);
    }
}

float int16ToFloat(int16_t v) {
    return float(v) / (numeric_limits<int16_t>::max() + 2);
}

void int16ToFloat(const vector<int16_t> & frame, vector<float> & result) {
    int16ToFloat(&frame[0], frame.size(), result);
}

void int16ToFloat(const int16_t * frame, size_t size, vector<float> & result) {
    result.resize(size);
    int16ToFloat(frame, size, &result[0]);
}

void int16ToFloat(const int16_t * frame, size_t size, float * result) {
    for (int i = 0; i < size; i++ ) {
        result[i] = int16ToFloat(frame[i]);
    }
}


string printFrame(const std::vector<float> & f) {
    return printFrame(&f[0], f.size());
}

string printFrame(const std::vector<int16_t> & f) {
    return printFrame(&f[0], f.size());
}

string printFrame(const float * f, size_t size) {
    stringstream ss;
    for (size_t i = 0; i < size; i++ ) {
        ss<<" "<<f[i];
    }
    return ss.str();
}

string printFrame(const int16_t * f, size_t size) {
    stringstream ss;
    for (size_t i = 0; i < size; i++ ) {
        ss<<" "<<f[i];
    }
    return ss.str();
}

}

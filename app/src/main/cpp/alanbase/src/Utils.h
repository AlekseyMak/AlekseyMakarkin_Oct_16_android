//
//  Utils.h
//  skyBase
//
//  Created by Andrey Ryabov on 16/02/15.
//  Copyright (c) 2015 SkylineInc.Net. All rights reserved.
//

#ifndef skyBase_Utils_h
#define skyBase_Utils_h

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <sstream>
#include <map>
#include <set>

#define _LOC_   ((sky::fileFromPath(__FILE__) + ":" + sky::toStr(__LINE__) + " [" + sky::toStr(__FUNCTION__) + "] - "))
#define EX(str) ((sky::fileFromPath(__FILE__) + ":" + sky::toStr(__LINE__) + " [" + sky::toStr(__PRETTY_FUNCTION__) + "] " + sky::toStr(str)) + sky::stackTrace())

#define CF_WHITE        "\033[1;37m"
#define CF_BLACK        "\033[0;30m"
#define CF_BLUE         "\033[0;34m"
#define CF_LIGHT_BLUE   "\033[1;34m"
#define CF_GREEN        "\033[0;32m"
#define CF_LIGHT_GREEN  "\033[1;32m"
#define CF_CYAN         "\033[0;36m"
#define CF_LIGHT_CYAN   "\033[1;36m"
#define CF_RED          "\033[0;31m"
#define CF_LIGHT_RED    "\033[1;31m"
#define CF_PURPLE       "\033[0;35m"
#define CF_LIGHT_PURPLE "\033[1;35m"
#define CF_BROWN        "\033[0;33m"
#define CF_YELLOW       "\033[1;33m"
#define CF_GRAY         "\033[0;30m"
#define CF_LIGHT_GRAY   "\033[0;37m"
#define C_DEF           "\033[0m"

namespace sky {

std::string timeNowStr();
std::string fileFromPath(const std::string &);
std::string stackTrace();

//--------------- Callbacks --------------------------------------------
template<typename... P_>
struct CallbackT {
    typedef typename std::function<void(P_...)> type;
};

template<>
struct CallbackT<void> {
    typedef typename std::function<void()> type;
};

template <typename... P_>
using Callback = typename CallbackT<P_...>::type;

//--------------- Pointer --------------------------------------------

template<typename T_>
using Ptr = std::shared_ptr<T_>;

template<typename T_>
using UPtr = std::unique_ptr<T_>;

template<typename T_>
using UPtrD = std::unique_ptr<T_, std::function<void(T_*)>>;

template<typename T_>
Ptr<T_> ptr(T_ * h) {
    return Ptr<T_>(h);
}

template<typename T_, typename D_>
Ptr<T_> ptr(T_ * h, D_ deleter) {
    return Ptr<T_>(h, deleter);
}

template<typename T_>
UPtr<T_> uptr(T_ * h) {
    return UPtr<T_>(h);
}

template<typename T_, typename D_>
UPtrD<T_> uptr(T_ * h, D_ d) {
    return UPtrD<T_>(h, d);
}

using BlobPtr = Ptr<const std::string>;

BlobPtr makeBlob(const std::string &);

//--------------- read<T> ---------------------------------------------
template<typename T_>
T_ read(const std::string & in) {
    std::stringstream ss(in);
    T_ v;
    ss>>v;
    return v;
}

template<typename T_>
T_ read(std::istream & in) {
    T_ v;
    in>>v;
    return v;
}

//--------------- toStr ---------------------------------------------
template<typename T>
std::string toStr(const T & t) {
    std::stringstream s;
    s<<t;
    return s.str();
}

inline std::string toStr(int8_t b) {
    return toStr(int(b));
}

inline std::string toStr(uint8_t b) {
    return toStr(int(b));
}

template<>
inline std::string toStr(const std::exception & e) {
    return toStr(e.what());
}

inline std::string toStr(char * msg) {
    return msg ? msg : "null";
}

inline std::string toStr(const char * msg) {
    return msg ? msg : "null";
}

template<typename T>
std::string toStr(const T * v) {
    if (!v) {
        return "null";
    }
    std::stringstream s;
    s<<std::hex<<(unsigned long)(v);
    return s.str();
}

template<typename T_, int N_>
std::string toStr(const T_ (&array)[N_]) {
    std::stringstream s;
    s<<N_<<"=[";
    bool coma = false;
    for (auto & v : array) {
        if (coma) {
            s<<", ";
        } else {
            coma = true;
        }
        s<<toStr(v);
    }
    s<<"]";
    return s.str();
}

template<typename T_>
std::string toStr(const std::vector<T_> & v) {
    std::stringstream s;
    s<<v.size()<<"=[";
    for (int i = 0; i < v.size(); i++ ) {
        if (i) {
            s<<", ";
        }
        s<<v[i];
    }
    s<<"]";
    return s.str();
}

template<typename K_, typename V_>
std::string toStr(const std::map<K_, V_> & m) {
    std::stringstream s;
    s<<m.size()<<"={";
    bool coma = false;
    for (auto & kv : m) {
        if (coma) {
            s<<", ";
        } else {
            coma = true;
        }
        s<<toStr(kv.first)<<": "<<toStr(kv.second);
    }
    s<<"}";
    return s.str();
}

void        toLower(std::string &);
std::string toLower(const std::string &);

typedef std::chrono::steady_clock::time_point Timestamp;

inline Timestamp now() {
    return std::chrono::steady_clock::now();
}

int64_t nowMs();
int64_t epochMs();


template<typename C_, typename T_>
std::set<T_> fields(const C_ & c, T_ const C_::value_type::* m) {
    std::set<T_> res;
    for (auto & r : c) {
        res.insert(r.*m);
    }
    return res;
}

template<typename C_>
std::vector<typename C_::value_type> toVec(const C_ & values) {
    std::vector<typename C_::value_type> res;
    res.reserve(values.size());
    for (auto & c : values) {
        res.push_back(c);
    }
    return res;
}


bool makeDir(const std::string & path);

//------------- byte array printing functions -----------------------------------------
std::string toHex(void *);
std::string toHex(int64_t);
std::string toHexStr(char byte);
std::string toHexStr(unsigned char byte);
std::string toHexStr(const void * ptr, size_t size, size_t wrap = 80);
std::string toHexStr(const std::vector<char> & buf, size_t wrap = 80);
std::string dumpStr (const void * ptr, size_t size, size_t limit = 0);


//------------- random ---------------------------------------------------------------
std::string randStr(size_t);
std::string randStrHex(size_t);
int64_t     randInt64();
int         randInt(int from, int to);
double      randNormal();


template<typename T_>
void removeDups(std::vector<T_> & v) {
    if (v.size() < 2) {
        return;
    }
    auto i = v.begin(), o = i, end = v.end();
    while (i < end) {
        if (*i == *o) {
            i++;
        } else {
            *++o = *i++;
        }
    }
    o++;
    if (o != end) {
        v.erase(o, end);
    }
}

template<typename C_, typename P_>
void removeIf(C_ & s, P_ && p) {
    if (s.empty()) {
        return;
    }
    auto it = std::remove_if(s.begin(), s.end(), p);
    if (it != s.end()) {
        s.erase(it);
    }
};

//------------- CountSet ---------------------------------------------------------------
template<typename E_>
class CountSet {
  public:
    using Vect = std::vector<std::pair<E_, int>>;
    using Iter = typename Vect::const_iterator;

    void add(const std::vector<E_> & values) {
        add(values.begin(), values.end());
    }

    template<typename It_>
    void add(It_ i0, It_ i1) {
        if (std::distance(i0, i1) > 1) {
            for (auto i = i0, j = i0 + 1; j < i1; i++, j++ ) {
                if (*i >= *j) {
                    throw std::invalid_argument(EX("input array must be sorted and unique"));
                }
            }
        }
        auto cmp = [](const std::pair<E_, int> & p1, const std::pair<E_, int> & p2) {
            return p1.first < p2.first;
        };
        auto size = _arr.size();
        for (auto i = i0; i < i1; i++) {
            auto middle = _arr.begin() + size;
            auto p  = std::make_pair(*i, 1);
            auto it = lower_bound(_arr.begin(), middle, p, cmp);
            if (it == middle || it->first != *i) {
                _arr.push_back(p);
            } else {
                it->second++;
            }
        }
        auto middle = _arr.begin() + size;
        if (middle != _arr.end()) {
            inplace_merge(_arr.begin(), middle, _arr.end(), cmp);
        }
    }

    Iter begin() const {
        return _arr.begin();
    }

    Iter end() const {
        return _arr.end();
    }

    friend std::ostream & operator << (std::ostream & o, const CountSet<E_> & cs) {
        o<<"size: "<<cs._arr.size();
        for (auto & p : cs._arr) {
            o<<" "<<p.first<<": "<<p.second;
        }
        return o;
    }
    
  private:
    Vect _arr;
};

//------------- CountSet ---------------------------------------------------------------
template<typename Key_, typename Value_>
class CountSetMax {
  public:
    struct Item {
        Key_    key;
        Value_  value;
        int     count;
    };

    using Vect = std::vector<Item>;
    using Iter = typename Vect::const_iterator;

    void add(Key_ k, Value_ v) {
        add(&k, &k + 1, v);
    }

    void add(const std::vector<Key_> & keys, Value_ v) {
        add(keys.begin(), keys.end(), v);
    }

    template<typename It_>
    void add(It_ i0, It_ i1, Value_ v) {
        if (std::distance(i0, i1) > 1) {
            for (auto i = i0, j = i0 + 1; j < i1; i++, j++ ) {
                if (*i >= *j) {
                    throw std::invalid_argument(EX("input array must be sorted and unique"));
                }
            }
        }
        auto cmp = [](const Item & p1, const Item & p2) { return p1.key < p2.key; };
        auto size = _arr.size();
        for (auto i = i0; i < i1; i++) {
            auto middle = _arr.begin() + size;
            Item p  = {*i, v, 1};
            auto it = lower_bound(_arr.begin(), middle, p, cmp);
            if (it == middle || it->key != *i) {
                _arr.push_back(p);
            } else {
                it->count++;
                it->value = std::max(it->value, v);
            }
        }
        auto middle = _arr.begin() + size;
        if (middle != _arr.end()) {
            inplace_merge(_arr.begin(), middle, _arr.end(), cmp);
        }
    }

    Iter begin() const {
        return _arr.begin();
    }

    Iter end() const {
        return _arr.end();
    }

    friend void swap(typename CountSetMax<Key_, Value_>::Item & i1, typename CountSetMax<Key_, Value_>::Item & i2) {
        std::swap(i1.key,   i2.key);
        std::swap(i1.value, i2.value);
        std::swap(i1.count, i2.count);
    }

    template<typename Cmp_>
    void sort(Cmp_ && cmp) {
        std::sort(_arr.begin(), _arr.end(), [&](const CountSetMax::Item & c1, const CountSetMax::Item & c2) {
            return cmp(c1.key, c1.value, c2.key, c2.value);
        });
    }

    template<typename Cond_>
    void remIf(Cond_ && cond) {
        auto it = remove_if(_arr.begin(), _arr.end(), [&](const CountSetMax::Item & c) {
            return cond(c.key, c.value);
        });
        if (it != _arr.end()) {
            _arr.erase(it, _arr.end());
        }
    }

    template<typename Cond_>
    int count(Cond_ && cond) {
        int result = 0;
        for (auto && c : _arr) {
            if (cond(c.key)) {
                result++;
            }
        }
        return result;
    }

    friend std::ostream & operator << (std::ostream & o, const CountSetMax<Key_, Value_> & cs) {
        o<<"size: "<<cs._arr.size();
        for (auto & p : cs._arr) {
            o<<" ("<<p.key<<", "<<p.value<<", "<<p.count<<")";
        }
        return o;
    }
    
  private:
    Vect _arr;
};


//------------- VectSet ---------------------------------------------------------------
template<typename V_>
class VectSet {
    typedef std::vector<V_> Vect;
    typedef typename Vect::const_iterator CIter;
  public:
    size_t size() const {
        return _values.size();
    }

    bool empty() const {
        return _values.empty();
    }

    bool has(V_ v) const {
        auto it = std::lower_bound(_values.begin(), _values.end(), v);
        return it != _values.end() && *it == v;
    }

    template<typename P_>
    bool containsIf(P_ && p) const {
        return std::find_if(_values.begin(), _values.end(), p) != _values.end();
    }

    bool add(V_ v) {
        if (has(v)) {
            return false;
        }
        auto s0 = _values.size();
        _values.push_back(v);
        std::inplace_merge(_values.begin(), _values.begin() + s0, _values.end());
        return true;
    }

    void add(const VectSet & v) {
        return add(v._values.begin(), v._values.end());
    }

    template<typename It_>
    bool add(It_ i0, It_ i1) {
        bool res = false;
        auto s0 = _values.size();
        while (i0 < i1) {
            auto v = *i0++;
            if (!has(v)) {
                res = true;
                _values.push_back(v);
            }
        }
        std::sort(_values.begin() + s0, _values.end());
        std::inplace_merge(_values.begin(), _values.begin() + s0, _values.end());
        return res;
    }

    void clear() {
        _values.clear();
    }

    template<typename P_>
    size_t removeIf(P_ && p) {
        auto it = std::remove_if(_values.begin(), _values.end(), p);
        size_t res = _values.end() - it;
        if (res > 0) {
            _values.erase(it, _values.end());
        }
        return res;
    }

    operator const std::vector<V_> & () const {
        return _values;
    }

    friend bool operator && (const VectSet & v1, const VectSet & v2) {
        return intersect(v1, v2);
    }

    friend bool intersect(const VectSet & v1, const VectSet & v2) {
        auto i1 = v1._values.begin(), e1 = v1._values.end();
        auto i2 = v2._values.begin(), e2 = v2._values.end();
        while (i1 < e1 && i2 < e2) {
            if (*i1 == *i2) {
                return true;
            }
            if (*i1 < *i2) {
                i1++;
            } else {
                i2++;
            }
        }
        return false;
    }

    CIter begin() const {
        return _values.begin();
    }

    CIter end() const {
        return _values.end();
    }

  private:
    std::vector<V_> _values;
};

//---------- Base64 ----------------------------------------------------------------
std::string base64encode(const std::string &);
std::string base64decode(const std::string &);


class ThreadChecker {
  public:
    void operator() () const;

    ThreadChecker(const char * c = "");
  private:
    mutable std::string          _checker;
    mutable std::atomic<int64_t> _id{-1};
    mutable std::string          _name;
};

int16_t floatToInt16(float);
void    floatToInt16(const std::vector<float> &,   std::vector<int16_t> & result);
void    floatToInt16(const float *, size_t,        std::vector<int16_t> & result);
void    floatToInt16(const float *, size_t,        int16_t * result);

float   int16ToFloat(int16_t);
void    int16ToFloat(const std::vector<int16_t> &, std::vector<float> & result);
void    int16ToFloat(const int16_t *, size_t,      std::vector<float> & result);
void    int16ToFloat(const int16_t *, size_t,      float * result);

std::string printFrame(const float *, size_t);
std::string printFrame(const int16_t *, size_t);
std::string printFrame(const std::vector<float> &);
std::string printFrame(const std::vector<int16_t> &);

}

#endif

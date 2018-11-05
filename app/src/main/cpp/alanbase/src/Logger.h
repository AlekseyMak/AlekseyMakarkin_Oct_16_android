//
//  Logger.h
//  skyBase
//
//  Created by Andrey Ryabov on 25/03/15.
//  Copyright (c) 2015 SkylineInc.Net. All rights reserved.
//

#ifndef skyBase_Logger_h
#define skyBase_Logger_h

#include <string>
#include <sstream>

namespace sky {

class Logger {
  public:
    enum class Level : char { Trace = 'T',  Info = 'I', Error = 'E'};

    class Entry {
      public:
        Level level;
        const char * file;
        const char * method;
        int line;

        Entry(Level ll, const char * fe, const char * md, int le)
            : level(ll), file(fe), method(md), line(le) {
        }
    };

    virtual bool isEnabled(const Entry &)=0;
    virtual void onMessage(const Entry &, const std::string &)=0;

    static bool check(const Entry &);
    static void set(Logger *);
    static void unset(Logger *);
    static Logger * get();
};

class LogMsg {
  public:
    class End {};

    template<typename T>
    LogMsg & operator << (const T & t) {
        _dirty = true;    
        _msg<<t;
        return * this;
    }

    void operator << (End) {
        _dirty = true;
        done();
    }

    operator bool () const;

    ~LogMsg();
    LogMsg(const Logger::Entry & e);

    LogMsg(LogMsg &&) = delete;
    LogMsg(const LogMsg &) = delete;

    LogMsg & operator = (LogMsg &&) = delete;
    LogMsg & operator = (const LogMsg &) = delete;
  private:
    void done();

    bool               _dirty{false};
    bool               _done {false};
    Logger::Entry      _entry;
    std::stringstream  _msg;
};

extern LogMsg::End eol;

}

#define Err LogMsg(Logger::Entry(Logger::Level::Error, __FILE__, __PRETTY_FUNCTION__, __LINE__))
#define Log LogMsg(Logger::Entry(Logger::Level::Info,  __FILE__, __PRETTY_FUNCTION__, __LINE__))
#define Trx LogMsg(Logger::Entry(Logger::Level::Trace, __FILE__, __PRETTY_FUNCTION__, __LINE__))

#endif


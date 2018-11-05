//
//  Logger.cpp
//  skyBase
//
//  Created by Andrey Ryabov on 25/03/15.
//  Copyright (c) 2015 SkylineInc.Net. All rights reserved.
//

#include "Logger.h"
#include <iostream>

using namespace std;

namespace sky {

static atomic<Logger*> _logger{nullptr};

LogMsg::End Eol;

bool Logger::check(const Entry & e) {
    Logger * log = get();
    return log && log->isEnabled(e);
}

void Logger::set(Logger * l) {
    _logger.store(l);
}

void Logger::unset(Logger * l) {
    _logger.compare_exchange_strong(l, nullptr);
}

Logger * Logger::get() {
    return _logger.load();
}

LogMsg::operator bool () const {
    return Logger::check(_entry);
}

void LogMsg::done() {
    if (!_dirty || _done) {
        return;
    }
    _done = true;
    Logger * l = Logger::get();
    try {
        if (l) {
            if (l->isEnabled(_entry)) {
                l->onMessage(_entry, _msg.str());
            }
        } else {
            cout<<_msg.str()<<endl;
        }
    } catch (const exception & e) {
        cerr<<"exception while logging"<<endl;
    }
}

LogMsg::~LogMsg() {
    done();
}

LogMsg::LogMsg(const Logger::Entry & e) : _entry(e) {
}

}


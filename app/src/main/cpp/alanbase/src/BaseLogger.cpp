//
// Created by Andrey Ryabov on 6/14/17.
//

#include "Utils.h"
#include "BaseLogger.h"
#include <iostream>
#include <iomanip>

using namespace std;

namespace sky {

BaseLogger::~BaseLogger() {
    Logger::unset(this);
    cout<<"~BaseLogger: "<<Logger::get()<<endl;
}

BaseLogger::BaseLogger() : _threadColors{
        CF_GRAY, CF_GREEN, CF_LIGHT_BLUE, CF_PURPLE
        CF_LIGHT_RED, CF_LIGHT_CYAN, CF_BLUE, CF_YELLOW,
        CF_LIGHT_PURPLE, CF_BROWN, CF_CYAN CF_LIGHT_GREEN} {
    _startTs = nowMs();
    Logger::set(this);
}

void BaseLogger::setLevel(string level) {
    toLower(level);
    trace = info = error = true;
    if (level == "error") {
        trace = info = false;
    } else if (level == "info") {
        trace = false;
    }
}

bool BaseLogger::isEnabled(const Entry & e) {
    if (e.level == Level::Trace && !trace) {
        return false;
    }
    if (e.level == Level::Info  && !info) {
        return false;
    }
    if (e.level == Level::Error && !error) {
        return false;
    }
    return true;
}

void BaseLogger::onMessage(const Entry & e, const std::string & m) {
    const char * level =
            e.level == Level::Info  ? "INFO "  :
            e.level == Level::Error ? "ERROR" :
            e.level == Level::Trace ? "TRACE" : "-";
    const char * lineColor =
            e.level == Level::Info  ? CF_BLACK :
            e.level == Level::Error ? CF_RED   :
            e.level == Level::Trace ? CF_LIGHT_GRAY : C_DEF ;

    stringstream line;
    auto now = nowMs() - _startTs;
    auto sec = now / 1000, msec = now % 1000;
    if (colors) {
        line<<lineColor;
    }
    line<<setw(6)<<sec<<"."<<setfill('0')<<setw(3)<<msec;
    line<<" ["<<level<<"] ";
    if (threads) {
        auto id = this_thread::get_id();
        if (colors) {
            auto size = _colorMap.size();
            auto & c  = _colorMap[id];
            if (!c) {
                c = _threadColors[size % _threadColors.size()];
            }
            line<<"["<<c<<id<<lineColor<<"] ";
        } else {
            line<<"["<<id<<"] ";
        }
    }
    line<<"["<<fileFromPath(e.file)<<":"<<e.line
        <<"] ["<<simpleMethod(e.method)
        <<"] - "<<m;
    if (colors) {
        line<<C_DEF;
    }
    if (handleMessage) {
        handleMessage(line.str());
    }
}

string BaseLogger::simpleMethod(const string & method) {
    auto pIx = method.find('(');
    if (pIx == string::npos) {
        return method;
    }
    auto res = method.substr(0, pIx);
    auto sIdx = res.find(' ');
    if (sIdx + 1 < res.size()) {
        res = res.substr(sIdx + 1);
    }
    return res;
}

}
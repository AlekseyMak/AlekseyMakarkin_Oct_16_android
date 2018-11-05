//
// Created by Andrey Ryabov on 6/14/17.
//

#ifndef SKYBASE_KERNELLOGGER_H_H
#define SKYBASE_KERNELLOGGER_H_H

#include "Logger.h"
#include <map>
#include <thread>
#include <iostream>
#include <functional>

namespace sky {

class BaseLogger : public Logger {
  public:
    volatile bool colors {true};
    volatile bool threads{true};
    volatile bool trace  {true};
    volatile bool info   {true};
    volatile bool error  {true};

    std::function<void(const std::string &)> handleMessage = [](const std::string & m) {
        std::cout<<m<<std::endl;
    };

    bool isEnabled(const Entry &) override;
    void onMessage(const Entry &, const std::string &m) override;
    void setLevel(std::string);

    ~BaseLogger();
    BaseLogger();
  private:
    std::string simpleMethod(const std::string &);

    std::map<std::thread::id, const char*> _colorMap;
    std::vector<const char*> _threadColors;
    int64_t      _startTs;
};

}

#endif //SKYBASE_KERNELLOGGER_H_H

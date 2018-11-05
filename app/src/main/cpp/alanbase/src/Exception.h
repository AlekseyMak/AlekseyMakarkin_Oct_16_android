//
//  Exception.h
//  skyBase
//
//  Created by Andrey Ryabov on 16/02/15.
//  Copyright (c) 2015 SkylineInc.Net. All rights reserved.
//

#ifndef skyBase_Exception_h
#define skyBase_Exception_h

#include <stdexcept>

namespace sky {

class Exception : public std::runtime_error {
  public:
    Exception(const std::string & message) : std::runtime_error(message) {}
};

}

#endif

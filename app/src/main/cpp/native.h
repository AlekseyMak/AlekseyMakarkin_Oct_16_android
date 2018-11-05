#ifndef ENERGY_CALC_H
#define ENERGY_CALC_H

#include <cstdint>
#include "jni.h"
#include "alanbase/src/AlanBase.h"
#include "alanbase/lib/json.hpp"

#define HEADER(type) extern "C" \
JNIEXPORT type JNICALL

#define LOGE(str) __android_log_print(ANDROID_LOG_ERROR, "NATIVE", "%s", str);

#endif //ENERGY_CALC_H

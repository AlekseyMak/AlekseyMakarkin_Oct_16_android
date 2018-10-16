#ifndef ENERGY_CALC_H
#define ENERGY_CALC_H

#include <cstdint>
#include "jni.h"

#define HEADER(type) extern "C" \
JNIEXPORT type JNICALL

#define LOGE(str) __android_log_print(ANDROID_LOG_ERROR, "NATIVE", "%s", str);
#define LOGE_DIGIT(dg) __android_log_print(ANDROID_LOG_ERROR, "NATIVE", "%d", dg);

#endif //ENERGY_CALC_H

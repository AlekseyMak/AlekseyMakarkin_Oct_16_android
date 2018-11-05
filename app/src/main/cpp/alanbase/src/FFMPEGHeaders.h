//
// Created by Andrey Ryabov on 6/20/17.
//

#ifndef SKYBASE_FFMPEGHEADERS_H
#define SKYBASE_FFMPEGHEADERS_H

extern "C" {
//#include "libavcodec/avfft.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/audio_fifo.h"
#include "libswresample/swresample.h"
}

#include <string>
#include <limits>
#define ffCall(call) {  \
    int ___err_X_ = call;     \
    if (___err_X_ < 0) {      \
        throw Exception EX("call failed: "#call" - " + ffErrorMsg(___err_X_)); \
    }\
}

namespace sky {

inline std::string ffErrorMsg(int errnum) {
    char buff[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, errnum);
    return buff;
}

inline int16_t toInt16(float s) {
    if (s >= 1) {
        return std::numeric_limits<int16_t>::max() - 2;
    }
    if (s <= -1) {
        return std::numeric_limits<int16_t>::min() + 2;
    }
    return int16_t(s * (std::numeric_limits<int16_t>::max() - 2));
}

inline float toFloat(int16_t s) {
    float r = float(s) / std::numeric_limits<int16_t>::max();
    if (r > 1) {
        return 1;
    }
    if (r < -1) {
        return -1;
    }
    return r;
}

}

#endif //SKYBASE_FFMPEGHEADERS_H

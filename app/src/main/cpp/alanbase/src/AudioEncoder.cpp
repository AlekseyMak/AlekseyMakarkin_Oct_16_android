//
// Created by Andrey on 7/12/18.
//

#include "AudioEncoder.h"
#include "FFMPEGHeaders.h"
#include "Exception.h"
#include "Logger.h"
#include "Utils.h"
#include <queue>

using namespace std;

namespace sky {

template<typename T_>
static function<void(T_)> fun(void (*f)(T_)) {
    return [=](T_ p) { f(p); };
}

template<typename T_>
static function<void(T_*)> fun2(void (*f)(T_**)) {
    return [=](T_* p) { if (p) f(&p); };
}

class AVPacketHolder {
  public:
    operator AVPacket *  () { return &_packet; }
    AVPacket * operator->() { return &_packet; }

    ~AVPacketHolder() { av_packet_unref(&_packet); }
     AVPacketHolder() { av_init_packet (&_packet);  }
  private:
    AVPacket _packet;
};


class FFMPEGAudioEncoder : public AudioEncoder {
  public:
    void encode(const float * data, size_t size) {
        _resamplerBuffer.resize(size * 32);
        const uint8_t * cIn = (const uint8_t *)data;
        uint8_t * cOut = &_resamplerBuffer[0];
        auto res = swr_convert(_resampler.get(), &cOut, _resamplerBuffer.size() / 4, &cIn, size);
        if (res < 0) {
            throw Exception EX("resampler failed");
        }
        void * sOut = cOut;
        ffCall(av_audio_fifo_write(_fifo.get(), &sOut, res));
        for (;;) {
            auto fifoSize = av_audio_fifo_size(_fifo.get());
            if (!fifoSize) {
                break;
            }
            auto samples = _encCtx->frame_size;
            if (!samples) {
                samples = std::min(1024 * 64, fifoSize);
            } else if (fifoSize < samples) {
                break;
            }
            //TODO: optimize this...
            auto af = uptr(av_frame_alloc(), fun2(av_frame_free));
            af->channel_layout = av_get_default_channel_layout(1);
            af->channels       = 1;
            af->nb_samples     = samples;
            af->format         = _encCtx->sample_fmt;
            af->sample_rate    = _encCtx->sample_rate;
            ffCall(av_frame_get_buffer(af.get(), 0));
            auto cnt = av_audio_fifo_read(_fifo.get(), (void**)af->data, samples);
            if (cnt < 0) {
                ffCall(cnt);
            }
            af->pts = av_rescale_q(_samples, {1, _encCtx->sample_rate}, _encCtx->time_base);
            _samples += cnt;
            ffCall(avcodec_send_frame(_encCtx.get(), af.get()));

            for (;;) {
                AVPacketHolder packet;
                av_init_packet(packet);
                int err = avcodec_receive_packet(_encCtx.get(), packet);
                if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
                    break;
                }
                ffCall(err);
                _packetQueue.push(vector<uint8_t>(packet->data, packet->data + packet->size));
                _packets++;
            }
        }
    }

    bool getPacket(vector<uint8_t> & frame) {
        if (_packetQueue.empty()) {
            return false;
        }
        frame = move(_packetQueue.front());
        _packetQueue.pop();
        return true;
    }

    FFMPEGAudioEncoder(string name, int inSr, int outSr) : AudioEncoder(name, inSr, outSr) {
        auto codec = avcodec_find_encoder_by_name(_codec.c_str());
        if (!codec) {
            throw Exception EX("codec not found: " + _codec);
        }
        _encCtx = uptr(avcodec_alloc_context3(codec), fun2(&avcodec_free_context));
        if (!_encCtx) {
            throw Exception EX("failed to allocate codec context");
        }
        _encCtx->channels       = 1;
        _encCtx->channel_layout = av_get_default_channel_layout(1);
        _encCtx->sample_rate    = _outSampleRate;
        _encCtx->sample_fmt     = codec->sample_fmts[0];
        _encCtx->bit_rate       = _bitRate;
        _encCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
        ffCall(avcodec_open2(_encCtx.get(), codec, nullptr));

        _resampler = uptr(swr_alloc_set_opts(nullptr,
                                             AV_CH_LAYOUT_MONO, _encCtx->sample_fmt, _outSampleRate,
                                             AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_FLT, _inSampleRate,
                                             0, nullptr), fun2(swr_free));
        auto err = swr_init(_resampler.get());
        if (err < 0) {
            throw Exception EX("resampler configure failed: " + toStr(err));
        }
        _fifo = uptr(av_audio_fifo_alloc(_encCtx->sample_fmt, 1, 1024), fun(av_audio_fifo_free));
        if (!_fifo) {
            throw Exception EX("failed to allocate fifo");
        }
    }
  private:
    int                              _bitRate{128000};
    int64_t                          _samples{0};
    int64_t                          _packets{0};
    UPtrD<SwrContext>                _resampler;
    vector<uint8_t>                  _resamplerBuffer;
    UPtrD<AVCodecContext>            _encCtx;
    UPtrD<AVAudioFifo>               _fifo;
    std::queue<std::vector<uint8_t>> _packetQueue;
};

unique_ptr<AudioEncoder> AudioEncoder::create(string codec, int inputSampleRate, int outSampleRate) {
    return unique_ptr<AudioEncoder>(new FFMPEGAudioEncoder(codec, inputSampleRate, outSampleRate));
}

AudioEncoder::AudioEncoder(string codec, int inSr, int outSr) :
    _codec(codec), _inSampleRate(inSr), _outSampleRate(outSr) {
}

const string & AudioEncoder::codec() const {
    return _codec;
}

int AudioEncoder::inputSampleRate() const {
    return _inSampleRate;
}

int AudioEncoder::outputSampleRate() const {
    return _outSampleRate;
}

void AudioEncoder::encode(const int16_t * buffer, size_t size) {
    int16ToFloat(buffer, size, _floatBuffer);
    encode(&_floatBuffer[0], size);
}

}

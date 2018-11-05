//
// Created by Andrey on 5/16/18.
// Last update Jul 18 2018
//

#include "AudioDecoder.h"
#include "Exception.h"
#include "FFMPEGHeaders.h"
#include "Logger.h"

namespace sky {

using namespace std;

template<typename T_>
static function<void(T_*)> fun2(void (*f)(T_**)) {
    return [=](T_* p) { f(&p); };
}

AudioDecoder::AudioDecoder(string c, int i, int o) : _codec(c), _inputSampleRate(i), _outputSampleRate(o) {
    write(1024 * 8);
}

const string & AudioDecoder::codec() const {
    return _codec;
}

int AudioDecoder::inputSampleRate () const {
    return _inputSampleRate;
}

int AudioDecoder::outputSampleRate() const {
    return _outputSampleRate;
}

class FFPEGAudioDecoder : public AudioDecoder {
  public:
    void decode(const void * data, size_t size) override {
        if (!size) {
            return;
        }
        _avPacket.size = int(size);
        _avPacket.data = (uint8_t*)data;
        int ret = avcodec_send_packet(_decCtx.get(), &_avPacket);
        if (ret < 0) {
            Err<<"failed to decode packet: "<<size<<" - "<<ffErrorMsg(ret)<<eol;
            return;
        }
        for (;;) {
            auto _avFrame = uptr(av_frame_alloc(), fun2(av_frame_free));
            int err = avcodec_receive_frame(_decCtx.get(), _avFrame.get());
            if (err == AVERROR(EAGAIN)) {
                return;
            } else if (err) {
                Err<<"failed to decode frame: "<<ffErrorMsg(err)<<eol;
                return;
            }
            _resamplerBuffer.resize(_avFrame->nb_samples * 8);
            uint8_t * cOut = (uint8_t*)&_resamplerBuffer[0];
            const uint8_t * cInp = _avFrame->data[0];
            auto samples = swr_convert(_resampler.get(), &cOut, _resamplerBuffer.size(), &cInp, _avFrame->nb_samples);
            if (samples < 0) {
                Err<<"resampler failed: "<<samples<<eol;
                return;
            }
            auto w = write(samples);
            copy(_resamplerBuffer.begin(), _resamplerBuffer.begin() + samples, w);
            commit(samples);
        }
    }

    FFPEGAudioDecoder(string codec, int isr, int osr) : AudioDecoder(codec, isr, osr) {
        auto cdc = avcodec_find_decoder_by_name(_codec.c_str());
        if (!cdc) {
            throw Exception EX("codec not found: " + _codec);
        }
        _decCtx = uptr(avcodec_alloc_context3(cdc), fun2(avcodec_free_context));
        if (!_decCtx) {
            throw Exception EX("failed to create decoder");
        }
        _decCtx->request_sample_fmt = AV_SAMPLE_FMT_FLT;
        _decCtx->sample_rate = _inputSampleRate;
        _decCtx->channels = 1;
        ffCall(avcodec_open2(_decCtx.get(), cdc, nullptr));
        av_init_packet(&_avPacket);
        _resampler = uptr(swr_alloc_set_opts(nullptr,
                                             AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_FLT, _outputSampleRate,
                                             AV_CH_LAYOUT_MONO, _decCtx->sample_fmt, _decCtx->sample_rate,
                                             0, nullptr), fun2(swr_free));
        auto err = swr_init(_resampler.get());
        if (err < 0) {
            throw Exception EX("resampler configure failed: " + toStr(err));
        }

    }
  private:
    UPtrD<AVCodecContext> _decCtx;
    UPtrD<SwrContext>     _resampler;
    AVPacket              _avPacket;
    vector<float>         _resamplerBuffer;

};

UPtr<AudioDecoder> AudioDecoder::create(const string & codec, int isr, int osr) {
    return uptr(new FFPEGAudioDecoder(codec, isr, osr));
}

};

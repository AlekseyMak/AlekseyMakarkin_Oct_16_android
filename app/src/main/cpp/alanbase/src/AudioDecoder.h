//
// Created by Andrey on 5/16/18.
// Last update Jul 18 2018
//

#ifndef ALANVOICE_AUDIODECODER_H
#define ALANVOICE_AUDIODECODER_H

#include "Utils.h"
#include "Logger.h"
#include "Buffer.h"
#include "Exception.h"

namespace sky {

class AudioDecoder : public Buffer<float> {
  public:
    virtual void decode(const void * data, size_t) = 0;

    const std::string & codec() const;
    int      inputSampleRate () const;
    int      outputSampleRate() const;

    static std::unique_ptr<AudioDecoder> create(const std::string & name, int inSampleRate, int outSampleRate);

    virtual ~AudioDecoder() = default;
    AudioDecoder(std::string codec, int inSampleRate, int outSampleRate);

    AudioDecoder(const AudioDecoder &) = delete;
    AudioDecoder & operator = (const AudioDecoder &) = delete;
    AudioDecoder(AudioDecoder &&) = delete;
    AudioDecoder & operator = (AudioDecoder &&) = delete;
  protected:
    const std::string _codec;
    const int         _inputSampleRate;
    const int         _outputSampleRate;
};

}

#endif //ALANVOICE_AUDIODECODER_H

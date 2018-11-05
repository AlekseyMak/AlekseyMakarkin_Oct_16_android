//
// Created by Andrey on 7/12/18.
//

#ifndef PROJECT_AUDIOENCODER_H
#define PROJECT_AUDIOENCODER_H

#include <memory>
#include <string>
#include <vector>

namespace sky {

class AudioEncoder {
  public:
    void         encode(const int16_t*, size_t);
    virtual void encode(const float  *, size_t)=0;
    virtual bool getPacket(std::vector<uint8_t>&)=0;

    const std::string & codec () const;
    int       inputSampleRate () const;
    int       outputSampleRate() const;

    static std::unique_ptr<AudioEncoder> create(std::string, int inputSampleRate, int outSampleRate);

    virtual ~AudioEncoder() = default;
  protected:
    AudioEncoder(std::string, int inSr, int outSr);

    AudioEncoder(const AudioEncoder &) = delete;
    AudioEncoder& operator = (const AudioEncoder &) = delete;
    AudioEncoder(AudioEncoder &&) = delete;
    AudioEncoder& operator = (AudioEncoder &&) = delete;

    std::string _codec;
    int         _inSampleRate;
    int         _outSampleRate;
  private:
    std::vector<float> _floatBuffer;
};


}

#endif //PROJECT_AUDIOENCODER_H

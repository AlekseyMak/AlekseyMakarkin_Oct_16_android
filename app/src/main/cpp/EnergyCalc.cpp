#include <string.h>
#include <jni.h>
#include <android/log.h>
#include "EnergyCalc.h"
#include "audiofile/AudioFile.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <math.h>
#include <iterator>
#include <vector>

class AudioManager
{
public:

private:

};

#define GROUND_NOISE_LEVEL 5000.0

HEADER(jint)
Java_com_amakarkin_alanchallenge_domain_Recorder_prepareFile(
        JNIEnv* env,
        jobject /* this */,
        jstring path) {
    return 5;
}

bool writeDataToFile (std::vector<uint16_t>& fileData, std::string filePath)
{
    std::ofstream outputFile (filePath, std::ios::binary);

    if (outputFile.is_open())
    {
        for (int i = 0; i < fileData.size(); i++)
        {
            uint16_t value = fileData[i];
            outputFile.write (reinterpret_cast<const char*>(&value), sizeof (uint16_t));
        }

        outputFile.close();

        return true;
    }

    return false;
}

HEADER(jint)
Java_com_amakarkin_alanchallenge_domain_Recorder_processFrame(JNIEnv* env,
                                                              jobject /* this */,
                                                              jshortArray frame)
{
    jsize len = env->GetArrayLength(frame);
    jshort *body = env->GetShortArrayElements(frame, 0);

    LOGE("Got samples")
    LOGE_DIGIT(len)

    std::vector<uint16_t> samples;

    unsigned long accumulator = 0;

    for (int i=0; i<len; i++)
    {
        uint16_t sample = (uint16_t) body[i];
        accumulator += sample * sample;

        samples.push_back(sample);
    }

    double energy = 20 * log10(accumulator / len / GROUND_NOISE_LEVEL);
    return (int)fmin(energy, 100);
}

jstring fromCharArray(JNIEnv* env, const char* str)
{
    jclass strClass = env->FindClass("java/lang/String");
    jmethodID ctorID = env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
    jstring encoding = env->NewStringUTF("UTF-8");

    jbyteArray bytes = env->NewByteArray(strlen(str));
    env->SetByteArrayRegion(bytes, 0, strlen(str), (jbyte*)str);
    jstring res = (jstring)env->NewObject(strClass, ctorID, bytes, encoding);
    return res;
}

char* appendCharToCharArray(const char* array, char a)
{
    size_t len = strlen(array);

    char* ret = new char[len+2];

    strcpy(ret, array);
    ret[len] = a;
    ret[len+1] = '\0';

    return ret;
}

void writeSample() {
    AudioFile<double> audioFile;
    AudioFile<double>::AudioBuffer buffer;

// 2. Set to (e.g.) two channels
    buffer.resize (2);

// 3. Set number of samples per channel
    buffer[0].resize (100000);
    buffer[1].resize (100000);

// 4. do something here to fill the buffer with samples

// 5. Put into the AudioFile object
//    bool ok = audioFile.setAudioBuffer (buffer);

    int numChannels = 2;
    int numSamplesPerChannel = 100000;
    float sampleRate = 44100.f;
    float frequency = 440;

    for (int i = 0; i < numSamplesPerChannel; i++)
    {
        float sample = sinf (2. * M_PI * ((float) i / sampleRate) * frequency) ;

        for (int channel = 0; channel < numChannels; channel++)
            buffer[channel][i] = sample * 0.5;
    }

    audioFile.setAudioBuffer(buffer);
    bool ok = audioFile.save("/data/data/com.amakarkin.audiorecorder/files/mydir/test.wav", AudioFileFormat::Wave);
    if (ok)
    {
        LOGE("OK")
    } else {
        LOGE("NOT OK")
    }
}

HEADER(jstring)
Java_com_amakarkin_audiorecorder_Recorder_readNative(
        JNIEnv* env,
        jobject,
        jstring path) {

    jboolean isCopy;
    const char *convertedValue = (env)->GetStringUTFChars(path, &isCopy);
    LOGE(convertedValue)
    char* res = appendCharToCharArray(convertedValue, 'X');
    LOGE(res)

    std::string line;
    std::ifstream myfile (convertedValue);
    if (myfile.is_open())
    {
        while ( getline (myfile,line) )
        {
            LOGE(line.c_str())
        }
        myfile.close();
    }

    else LOGE("Unable to open file")

    writeSample();

    return fromCharArray(env, res);
}
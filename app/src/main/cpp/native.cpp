#include <string.h>
#include <jni.h>
#include <android/log.h>
#include "native.h"
#include "AudioManager.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <math.h>

AudioManager manager = AudioManager();

#define GROUND_NOISE_LEVEL 4000.0

int computeEnergyLevel(AudioManager::sample_t *frame, size_t len, bool needWrite)
{
    unsigned long accumulator = 0;
    for (int i=0; i<len; i++)
    {
        AudioManager::sample_t sample = (AudioManager::sample_t) frame[i];
        accumulator += sample * sample;

        if (needWrite) {
            manager.writeData(sample);
        }
    }

    double energy = 20 * log10(accumulator / len / GROUND_NOISE_LEVEL);
    return (int)fmin(energy, 100);
}

HEADER(jboolean)
Java_com_amakarkin_alanchallenge_domain_Recorder_prepareNative(JNIEnv* env,
                                                               jobject /* this */,
                                                               jstring path)
{
    jboolean isCopy;
    const char *nativePath = (env)->GetStringUTFChars(path, &isCopy);

    if (manager.prepareOutputFile(nativePath))
    {
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}

HEADER(void)
Java_com_amakarkin_alanchallenge_domain_Recorder_stopRecordingNative(JNIEnv* env,
                                                                     jobject /* this */)
{
    manager.closeOutputFile();
}

HEADER(jint)
Java_com_amakarkin_alanchallenge_domain_Recorder_computeEnergyLevel(JNIEnv* env,
                                                              jobject /* this */,
                                                              jshortArray frame)
{
    jsize len = env->GetArrayLength(frame);
    jshort *body = env->GetShortArrayElements(frame, 0);

    return computeEnergyLevel(body, len, false);
}

HEADER(jint)
Java_com_amakarkin_alanchallenge_domain_Recorder_processFrame(JNIEnv* env,
                                                              jobject /* this */,
                                                              jshortArray frame)
{
    jsize len = env->GetArrayLength(frame);
    jshort *body = env->GetShortArrayElements(frame, 0);

    return computeEnergyLevel(body, len, true);
}

HEADER(jboolean)
Java_com_amakarkin_alanchallenge_domain_Recorder_prepareInputNative(JNIEnv* env,
                                                            jobject /* this */,
                                                            jstring path)
{
    jboolean isCopy;
    const char *nativePath = (env)->GetStringUTFChars(path, &isCopy);
    if (manager.prepareInputFile(nativePath)) {
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}

HEADER(jshortArray)
Java_com_amakarkin_alanchallenge_domain_Recorder_readNative(JNIEnv* env,
                                                              jobject /* this */)
{
    std::vector<AudioManager::sample_t> buffer = manager.read();
    jshort *jBuffer = &buffer[0];
    jshortArray outJNIArray = (*env).NewShortArray(buffer.size());
    if (NULL == outJNIArray) return NULL;
    (*env).SetShortArrayRegion(outJNIArray, 0 , buffer.size(), jBuffer);
    return outJNIArray;
}


HEADER(void)
Java_com_amakarkin_alanchallenge_domain_Recorder_closeInputNative(JNIEnv* env,
                                                            jobject /* this */)
{
    manager.stop();
}
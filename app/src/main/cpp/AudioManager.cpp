#include <android/log.h>
#include "AudioManager.h"
#include "EnergyCalc.h"

bool AudioManager::prepareOutputFile(std::string path)
{
    outputFile = std::ofstream(path, std::ios::binary);
    return outputFile.is_open();
}

void AudioManager::closeOutputFile()
{
        if (outputFile.is_open())
        {
            outputFile.close();
        }
};

void AudioManager::writeData(AudioManager::sample_t data)
{
    outputFile.write (reinterpret_cast<const char*>(&data), sizeof (data));
}

bool AudioManager::prepareInputFile(std::string path)
{
    inputFile = std::ifstream(path, std::ios::binary);
    inputFile.seekg(0, std::ios::beg);
    return inputFile.is_open();
}

std::vector<AudioManager::sample_t> AudioManager::read() {
    std::vector<AudioManager::sample_t > vec;
    vec.resize(BUFFER_SIZE  * sizeof(AudioManager::sample_t));
    inputFile.read((char*)vec.data(), BUFFER_SIZE * sizeof(AudioManager::sample_t));
    return vec;
}

void AudioManager::stop() {
    if (inputFile.is_open())
    {
        inputFile.close();
    }
}
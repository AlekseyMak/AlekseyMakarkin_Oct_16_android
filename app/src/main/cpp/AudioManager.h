#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <math.h>
#include <string>
#include <vector>

class AudioManager
{
public:

    typedef int16_t sample_t;

    bool prepareOutputFile(std::string path);
    void closeOutputFile();
    void writeData(AudioManager::sample_t data);

    bool prepareInputFile(std::string path);
    std::vector<AudioManager::sample_t> read();
    void stop();

private:
    const size_t BUFFER_SIZE = 1344;

    std::ofstream outputFile;
    std::ifstream inputFile;
};

#endif //AUDIO_MANAGER_H

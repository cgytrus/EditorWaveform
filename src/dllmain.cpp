#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <matdash.hpp>
#include <matdash/minhook.hpp>
#include <matdash/boilerplate.hpp>
#include <matdash/console.hpp>
#include <gd.h>
#include "extensions2.h"
#include <fmod.hpp>

#include "config.hpp"

#undef min
#undef max

#ifdef CUSTOM_DEBUG
#include <iostream>
#include <fstream>
#endif

using namespace cocos2d;

float xPosForTime(CCArray* speedObjects, gd::Speed speed, float time) {
    float x;
    __asm movss xmm0, time;
    reinterpret_cast<void(__fastcall*)(CCArray*, gd::Speed)>(gd::base + 0x18acd0)(speedObjects, speed);
    __asm movss x, xmm0;
    return x;
}
float timeForXPos(CCArray* speedObjects, gd::Speed speed, float x) {
    float time;
    __asm movss xmm0, x;
    reinterpret_cast<void(__fastcall*)(CCArray*, gd::Speed)>(gd::base + 0x18ae70)(speedObjects, speed);
    __asm movss time, xmm0;
    return time;
}

struct sample_t {
    float value;
    float x;
};
static std::vector<sample_t> samples;
static float sampleRate = 0.f;
static float position = 0.f;

matdash::cc::thiscall<bool> LevelEditorLayer_init(gd::LevelEditorLayer* self, gd::GJGameLevel* level) {
    std::string songPath = CCFileUtils::sharedFileUtils()->fullPathForFilename(level->getAudioFileName().c_str(), false);
    std::cout << "what if you wanted to edit " << level->m_sLevelName << std::endl
              << "but god said " << songPath << std::endl;

    position = 0.f;

    auto engine = gd::FMODAudioEngine::sharedEngine();

    FMOD::Sound* sound;
    const FMOD_MODE mode = FMOD_DEFAULT | FMOD_CREATESAMPLE | FMOD_OPENONLY;
    engine->m_pSystem->createSound(songPath.c_str(), mode, nullptr, &sound);

    FMOD_SOUND_FORMAT format;
    int channels;
    sound->getFormat(nullptr, &format, &channels, nullptr);

    if(format != FMOD_SOUND_FORMAT_PCM8 && format != FMOD_SOUND_FORMAT_PCM16 && format != FMOD_SOUND_FORMAT_PCM24 &&
        format != FMOD_SOUND_FORMAT_PCM32 && format != FMOD_SOUND_FORMAT_PCMFLOAT) {
        std::cout << "unsupported format" << std::endl;
        sound->release();
        sound = nullptr;
        samples.clear();
        return matdash::orig<&LevelEditorLayer_init>(self, level);
    }

    unsigned int length;
    unsigned int sampleCount;
    sound->getLength(&length, FMOD_TIMEUNIT_PCMBYTES);
    sound->getLength(&sampleCount, FMOD_TIMEUNIT_PCM);
    unsigned int bytesPerSample = length / sampleCount;
    int8_t* data = new int8_t[length];
    sound->readData(data, length, &length);

    FMOD::ChannelGroup* channelGroup;
    engine->m_pGlobalChannel->getChannelGroup(&channelGroup);
    engine->m_pSystem->playSound(sound, channelGroup, true, &engine->m_pGlobalChannel);
    engine->m_pGlobalChannel->getFrequency(&sampleRate);

    sound->release();
    sound = nullptr;

    sampleCount = length / bytesPerSample;
    samples.clear();
    samples.reserve(sampleCount);
    size_t dataIndex = 0;
    for(size_t i = 0; i < sampleCount; i++) {
        float sample = 0.f;
        for(int j = 0; j < channels; j++) {
            int32_t rawChannelSample;
            switch(format) {
                case FMOD_SOUND_FORMAT_PCM8:
                    rawChannelSample = data[dataIndex++];
                    sample += rawChannelSample / 255.f;
                    break;
                case FMOD_SOUND_FORMAT_PCM16:
                    rawChannelSample = (data[dataIndex++] << 0 |
                                        data[dataIndex++] << 8);
                    sample += rawChannelSample / (255.f * 255.f);
                    break;
                case FMOD_SOUND_FORMAT_PCM24:
                    rawChannelSample = (data[dataIndex++] << 0 |
                                        data[dataIndex++] << 8 |
                                        data[dataIndex++] << 16);
                    sample += rawChannelSample / (255.f * 255.f * 255.f);
                    break;
                case FMOD_SOUND_FORMAT_PCM32:
                    rawChannelSample = (data[dataIndex++] << 0  |
                                        data[dataIndex++] << 8  |
                                        data[dataIndex++] << 16 |
                                        data[dataIndex++] << 24);
                    sample += rawChannelSample / (255.f * 255.f * 255.f * 255.f);
                    break;
                case FMOD_SOUND_FORMAT_PCMFLOAT:
                    rawChannelSample = (data[dataIndex++] << 0  |
                                        data[dataIndex++] << 8  |
                                        data[dataIndex++] << 16 |
                                        data[dataIndex++] << 24);
                    sample += *reinterpret_cast<float*>(&rawChannelSample);
                    break;
            }
        }
        sample /= channels;
        samples.push_back({sample, 0.f});
    }
    delete[] data;

    return matdash::orig<&LevelEditorLayer_init>(self, level);
}

matdash::cc::thiscall<void> DrawGridLayer_loadTimeMarkers(gd::DrawGridLayer* self, std::string guidelines) {
    matdash::orig<&DrawGridLayer_loadTimeMarkers>(self, guidelines);

    auto speeds = self->m_pSpeedObjects2;
    auto startSpeed = self->m_pEditor->m_pLevelSettings->m_speed;
    auto songOffset = self->m_pEditor->m_pLevelSettings->m_songOffset;

    for(size_t i = 0; i < samples.size(); i++)
        samples[i].x = xPosForTime(speeds, startSpeed, i / sampleRate - songOffset);

    return {};
}

static std::vector<CCPoint> lines;
matdash::cc::thiscall<void> DrawGridLayer_draw(gd::DrawGridLayer* self) {
    matdash::orig<&DrawGridLayer_draw>(self);

    float lineWidth = static_cast<float>(waveform::config::lineWidth);
    float verticalScale = waveform::config::verticalScale;
    float offset = waveform::config::verticalOffset;

    auto director = CCDirector::sharedDirector();
    float localWinWidth = director->getWinSize().width / self->getParent()->getScaleX();
    float winWidthPixels = director->getOpenGLView()->getFrameSize().width;
    float unitsToPixels = winWidthPixels / localWinWidth;

    CCPoint pos = self->convertToNodeSpace(CCPoint{0.f, 0.f});
    float leftBound = std::floor(std::max(pos.x, 0.f) * unitsToPixels / lineWidth) * lineWidth / unitsToPixels;
    float rightBound = std::ceil((leftBound + localWinWidth) * unitsToPixels / lineWidth + 3.f) * lineWidth / unitsToPixels;

    if(waveform::config::lockToCamera)
        position = pos.y - offset + offset / self->getParent()->getScaleY();
    float drawPos = position + offset;

    auto speeds = self->m_pSpeedObjects2;
    auto startSpeed = self->m_pEditor->m_pLevelSettings->m_speed;
    auto songOffset = self->m_pEditor->m_pLevelSettings->m_songOffset;

    float startTime = timeForXPos(speeds, startSpeed, leftBound) + songOffset;
    float endTime = timeForXPos(speeds, startSpeed, rightBound) + songOffset;

    size_t startSample = std::clamp(static_cast<size_t>(startTime * sampleRate), 0u, samples.size());
    size_t endSample = std::clamp(static_cast<size_t>(endTime * sampleRate), 0u, samples.size());

    float sample = 0.f;
    int lastPixels = 0;
    float lastX = 0.f;
    lines.clear();
    for(size_t i = startSample; i < endSample; i++) {
        sample_t currentSample = samples[i];
        int pixels = static_cast<int>(std::floor(currentSample.x * unitsToPixels / lineWidth));
        if(std::abs(currentSample.value) > std::abs(sample))
            sample = currentSample.value;
        if(pixels == lastPixels)
            continue;

        lines.push_back(CCPoint{lastX, -sample * verticalScale + drawPos});
        lines.push_back(CCPoint{lastX, sample * verticalScale + drawPos});

        sample = 0.f;
        lastPixels = pixels;
        lastX = std::floor(currentSample.x * unitsToPixels) / unitsToPixels;
    }

    ccDrawColor4B(31, 31, 31, waveform::config::opacity);
    float prevLineWidth;
    glGetFloatv(GL_LINE_WIDTH, &prevLineWidth);
    glLineWidth(lineWidth);
    ccDrawLines(lines.data(), lines.size());
    glLineWidth(prevLineWidth);

    return {};
}

void waveform::addHooks() {
    matdash::add_hook<&LevelEditorLayer_init>(gd::base + 0x15ee00);
    matdash::add_hook<&DrawGridLayer_loadTimeMarkers>(gd::base + 0x16cab0);
    matdash::add_hook<&DrawGridLayer_draw>(gd::base + 0x16ce90);
}

void mod_main(HMODULE hModule) {
#ifdef CUSTOM_DEBUG
    auto console = new matdash::Console();
#endif

    waveform::config::loadConfig();
    waveform::config::initMegahack();
    if(waveform::config::enabled)
        waveform::config::enableHooks();

#ifdef CUSTOM_DEBUG
    std::string input;
    std::getline(std::cin, input);

    MH_Uninitialize();
    delete console;
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
#endif
}

#pragma once

#include <filesystem>
#include <fstream>
#include "extensions2.h"

namespace waveform {
    void addHooks();
}

namespace waveform::config {
    static bool enabled = true;
    static bool lockToCamera = true;
    static float verticalOffset = 270.f;
    static float verticalScale = 90.f;
    static int lineWidth = 4;
    static uint8_t opacity = 190;

    static MegaHackExt::Spinner* mhVerticalOffset;
    static MegaHackExt::Spinner* mhVerticalScale;
    static MegaHackExt::Spinner* mhLineWidth;
    static MegaHackExt::Spinner* mhOpacity;

    std::filesystem::path getConfigPath() {
        auto configDir = std::filesystem::path(cocos2d::CCFileUtils::sharedFileUtils()->getWritablePath2()) / "config";
        std::filesystem::create_directories(configDir);
        return configDir / "EditorWaveform.txt";
    }
    void loadConfig() {
        auto configPath = getConfigPath();
        if(!std::filesystem::exists(configPath))
            return;
        std::ifstream config(configPath);
        config >> enabled >> lockToCamera >> verticalOffset >> verticalScale >> lineWidth >> opacity;
        config.close();
    }
    void saveConfig() {
        std::ofstream config(getConfigPath());
        config << enabled << ' ' << lockToCamera << ' ' << verticalOffset << ' ' << verticalScale << ' ' << lineWidth << ' ' << opacity;
        config.close();
    }

    static bool _wasEverEnabled = false;
    void enableHooks() {
        if(_wasEverEnabled) {
            MH_EnableHook(MH_ALL_HOOKS);
            return;
        }
        _wasEverEnabled = true;
        addHooks();
    }
    void disableHooks() {
        if(_wasEverEnabled)
            MH_DisableHook(MH_ALL_HOOKS);
    }

    void initMegahack() {
        if(!GetModuleHandle(TEXT("hackpro.dll")))
            return;

        auto window = MegaHackExt::Window::Create("Editor Waveform");

        mhOpacity = MegaHackExt::Spinner::Create("Opacity: ", "");
        mhOpacity->set(opacity);
        mhOpacity->setCallback([](MegaHackExt::Spinner* obj, double value) {
            value = std::clamp(std::floor(value), 0.0, 255.0);
            mhOpacity->set(value, false);
            opacity = static_cast<uint8_t>(value);
            saveConfig();
        });
        window->add(mhOpacity);

        mhLineWidth = MegaHackExt::Spinner::Create("Line width: ", "px");
        mhLineWidth->set(lineWidth);
        mhLineWidth->setCallback([](MegaHackExt::Spinner* obj, double value) {
            value = std::clamp(std::floor(value), 1.0, 10.0);
            mhLineWidth->set(value, false);
            lineWidth = static_cast<int>(value);
            saveConfig();
        });
        window->add(mhLineWidth);

        mhVerticalScale = MegaHackExt::Spinner::Create("Vertical scale: ", "x");
        mhVerticalScale->set(verticalScale);
        mhVerticalScale->setCallback([](MegaHackExt::Spinner* obj, double value) {
            value = std::floor(value * 10.0) / 10.0;
            mhVerticalScale->set(value, false);
            verticalScale = static_cast<float>(value);
            saveConfig();
        });
        window->add(mhVerticalScale);

        mhVerticalOffset = MegaHackExt::Spinner::Create("Vertical offset: ", " units");
        mhVerticalOffset->set(verticalOffset);
        mhVerticalOffset->setCallback([](MegaHackExt::Spinner* obj, double value) {
            value = std::floor(value * 10.0) / 10.0;
            mhVerticalOffset->set(value, false);
            verticalOffset = static_cast<float>(value);
            saveConfig();
        });
        window->add(mhVerticalOffset);

        auto checkbox = MegaHackExt::CheckBox::Create("Lock to camera");
        checkbox->set(lockToCamera);
        checkbox->setCallback([](MegaHackExt::CheckBox* obj, bool value) {
            lockToCamera = value;
            saveConfig();
        });
        window->add(checkbox);

        checkbox = MegaHackExt::CheckBox::Create("Enabled");
        checkbox->set(enabled);
        checkbox->setCallback([](MegaHackExt::CheckBox* obj, bool value) {
            enabled = value;
            if(value)
                enableHooks();
            else
                disableHooks();
            saveConfig();
        });
        window->add(checkbox);

        MegaHackExt::Client::commit(window);
    }
}

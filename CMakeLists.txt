cmake_minimum_required(VERSION 3.22)

project(AudioMixer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(third_party/JUCE)

juce_add_gui_app(AudioMixer
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "AudioMixer"
    PRODUCT_NAME "AudioMixer"
    BUNDLE_ID "com.audiomixer.app")

target_sources(AudioMixer
    PRIVATE
        src/Main.cpp
        src/MainComponent.cpp
        src/MainComponent.h
        src/AppSettings.cpp
        src/AppSettings.h
        src/SettingsStore.cpp
        src/SettingsStore.h
        src/AppLogger.cpp
        src/AppLogger.h
        src/AudioCaptureEngine.cpp
        src/AudioCaptureEngine.h
        src/LevelMeter.cpp
        src/LevelMeter.h)

target_link_libraries(AudioMixer
    PRIVATE
        juce::juce_gui_extra
        juce::juce_data_structures
        juce::juce_audio_devices)

target_compile_definitions(AudioMixer
    PRIVATE
        AUDIO_MIXER_VERSION="${PROJECT_VERSION}")

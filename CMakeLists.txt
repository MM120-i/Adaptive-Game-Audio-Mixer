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
        src/ui/MainComponent.cpp
        src/ui/MainComponent.h
        src/ui/LevelMeter.cpp
        src/ui/LevelMeter.h
        src/ui/MixerLookAndFeel.h
        src/audio/AudioCaptureEngine.cpp
        src/audio/AudioCaptureEngine.h
        src/core/AppSettings.cpp
        src/core/AppSettings.h
        src/core/SettingsStore.cpp
        src/core/SettingsStore.h
        src/core/AppLogger.cpp
        src/core/AppLogger.h)

target_include_directories(AudioMixer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(AudioMixer
    PRIVATE
        juce::juce_gui_extra
        juce::juce_data_structures
        juce::juce_audio_devices)

target_compile_definitions(AudioMixer
    PRIVATE
        AUDIO_MIXER_VERSION="${PROJECT_VERSION}")

cmake_minimum_required(VERSION 3.22)

project(AudioMixer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/MP)
endif()

option(ENABLE_ANALYZE "Enable MSVC /analyze static code analysis" OFF)

if(MSVC AND ENABLE_ANALYZE)
    add_compile_options(/analyze)
endif()

file(STRINGS "${CMAKE_SOURCE_DIR}/.env" ENV_LINES)

foreach(LINE ${ENV_LINES})
    if(LINE MATCHES "^SPOTIFY_CLIENT_ID=(.*)")
        set(SPOTIFY_CLIENT_ID "${CMAKE_MATCH_1}")
    endif()
endforeach()

configure_file(spotify_config.h.in spotify_config.h @ONLY)

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
        src/core/AppLogger.h
        src/ui/VolumeControl.cpp
        src/ui/VolumeControl.h
        src/core/SpotifyClient.cpp
        src/core/SpotifyClient.h
)

target_include_directories(AudioMixer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR})

target_link_libraries(AudioMixer
    PRIVATE
        juce::juce_gui_extra
        juce::juce_data_structures
        juce::juce_cryptography
        ole32)

target_compile_definitions(AudioMixer
    PRIVATE
        AUDIO_MIXER_VERSION="${PROJECT_VERSION}")

add_custom_command(TARGET AudioMixer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/src/core/auth"
        "$<TARGET_FILE_DIR:AudioMixer>/auth"
    COMMENT "Copying auth pages to output directory"
)

# ── Tests ──────────────────────────────────────────────────────
enable_testing()

add_executable(AudioMixerTests)

target_sources(AudioMixerTests
    PRIVATE
        tests/MainTest.cpp
        tests/LevelMeterTest.cpp
        tests/AppSettingsTest.cpp
        tests/VolumeControlTest.cpp
        tests/SpotifyClientTest.cpp
        src/ui/VolumeControl.cpp
        src/ui/VolumeControl.h
        src/ui/LevelMeter.cpp
        src/ui/LevelMeter.h
        src/core/AppSettings.cpp
        src/core/AppSettings.h
        src/core/SettingsStore.cpp
        src/core/SettingsStore.h
        src/core/AppLogger.cpp
        src/core/AppLogger.h
        src/core/SpotifyClient.cpp
        src/core/SpotifyClient.h
)

target_include_directories(AudioMixerTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR})

target_link_libraries(AudioMixerTests
    PRIVATE
        juce::juce_gui_extra
        juce::juce_data_structures
        juce::juce_cryptography)

add_test(NAME AudioMixerTests COMMAND AudioMixerTests)

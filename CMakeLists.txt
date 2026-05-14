cmake_minimum_required(VERSION 3.22)

project(VOXLINE VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)
include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version" FORCE)
endif()

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(juce)

set(VOXLINE_PLUGIN_FORMATS VST3)

if(APPLE)
    list(APPEND VOXLINE_PLUGIN_FORMATS AU)
endif()

juce_add_plugin(VOXLINE
    COMPANY_NAME "ONETAKE"
    BUNDLE_ID "com.onetake.voxline"
    PLUGIN_MANUFACTURER_CODE Otnk
    PLUGIN_CODE Vxln
    FORMATS ${VOXLINE_PLUGIN_FORMATS}
    PRODUCT_NAME "VOXLINE"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

target_sources(VOXLINE
    PRIVATE
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
)

juce_generate_juce_header(VOXLINE)

target_compile_definitions(VOXLINE
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(VOXLINE
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(BUILD_TESTING)
    juce_add_console_app(VOXLINEPhase1Tests PRODUCT_NAME "VOXLINEPhase1Tests")

    target_sources(VOXLINEPhase1Tests
        PRIVATE
            Source/PluginEditor.cpp
            Source/PluginEditor.h
            Source/PluginProcessor.cpp
            Source/PluginProcessor.h
            Tests/Phase1Tests.cpp
    )

    juce_generate_juce_header(VOXLINEPhase1Tests)

    target_compile_definitions(VOXLINEPhase1Tests
        PRIVATE
            JucePlugin_Build_VST=0
            JucePlugin_Build_VST3=0
            JucePlugin_Build_AU=0
            JucePlugin_Build_AUv3=0
            JucePlugin_Build_AAX=0
            JucePlugin_Build_Standalone=0
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
    )

    target_link_libraries(VOXLINEPhase1Tests
        PRIVATE
            juce::juce_audio_utils
            juce::juce_audio_processors
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )

    add_test(NAME VOXLINEPhase1Tests COMMAND VOXLINEPhase1Tests)
endif()

cmake_minimum_required(VERSION 3.22)

project(RotaryDemo VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(juce)

juce_add_gui_app(RotaryDemo
    PRODUCT_NAME "RotaryDemo"
    COMPANY_NAME "ONETAKE"
)

target_sources(RotaryDemo
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/VoxLookAndFeel.cpp
)

target_compile_definitions(RotaryDemo
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:RotaryDemo,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:RotaryDemo,JUCE_VERSION>"
)

target_link_libraries(RotaryDemo
    PRIVATE
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

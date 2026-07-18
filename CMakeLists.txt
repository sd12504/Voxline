cmake_minimum_required(VERSION 3.22)

project(VOXLINE VERSION 2.1.0 LANGUAGES C CXX)

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

file(GLOB_RECURSE VOXLINE_SHARED_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)

file(GLOB_RECURSE VOXLINE_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.h"
)

target_sources(VOXLINE
    PRIVATE
        ${VOXLINE_SHARED_SOURCES}
)

juce_add_binary_data(VOXLINEAssets
    SOURCES
        # SVG icons
        Source/UI/Assets/svg/voxline_logo_mark_light.svg
        Source/UI/Assets/svg/voxline_logo_mark_dark.svg
        Source/UI/Assets/svg/settings_light.svg
        Source/UI/Assets/svg/settings_dark.svg
        Source/UI/Assets/svg/bypass_light.svg
        Source/UI/Assets/svg/bypass_dark.svg
        Source/UI/Assets/svg/listen_light.svg
        Source/UI/Assets/svg/listen_dark.svg
        Source/UI/Assets/svg/subtle_noise_texture_light.svg
        Source/UI/Assets/svg/subtle_noise_texture_dark.svg
        Source/UI/Assets/svg/soft_radial_glow_light.svg
        Source/UI/Assets/svg/soft_radial_glow_dark.svg
        Source/UI/Assets/svg/moon.svg
        Source/UI/Assets/svg/sun.svg
        Source/UI/Assets/svg/ab_dark.svg
        Source/UI/Assets/svg/ab_light.svg
        Source/UI/Assets/svg/auto_gain_dark.svg
        Source/UI/Assets/svg/auto_gain_light.svg
        Source/UI/Assets/svg/chevron_down_dark.svg
        Source/UI/Assets/svg/chevron_down_light.svg
        Source/UI/Assets/svg/clean_sparkle_dark.svg
        Source/UI/Assets/svg/clean_sparkle_light.svg
        # Background PNGs
        Source/UI/Assets/backgrounds/background_dark.png
        Source/UI/Assets/backgrounds/background_light.png
        # Button PNGs — theme-aware (dark/light)
        Source/UI/Assets/buttons/bypass_normal_dark.png
        Source/UI/Assets/buttons/bypass_normal_light.png
        Source/UI/Assets/buttons/bypass_active_dark.png
        Source/UI/Assets/buttons/bypass_active_light.png
        Source/UI/Assets/buttons/listen_normal_dark.png
        Source/UI/Assets/buttons/listen_normal_light.png
        Source/UI/Assets/buttons/listen_active_dark.png
        Source/UI/Assets/buttons/listen_active_light.png
        Source/UI/Assets/buttons/on_normal_dark.png
        Source/UI/Assets/buttons/on_normal_light.png
        Source/UI/Assets/buttons/on_active_dark.png
        Source/UI/Assets/buttons/on_active_light.png
        # Button PNGs — single-theme
        Source/UI/Assets/buttons/hpf_normal.png
        Source/UI/Assets/buttons/hpf_active.png
        Source/UI/Assets/buttons/low_normal.png
        Source/UI/Assets/buttons/low_active.png
        Source/UI/Assets/buttons/mud_normal.png
        Source/UI/Assets/buttons/mud_active.png
        Source/UI/Assets/buttons/pres_normal.png
        Source/UI/Assets/buttons/pres_active.png
        Source/UI/Assets/buttons/air_normal.png
        Source/UI/Assets/buttons/air_active.png
        Source/UI/Assets/buttons/lpf_normal.png
        Source/UI/Assets/buttons/lpf_active.png
        # Knob face PNGs
        Source/UI/Assets/buttons/knob_face_large_dark@2x.png
        Source/UI/Assets/buttons/knob_face_large_light@2x.png
        Source/UI/Assets/buttons/knob_face_medium_dark@2x.png
        Source/UI/Assets/buttons/knob_face_medium_light@2x.png
        Source/UI/Assets/buttons/knob_face_small_dark@2x.png
        Source/UI/Assets/buttons/knob_face_small_light@2x.png
        # Knob inside sprite strips (64 frames)
        Source/UI/Assets/buttons/knob_face_large_inside_dark@2x.png
        Source/UI/Assets/buttons/knob_face_large_inside_light@2x.png
        Source/UI/Assets/buttons/knob_face_medium_inside_dark@2x.png
        Source/UI/Assets/buttons/knob_face_medium_inside_light@2x.png
        Source/UI/Assets/buttons/knob_face_small_inside_dark@2x.png
        Source/UI/Assets/buttons/knob_face_small_inside_light@2x.png
        # V2 knob PNGs
        Source/UI/Assets/png/knob_hero_polish_dark.png
        Source/UI/Assets/png/knob_hero_polish_light.png
        Source/UI/Assets/png/knob_medium_body_dark.png
        Source/UI/Assets/png/knob_medium_body_light.png
        Source/UI/Assets/png/knob_medium_clarity_dark.png
        Source/UI/Assets/png/knob_medium_clarity_light.png
        Source/UI/Assets/png/knob_small_output_dark.png
        Source/UI/Assets/png/knob_small_output_light.png
        # Layout JSON
        Source/UI/layout.json
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
        juce::juce_dsp
        VOXLINEAssets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(BUILD_TESTING)
    juce_add_console_app(VOXLINEPhase1Tests PRODUCT_NAME "VOXLINEPhase1Tests")

    target_sources(VOXLINEPhase1Tests
        PRIVATE
            ${VOXLINE_SHARED_SOURCES}
            ${VOXLINE_TEST_SOURCES}
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
            juce::juce_dsp
            VOXLINEAssets
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )

    add_test(NAME VOXLINEPhase1Tests COMMAND VOXLINEPhase1Tests)
endif()

# ===========================================================================
# CPack — cross-platform release packaging
# ===========================================================================
set(CPACK_PACKAGE_NAME "VOXLINE")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "ONETAKE")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Vocal channel strip audio plugin")
set(CPACK_PACKAGE_FILE_NAME "VOXLINE-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_COMPONENTS_ALL Plugins)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "VOXLINE")
    set(CPACK_DMG_FORMAT "UDZO")

    install(DIRECTORY ${CMAKE_BINARY_DIR}/VOXLINE_artefacts/Release/VST3/
            DESTINATION "VST3"
            COMPONENT Plugins)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/VOXLINE_artefacts/Release/AU/
            DESTINATION "Components"
            COMPONENT Plugins)
elseif(WIN32)
    set(CPACK_GENERATOR "ZIP")
    install(DIRECTORY ${CMAKE_BINARY_DIR}/VOXLINE_artefacts/Release/VST3/
            DESTINATION "VST3"
            COMPONENT Plugins)
endif()

include(CPack)

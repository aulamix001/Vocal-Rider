cmake_minimum_required(VERSION 3.22)
# CMP0091 must be NEW before enabling the MSVC languages, including JUCE helpers.
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    CACHE STRING "Static CRT: /MT Release, /MTd Debug, for every target" FORCE)
project(VocalBalance VERSION 0.2.1 LANGUAGES C CXX)
if(WIN32)
  if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Windows releases require MSVC x64; use Visual Studio 2022 -A x64")
  endif()
  string(TOUPPER "${MSVC_CXX_ARCHITECTURE_ID}" VB_ARCH)
  if(NOT VB_ARCH STREQUAL "X64")
    message(FATAL_ERROR "Expected x64 compiler, got ${MSVC_CXX_ARCHITECTURE_ID}")
  endif()
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(VB_BUILD_PLUGIN "Build Vocal Balance JUCE plugins" ON)
option(VB_BUILD_VST2_COMPAT "Build the independent VST2 compatibility entry point" ON)
include(CTest)
add_executable(dsp_tests tests/DspTests.cpp)
target_include_directories(dsp_tests PRIVATE Source)
add_test(NAME dsp COMMAND dsp_tests)
if(NOT VB_BUILD_PLUGIN)
  return()
endif()
if(DEFINED JUCE_SOURCE_DIR)
  add_subdirectory("${JUCE_SOURCE_DIR}" JUCE)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 29396c22c93392d6738e021b83196283d6e4d850)
  FetchContent_MakeAvailable(JUCE)
endif()
# VB20 is a new identity for the rebuild. Display name remains Vocal Balance.
juce_add_plugin(VocalBalance
  COMPANY_NAME "Independent Audio"
  BUNDLE_ID "audio.independent.vocalbalance2"
  PLUGIN_MANUFACTURER_CODE InAu
  PLUGIN_CODE VB20
  PRODUCT_NAME "VocalBalance"
  PLUGIN_NAME "Vocal Balance"
  DESCRIPTION "Automatic vocal gain rider"
  FORMATS VST3 Standalone
  VST3_CATEGORIES Fx Dynamics
  IS_SYNTH FALSE
  IS_MIDI_EFFECT FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(VocalBalance PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/UI/HardwareLookAndFeel.cpp)
target_compile_definitions(VocalBalance PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(VocalBalance PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
get_target_property(VB_MODULES juce::juce_audio_processors INTERFACE_JUCE_MODULE_PATH)
set(VB_SDK "${VB_MODULES}/juce_audio_processors_headless/format_types/VST3_SDK")
if(VB_BUILD_VST2_COMPAT)
  add_library(VocalBalance_VST2 MODULE Source/LegacyVst2.cpp)
  target_include_directories(VocalBalance_VST2 PRIVATE Source third_party "${VB_MODULES}")
  target_compile_definitions(VocalBalance_VST2 PRIVATE $<TARGET_PROPERTY:VocalBalance,COMPILE_DEFINITIONS>)
  target_link_libraries(VocalBalance_VST2 PRIVATE VocalBalance)
  if(WIN32 AND TARGET VocalBalance_rc_lib)
    target_link_libraries(VocalBalance_VST2 PRIVATE VocalBalance_rc_lib)
  endif()
  set_target_properties(VocalBalance_VST2 PROPERTIES PREFIX "" OUTPUT_NAME "VocalBalance"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/VocalBalance_artefacts/$<CONFIG>/VST2"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/VocalBalance_artefacts/$<CONFIG>/VST2")
endif()
juce_add_console_app(plugin_tests PRODUCT_NAME "VocalBalanceProcessorTests")
target_sources(plugin_tests PRIVATE tests/PluginTests.cpp)
target_include_directories(plugin_tests PRIVATE Source)
target_link_libraries(plugin_tests PRIVATE VocalBalance juce::juce_audio_utils)
add_test(NAME processor COMMAND plugin_tests "${CMAKE_BINARY_DIR}/VocalBalance-editor.png")
add_executable(vst3_smoke tests/Vst3Smoke.cpp "${VB_SDK}/pluginterfaces/base/coreiids.cpp" "${VB_SDK}/pluginterfaces/base/funknown.cpp" "${VB_SDK}/public.sdk/source/vst/vstinitiids.cpp")
target_include_directories(vst3_smoke PRIVATE "${VB_SDK}")
target_link_libraries(vst3_smoke PRIVATE ${CMAKE_DL_LIBS})
add_dependencies(vst3_smoke VocalBalance_VST3)
add_test(NAME vst3_module COMMAND vst3_smoke "$<TARGET_FILE:VocalBalance_VST3>")
if(TARGET VocalBalance_VST2)
  add_executable(vst2_smoke tests/Vst2Smoke.cpp)
  target_include_directories(vst2_smoke PRIVATE third_party)
  target_link_libraries(vst2_smoke PRIVATE ${CMAKE_DL_LIBS})
  if(WIN32)
    target_link_libraries(vst2_smoke PRIVATE user32)
  else()
    find_package(X11 REQUIRED)
    target_include_directories(vst2_smoke PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(vst2_smoke PRIVATE ${X11_LIBRARIES})
    target_link_options(vst2_smoke PRIVATE -Wl,--export-dynamic)
  endif()
  add_dependencies(vst2_smoke VocalBalance_VST2)
  add_test(NAME vst2_module COMMAND vst2_smoke "$<TARGET_FILE:VocalBalance_VST2>")
endif()
set_tests_properties(dsp processor vst3_module PROPERTIES TIMEOUT 45)
if(TARGET vst2_smoke)
  set_tests_properties(vst2_module PROPERTIES TIMEOUT 45)
endif()
# Test helper only: never distributed as part of the audio plugin.
if(WIN32 AND BUILD_TESTING)
  add_library(reaper_vocalbalance_discovery MODULE tests/ReaperDiscovery.cpp)
  target_include_directories(reaper_vocalbalance_discovery PRIVATE third_party)
  set_target_properties(reaper_vocalbalance_discovery PROPERTIES PREFIX "")
endif()

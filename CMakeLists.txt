# Opciones de macOS: deben ir ANTES de project()
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Binario universal: Apple Silicon + Intel")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS minimo")

cmake_minimum_required(VERSION 3.22)
project(SemeParaEQ VERSION 5.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
  set(PLUGIN_FORMATS VST3 AU)      # FL Studio para Mac abre VST3 y AU
else()
  set(PLUGIN_FORMATS VST3)
endif()

juce_add_plugin(SemeParaEQ
  COMPANY_NAME "Ivanjoko"
  BUNDLE_ID com.ivanjoko.semeparaeq
  PLUGIN_MANUFACTURER_CODE Ivjk
  PLUGIN_CODE Spq1
  FORMATS ${PLUGIN_FORMATS}
  PRODUCT_NAME "SemeParaEQ"
  VST3_CATEGORIES Fx EQ
  AU_MAIN_TYPE kAudioUnitType_Effect
  COPY_PLUGIN_AFTER_BUILD TRUE
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE)

target_sources(SemeParaEQ PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp
  Source/CyberUI.cpp)

target_compile_features(SemeParaEQ PRIVATE cxx_std_17)
target_compile_definitions(SemeParaEQ PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SemeParaEQ PRIVATE
  juce::juce_audio_utils
  juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# Optimización: velocidad máxima en la versión final
if(NOT MSVC)
  target_compile_options(SemeParaEQ PRIVATE $<$<CONFIG:Release>:-O3>)
endif()

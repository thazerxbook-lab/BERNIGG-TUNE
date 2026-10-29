cmake_minimum_required(VERSION 3.22)
project(BERNIGG_TUNE VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(BERNIGG_TUNE
    COMPANY_NAME "BERNIGG"
    BUNDLE_ID "com.bernigg.tune"
    PLUGIN_MANUFACTURER_CODE Brng
    PLUGIN_CODE BnT1
    FORMATS VST3 Standalone
    PRODUCT_NAME "BERNIGG TUNE"
    DESCRIPTION "Real-time vocal pitch correction prototype")

juce_generate_juce_header(BERNIGG_TUNE)
target_sources(BERNIGG_TUNE PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)
target_compile_features(BERNIGG_TUNE PRIVATE cxx_std_17)
target_compile_definitions(BERNIGG_TUNE PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BERNIGG_TUNE PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

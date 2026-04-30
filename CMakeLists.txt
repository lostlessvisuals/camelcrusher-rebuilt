cmake_minimum_required(VERSION 3.25)

include(cmake/CamelCrusherMetadata.cmake)

project(CamelCrusherRecalled VERSION ${CAMELCRUSHER_RELEASE_VERSION} LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CAMELCRUSHER_RELEASE_VERSION_STRING "${CAMELCRUSHER_RELEASE_VERSION}")
set(CAMELCRUSHER_RELEASE_BUILD_INT "${CAMELCRUSHER_RELEASE_BUILD}")
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
set(CAMELCRUSHER_AU_CODESIGN_IDENTITY "-"
    CACHE STRING "Code signing identity for local AU bundles; use '-' for ad hoc signing.")
set(CAMELCRUSHER_VST2_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vst2sdk"
    CACHE PATH "Local VST2 SDK root for the VST2 compatibility build.")
option(CAMELCRUSHER_ENABLE_VST2_COMPATIBILITY
    "Build the same-identity CamelCrusher VST2 compatibility target."
    ON)

add_library(camelcrusher_core
    src/core/DefaultLegacyFactoryBank.cpp
    src/core/LegacyChunk.cpp
    src/core/LegacyImport.cpp
    src/core/LegacyParameters.cpp
    src/core/LegacyPresetBank.cpp
    src/core/LegacySchema.cpp
    src/core/LegacyState.cpp
    src/core/ModernPluginModel.cpp
    src/core/ModernProcessor.cpp
    src/core/ModernRuntime.cpp
)

target_include_directories(camelcrusher_core
    PUBLIC
        include
)

target_compile_features(camelcrusher_core PUBLIC cxx_std_20)
set_target_properties(camelcrusher_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(camelcrusher_runtime_bridge SHARED
    src/core/ModernRuntimeBridge.cpp
)

target_include_directories(camelcrusher_runtime_bridge
    PUBLIC
        include
)

target_link_libraries(camelcrusher_runtime_bridge
    PRIVATE
        camelcrusher_core
)

target_compile_definitions(camelcrusher_runtime_bridge
    PRIVATE
        CAMELCRUSHER_RUNTIME_BRIDGE_BUILD
)

add_executable(legacy_schema_smoke
    tests/legacy_schema_smoke.cpp
)

target_link_libraries(legacy_schema_smoke
    PRIVATE
        camelcrusher_core
)

enable_testing()

add_test(NAME legacy_schema_smoke COMMAND legacy_schema_smoke)

add_executable(legacy_chunk_smoke
    tests/legacy_chunk_smoke.cpp
)

target_link_libraries(legacy_chunk_smoke
    PRIVATE
        camelcrusher_core
)

target_compile_definitions(legacy_chunk_smoke
    PRIVATE
        CAMELCRUSHER_RECALLED_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME legacy_chunk_smoke COMMAND legacy_chunk_smoke)

add_executable(legacy_model_smoke
    tests/legacy_model_smoke.cpp
)

target_link_libraries(legacy_model_smoke
    PRIVATE
        camelcrusher_core
)

add_test(NAME legacy_model_smoke COMMAND legacy_model_smoke)

add_executable(modern_plugin_model_smoke
    tests/modern_plugin_model_smoke.cpp
)

target_link_libraries(modern_plugin_model_smoke
    PRIVATE
        camelcrusher_core
)

add_test(NAME modern_plugin_model_smoke COMMAND modern_plugin_model_smoke)

add_executable(modern_processor_smoke
    tests/modern_processor_smoke.cpp
)

target_link_libraries(modern_processor_smoke
    PRIVATE
        camelcrusher_core
)

add_test(NAME modern_processor_smoke COMMAND modern_processor_smoke)

add_executable(modern_runtime_smoke
    tests/modern_runtime_smoke.cpp
)

target_link_libraries(modern_runtime_smoke
    PRIVATE
        camelcrusher_core
)

add_test(NAME modern_runtime_smoke COMMAND modern_runtime_smoke)

add_executable(modern_runtime_bridge_smoke
    tests/modern_runtime_bridge_smoke.cpp
)

target_link_libraries(modern_runtime_bridge_smoke
    PRIVATE
        camelcrusher_core
        camelcrusher_runtime_bridge
)

add_test(NAME modern_runtime_bridge_smoke COMMAND modern_runtime_bridge_smoke)

if(CAMELCRUSHER_ENABLE_VST2_COMPATIBILITY)
    set(CAMELCRUSHER_VST2_SOURCE_ROOT "${CAMELCRUSHER_VST2_SDK_ROOT}")
    if(EXISTS "${CAMELCRUSHER_VST2_SDK_ROOT}/VST2_SDK/public.sdk/source/vst2.x/audioeffect.h")
        set(CAMELCRUSHER_VST2_SOURCE_ROOT "${CAMELCRUSHER_VST2_SDK_ROOT}/VST2_SDK")
    endif()

    set(CAMELCRUSHER_VST2_AUDIOEFFECT_H
        "${CAMELCRUSHER_VST2_SOURCE_ROOT}/public.sdk/source/vst2.x/audioeffect.h")
    set(CAMELCRUSHER_VST2_AUDIOEFFECTX_H
        "${CAMELCRUSHER_VST2_SOURCE_ROOT}/public.sdk/source/vst2.x/audioeffectx.h")
    if(NOT EXISTS "${CAMELCRUSHER_VST2_AUDIOEFFECT_H}"
       OR NOT EXISTS "${CAMELCRUSHER_VST2_AUDIOEFFECTX_H}")
        message(FATAL_ERROR
            "CAMELCRUSHER_ENABLE_VST2_COMPATIBILITY requires CAMELCRUSHER_VST2_SDK_ROOT to point to a root containing public.sdk/source/vst2.x.")
    endif()

    add_library(camelcrusher_vst2_sdk STATIC
        "${CAMELCRUSHER_VST2_SOURCE_ROOT}/public.sdk/source/vst2.x/audioeffect.cpp"
        "${CAMELCRUSHER_VST2_SOURCE_ROOT}/public.sdk/source/vst2.x/audioeffectx.cpp"
    )

    target_include_directories(camelcrusher_vst2_sdk
        PUBLIC
            src/compat_vst2/sdk
            "${CAMELCRUSHER_VST2_SOURCE_ROOT}/public.sdk/source/vst2.x"
    )

    target_compile_features(camelcrusher_vst2_sdk PUBLIC cxx_std_20)
    set_target_properties(camelcrusher_vst2_sdk PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )

    add_library(camelcrusher_vst2_shared STATIC
        src/compat_vst2/CamelCrusherVst2Effect.cpp
        src/compat_vst2/CamelCrusherVst2Editor.mm
    )

    target_include_directories(camelcrusher_vst2_shared
        PUBLIC
            include
        PRIVATE
            src/compat_vst2
            src/core
    )

    target_link_libraries(camelcrusher_vst2_shared
        PRIVATE
            camelcrusher_core
            camelcrusher_vst2_sdk
    )

    target_compile_features(camelcrusher_vst2_shared PUBLIC cxx_std_20)
    target_compile_definitions(camelcrusher_vst2_shared
        PRIVATE
            CAMELCRUSHER_RECALLED_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
            CAMELCRUSHER_BRAND_URL="${CAMELCRUSHER_SUPPORT_URL}"
    )
    if(APPLE)
        find_library(VST2_APPKIT_FRAMEWORK AppKit REQUIRED)
        find_library(VST2_FOUNDATION_FRAMEWORK Foundation REQUIRED)
        target_compile_options(camelcrusher_vst2_shared
            PRIVATE
                $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
        )
        target_link_libraries(camelcrusher_vst2_shared
            PRIVATE
                ${VST2_APPKIT_FRAMEWORK}
                ${VST2_FOUNDATION_FRAMEWORK}
        )
    endif()
    set_target_properties(camelcrusher_vst2_shared PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )

    add_library(camelcrusher_vst2_binary MODULE
        src/compat_vst2/CamelCrusherVst2Entry.cpp
    )

    target_include_directories(camelcrusher_vst2_binary
        PRIVATE
            include
            src/compat_vst2
    )

    target_link_libraries(camelcrusher_vst2_binary
        PRIVATE
            camelcrusher_vst2_shared
            camelcrusher_vst2_sdk
    )

    set_target_properties(camelcrusher_vst2_binary PROPERTIES
        OUTPUT_NAME CamelCrusher
        PREFIX ""
        SUFFIX ".bundle"
        BUNDLE TRUE
        POSITION_INDEPENDENT_CODE ON
    )

    set(camelcrusher_vst2_bundle_dir
        "${CMAKE_CURRENT_BINARY_DIR}/CamelCrusher.vst")
    set(camelcrusher_vst2_bundle_binary
        "${camelcrusher_vst2_bundle_dir}/Contents/MacOS/CamelCrusher")
    set(camelcrusher_vst2_bundle_plist
        "${CMAKE_CURRENT_BINARY_DIR}/CamelCrusher-vst2-Info.plist")
    set(camelcrusher_vst2_background
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat_vst2/resources/Background.png")
    set(camelcrusher_vst2_background_original
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat_vst2/resources/Background-Original.png")
    set(camelcrusher_vst2_knob
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat_vst2/resources/Knob.png")
    set(camelcrusher_vst2_on_button
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat_vst2/resources/OnButton.png")
    set(camelcrusher_vst2_phat_button
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat_vst2/resources/PhatButton.png")
    set(camelcrusher_vst2_random_button
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat_vst2/resources/ButtonRandom.png")
    set(camelcrusher_vst2_selector_default
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat_vst2/resources/SelectorPreset-Default.png")

    configure_file(
        src/compat_vst2/Info.plist.in
        "${camelcrusher_vst2_bundle_plist}"
        @ONLY
    )

    add_custom_command(
        OUTPUT "${camelcrusher_vst2_bundle_binary}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${camelcrusher_vst2_bundle_dir}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${camelcrusher_vst2_bundle_dir}/Contents/MacOS"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${camelcrusher_vst2_bundle_dir}/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:camelcrusher_vst2_binary>"
            "${camelcrusher_vst2_bundle_binary}"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${camelcrusher_vst2_bundle_plist}"
            "${camelcrusher_vst2_bundle_dir}/Contents/Info.plist"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${camelcrusher_vst2_background}"
            "${camelcrusher_vst2_bundle_dir}/Contents/Resources/Background.png"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${camelcrusher_vst2_background_original}"
            "${camelcrusher_vst2_bundle_dir}/Contents/Resources/Background-Original.png"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${camelcrusher_vst2_knob}"
            "${camelcrusher_vst2_bundle_dir}/Contents/Resources/Knob.png"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${camelcrusher_vst2_on_button}"
            "${camelcrusher_vst2_bundle_dir}/Contents/Resources/OnButton.png"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${camelcrusher_vst2_phat_button}"
            "${camelcrusher_vst2_bundle_dir}/Contents/Resources/PhatButton.png"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${camelcrusher_vst2_random_button}"
            "${camelcrusher_vst2_bundle_dir}/Contents/Resources/ButtonRandom.png"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${camelcrusher_vst2_selector_default}"
            "${camelcrusher_vst2_bundle_dir}/Contents/Resources/SelectorPreset-Default.png"
        COMMAND /bin/sh -c "printf 'BNDL${CAMELCRUSHER_VST2_SIGNATURE}' > \"${camelcrusher_vst2_bundle_dir}/Contents/PkgInfo\""
        COMMAND /bin/sh -c "/usr/bin/codesign --force --sign - --deep --timestamp=none \"${camelcrusher_vst2_bundle_dir}\" >/dev/null 2>&1 || true"
        DEPENDS
            camelcrusher_vst2_binary
            "${camelcrusher_vst2_bundle_plist}"
            "${camelcrusher_vst2_background}"
            "${camelcrusher_vst2_knob}"
            "${camelcrusher_vst2_on_button}"
            "${camelcrusher_vst2_phat_button}"
            "${camelcrusher_vst2_random_button}"
            "${camelcrusher_vst2_background_original}"
            "${camelcrusher_vst2_selector_default}"
        VERBATIM
    )

    add_custom_target(camelcrusher_vst2 ALL
        DEPENDS "${camelcrusher_vst2_bundle_binary}"
    )

    add_executable(vst2_compat_smoke
        tests/vst2_compat_smoke.cpp
    )

    target_include_directories(vst2_compat_smoke
        PRIVATE
            src/compat_vst2
            src/compat_vst2/sdk
            src/core
            "${CAMELCRUSHER_VST2_SOURCE_ROOT}/public.sdk/source/vst2.x"
    )

    target_link_libraries(vst2_compat_smoke
        PRIVATE
            camelcrusher_core
            camelcrusher_vst2_shared
            camelcrusher_vst2_sdk
    )

    add_test(NAME vst2_compat_smoke COMMAND vst2_compat_smoke)

    add_executable(vst2_bundle_smoke
        tests/vst2_bundle_smoke.cpp
    )

    target_include_directories(vst2_bundle_smoke
        PRIVATE
            src/compat_vst2/sdk
    )

    target_compile_definitions(vst2_bundle_smoke
        PRIVATE
            CAMELCRUSHER_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    )

    add_dependencies(vst2_bundle_smoke camelcrusher_vst2)
    add_test(NAME vst2_bundle_smoke COMMAND vst2_bundle_smoke)

    if(APPLE)
        add_executable(vst2_editor_smoke
            tests/vst2_editor_smoke.mm
        )

        target_include_directories(vst2_editor_smoke
            PRIVATE
                src/compat_vst2
                src/compat_vst2/sdk
                "${CAMELCRUSHER_VST2_SOURCE_ROOT}/public.sdk/source/vst2.x"
        )

        target_compile_options(vst2_editor_smoke
            PRIVATE
                -fobjc-arc
        )

        target_link_libraries(vst2_editor_smoke
            PRIVATE
                camelcrusher_vst2_shared
                camelcrusher_vst2_sdk
                ${VST2_APPKIT_FRAMEWORK}
                ${VST2_FOUNDATION_FRAMEWORK}
        )

        add_test(NAME vst2_editor_smoke COMMAND vst2_editor_smoke)
    endif()
endif()

if(APPLE)
    set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_AUV2_BUILDS OFF CACHE BOOL "" FORCE)
    set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
    set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
    set(SMTG_DISABLE_CODE_SIGNING ON CACHE BOOL "" FORCE)

    add_subdirectory(
        third_party/vst3sdk
        third_party/vst3sdk-build
        EXCLUDE_FROM_ALL
    )

    add_library(camelcrusher_modern_vst3_shared STATIC
        src/modern_vst3/CamelCrusherVst3State.cpp
        src/modern_vst3/CamelCrusherVst3Processor.cpp
        src/modern_vst3/CamelCrusherVst3Controller.cpp
        src/modern_vst3/CamelCrusherVst3Editor.mm
        src/shared_ui/CamelCrusherMacEditorSurface.mm
    )

    target_include_directories(camelcrusher_modern_vst3_shared
        PUBLIC
            include
        PRIVATE
            src/modern_vst3
            src/shared_ui
    )

    target_link_libraries(camelcrusher_modern_vst3_shared
        PRIVATE
            camelcrusher_core
            sdk
    )

    target_compile_features(camelcrusher_modern_vst3_shared PUBLIC cxx_std_20)
    target_compile_definitions(camelcrusher_modern_vst3_shared
        PRIVATE
            CAMELCRUSHER_RECALLED_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
            CAMELCRUSHER_BRAND_URL="${CAMELCRUSHER_SUPPORT_URL}"
    )
    target_compile_options(camelcrusher_modern_vst3_shared
        PRIVATE
            $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
    )
    set_target_properties(camelcrusher_modern_vst3_shared PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )

    set(camelcrusher_modern_vst3_sources
        src/modern_vst3/CamelCrusherVst3Entry.cpp
        src/modern_vst3/CamelCrusherVst3IDs.h
        src/modern_vst3/CamelCrusherVst3Version.h
        src/modern_vst3/CamelCrusherVst3Processor.h
        src/modern_vst3/CamelCrusherVst3Controller.h
        src/modern_vst3/CamelCrusherVst3Editor.h
        src/modern_vst3/CamelCrusherVst3State.h
    )

    smtg_add_vst3plugin(camelcrusher_modern_vst3
        PACKAGE_NAME "CamelCrusher"
        SOURCES_LIST ${camelcrusher_modern_vst3_sources}
    )
    target_sources(camelcrusher_modern_vst3
        PRIVATE
            third_party/vst3sdk/public.sdk/source/main/macmain.cpp
    )
    smtg_target_configure_version_file(camelcrusher_modern_vst3)

    target_include_directories(camelcrusher_modern_vst3
        PRIVATE
            include
            src/modern_vst3
    )

    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(AVFOUNDATION_FRAMEWORK AVFoundation REQUIRED)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
    find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
    find_library(COREAUDIOKIT_FRAMEWORK CoreAudioKit REQUIRED)
    find_program(CODESIGN_EXECUTABLE codesign)

    target_link_libraries(camelcrusher_modern_vst3_shared
        PUBLIC
            ${FOUNDATION_FRAMEWORK}
            ${APPKIT_FRAMEWORK}
    )

    target_link_libraries(camelcrusher_modern_vst3
        PRIVATE
            sdk
            sdk_common
            camelcrusher_modern_vst3_shared
            ${COREFOUNDATION_FRAMEWORK}
            ${FOUNDATION_FRAMEWORK}
            ${APPKIT_FRAMEWORK}
    )

    set(camelcrusher_ui_background
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat_vst2/resources/Background.png")
    set(camelcrusher_ui_knob
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat_vst2/resources/Knob.png")
    set(camelcrusher_ui_on_button
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat_vst2/resources/OnButton.png")
    set(camelcrusher_ui_phat_button
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat_vst2/resources/PhatButton.png")
    set(camelcrusher_ui_random_button
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat_vst2/resources/ButtonRandom.png")
    set(camelcrusher_ui_selector_default
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat_vst2/resources/SelectorPreset-Default.png")

    add_custom_command(TARGET camelcrusher_modern_vst3 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "$<TARGET_PROPERTY:camelcrusher_modern_vst3,SMTG_PLUGIN_PACKAGE_PATH>/Contents/Resources"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${camelcrusher_ui_background}"
                "$<TARGET_PROPERTY:camelcrusher_modern_vst3,SMTG_PLUGIN_PACKAGE_PATH>/Contents/Resources/Background.png"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${camelcrusher_ui_knob}"
                "$<TARGET_PROPERTY:camelcrusher_modern_vst3,SMTG_PLUGIN_PACKAGE_PATH>/Contents/Resources/Knob.png"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${camelcrusher_ui_on_button}"
                "$<TARGET_PROPERTY:camelcrusher_modern_vst3,SMTG_PLUGIN_PACKAGE_PATH>/Contents/Resources/OnButton.png"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${camelcrusher_ui_phat_button}"
                "$<TARGET_PROPERTY:camelcrusher_modern_vst3,SMTG_PLUGIN_PACKAGE_PATH>/Contents/Resources/PhatButton.png"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${camelcrusher_ui_random_button}"
                "$<TARGET_PROPERTY:camelcrusher_modern_vst3,SMTG_PLUGIN_PACKAGE_PATH>/Contents/Resources/ButtonRandom.png"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${camelcrusher_ui_selector_default}"
                "$<TARGET_PROPERTY:camelcrusher_modern_vst3,SMTG_PLUGIN_PACKAGE_PATH>/Contents/Resources/SelectorPreset-Default.png"
    )

    target_compile_features(camelcrusher_modern_vst3 PUBLIC cxx_std_20)

    smtg_target_set_bundle(camelcrusher_modern_vst3
        BUNDLE_IDENTIFIER "${CAMELCRUSHER_VST3_BUNDLE_IDENTIFIER}"
        COMPANY_NAME "${CAMELCRUSHER_COMPANY_NAME}"
    )

    add_executable(modern_vst3_smoke
        tests/modern_vst3_smoke.cpp
        third_party/vst3sdk/public.sdk/source/common/memorystream.cpp
    )

    target_include_directories(modern_vst3_smoke
        PRIVATE
            src/modern_vst3
    )

    target_link_libraries(modern_vst3_smoke
        PRIVATE
            camelcrusher_core
            camelcrusher_modern_vst3_shared
            sdk
            sdk_common
            sdk_hosting
            ${COREFOUNDATION_FRAMEWORK}
            ${FOUNDATION_FRAMEWORK}
            ${APPKIT_FRAMEWORK}
    )

    add_test(NAME modern_vst3_smoke COMMAND modern_vst3_smoke)

    add_executable(modern_vst3_editor_smoke
        tests/modern_vst3_editor_smoke.mm
    )

    target_include_directories(modern_vst3_editor_smoke
        PRIVATE
            src/modern_vst3
    )

    target_link_libraries(modern_vst3_editor_smoke
        PRIVATE
            camelcrusher_core
            camelcrusher_modern_vst3_shared
            sdk
            sdk_common
            sdk_hosting
            ${COREFOUNDATION_FRAMEWORK}
            ${FOUNDATION_FRAMEWORK}
            ${APPKIT_FRAMEWORK}
    )

    target_compile_options(modern_vst3_editor_smoke PRIVATE "-fobjc-arc")

    add_test(NAME modern_vst3_editor_smoke COMMAND modern_vst3_editor_smoke)

    add_executable(modern_vst3_bundle_smoke
        tests/modern_vst3_bundle_smoke.cpp
        third_party/vst3sdk/public.sdk/source/vst/hosting/module_mac.mm
    )

    add_dependencies(modern_vst3_bundle_smoke
        camelcrusher_modern_vst3
    )

    target_include_directories(modern_vst3_bundle_smoke
        PRIVATE
            src/modern_vst3
    )

    target_link_libraries(modern_vst3_bundle_smoke
        PRIVATE
            sdk
            sdk_common
            sdk_hosting
            ${COREFOUNDATION_FRAMEWORK}
            ${FOUNDATION_FRAMEWORK}
            ${APPKIT_FRAMEWORK}
    )

    target_compile_definitions(modern_vst3_bundle_smoke
        PRIVATE
            CAMELCRUSHER_VST3_BUNDLE_PATH="$<TARGET_PROPERTY:camelcrusher_modern_vst3,SMTG_PLUGIN_PACKAGE_PATH>"
    )
    target_compile_options(modern_vst3_bundle_smoke PRIVATE "-fobjc-arc")

    add_test(NAME modern_vst3_bundle_smoke COMMAND modern_vst3_bundle_smoke)

    add_library(camelcrusher_modern_au STATIC
        src/modern_au/CamelCrusherRecalledAudioUnit.mm
        src/modern_au/CamelCrusherRecalledAudioUnitFactory.m
        src/modern_au/CamelCrusherRecalledViewController.mm
        src/shared_ui/CamelCrusherMacEditorSurface.mm
    )

    target_include_directories(camelcrusher_modern_au
        PUBLIC
            include
        PRIVATE
            src/modern_au
            src/shared_ui
    )

    target_link_libraries(camelcrusher_modern_au
        PRIVATE
            camelcrusher_runtime_bridge
            ${FOUNDATION_FRAMEWORK}
            ${AVFOUNDATION_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
            ${APPKIT_FRAMEWORK}
            ${COREAUDIOKIT_FRAMEWORK}
    )

    target_compile_definitions(camelcrusher_modern_au
        PRIVATE
            CAMELCRUSHER_RECALLED_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
            CAMELCRUSHER_BRAND_URL="${CAMELCRUSHER_SUPPORT_URL}"
            CAMELCRUSHER_AU_COMPONENT_VERSION_INT=${CAMELCRUSHER_RELEASE_BUILD_INT}
    )
    target_compile_options(camelcrusher_modern_au PRIVATE "-fobjc-arc")

    add_executable(camelcrusher_modern_au_extension_bin
        src/modern_au/CamelCrusherRecalledExtensionMain.mm
    )

    target_link_libraries(camelcrusher_modern_au_extension_bin
        PRIVATE
            camelcrusher_modern_au
            ${FOUNDATION_FRAMEWORK}
            ${AVFOUNDATION_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
            ${COREAUDIOKIT_FRAMEWORK}
    )

    target_compile_options(camelcrusher_modern_au_extension_bin PRIVATE "-fobjc-arc")
    target_link_options(camelcrusher_modern_au_extension_bin PRIVATE
        "-ObjC"
        "-Wl,-rpath,@loader_path/../Frameworks"
    )
    set_target_properties(camelcrusher_modern_au_extension_bin PROPERTIES
        OUTPUT_NAME "${CAMELCRUSHER_AU_EXTENSION_EXECUTABLE}"
        PREFIX ""
    )

    set(CAMELCRUSHER_AU_BUNDLE_EXECUTABLE "${CAMELCRUSHER_AU_EXTENSION_EXECUTABLE}")
    set(CAMELCRUSHER_AU_BUNDLE_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/${CAMELCRUSHER_AU_EXTENSION_EXECUTABLE}.appex")
    set(CAMELCRUSHER_AU_BUNDLE_CONTENTS_DIR
        "${CAMELCRUSHER_AU_BUNDLE_DIR}/Contents")
    set(CAMELCRUSHER_AU_BUNDLE_RESOURCES_DIR
        "${CAMELCRUSHER_AU_BUNDLE_CONTENTS_DIR}/Resources")
    set(CAMELCRUSHER_AU_BUNDLE_BINARY
        "${CAMELCRUSHER_AU_BUNDLE_CONTENTS_DIR}/MacOS/${CAMELCRUSHER_AU_BUNDLE_EXECUTABLE}")
    set(CAMELCRUSHER_AU_BUNDLE_FRAMEWORKS_DIR
        "${CAMELCRUSHER_AU_BUNDLE_CONTENTS_DIR}/Frameworks")
    set(CAMELCRUSHER_AU_BUNDLE_RUNTIME_BRIDGE
        "${CAMELCRUSHER_AU_BUNDLE_FRAMEWORKS_DIR}/libcamelcrusher_runtime_bridge.dylib")
    set(CAMELCRUSHER_AU_BUNDLE_STAMP
        "${CMAKE_CURRENT_BINARY_DIR}/${CAMELCRUSHER_AU_EXTENSION_EXECUTABLE}.bundle.stamp")
    set(CAMELCRUSHER_AU_INFO_PLIST
        "${CMAKE_CURRENT_BINARY_DIR}/${CAMELCRUSHER_AU_EXTENSION_EXECUTABLE}-Info.plist")

    set(CAMELCRUSHER_AU_INFO_DISPLAY_NAME "${CAMELCRUSHER_AU_EXTENSION_DISPLAY_NAME}")
    set(CAMELCRUSHER_AU_BUNDLE_IDENTIFIER
        "${CAMELCRUSHER_AU_BUNDLE_IDENTIFIER}.service")
    set(CAMELCRUSHER_AU_EXTENSION_POINT_IDENTIFIER "com.apple.AudioUnit")
    set(CAMELCRUSHER_AU_PRINCIPAL_CLASS "CamelCrusherRecalledAudioUnitFactory")
    set(CAMELCRUSHER_AU_FACTORY_CLASS "CamelCrusherRecalledAudioUnitFactory")
    set(CAMELCRUSHER_AU_SHORT_VERSION "${CAMELCRUSHER_RELEASE_VERSION_STRING}")
    set(CAMELCRUSHER_AU_BUILD_VERSION "${CAMELCRUSHER_RELEASE_BUILD}")
    set(CAMELCRUSHER_AU_COMPONENT_VERSION "${CAMELCRUSHER_RELEASE_BUILD}")

    configure_file(
        src/modern_au/Info.plist.in
        "${CAMELCRUSHER_AU_INFO_PLIST}"
        @ONLY
    )

    set(CAMELCRUSHER_AU_UI_BUNDLE_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/${CAMELCRUSHER_AU_EXTENSION_EXECUTABLE}UI.appex")
    set(CAMELCRUSHER_AU_UI_BUNDLE_CONTENTS_DIR
        "${CAMELCRUSHER_AU_UI_BUNDLE_DIR}/Contents")
    set(CAMELCRUSHER_AU_UI_BUNDLE_RESOURCES_DIR
        "${CAMELCRUSHER_AU_UI_BUNDLE_CONTENTS_DIR}/Resources")
    set(CAMELCRUSHER_AU_UI_BUNDLE_FRAMEWORKS_DIR
        "${CAMELCRUSHER_AU_UI_BUNDLE_CONTENTS_DIR}/Frameworks")
    set(CAMELCRUSHER_AU_UI_BUNDLE_BINARY
        "${CAMELCRUSHER_AU_UI_BUNDLE_CONTENTS_DIR}/MacOS/${CAMELCRUSHER_AU_BUNDLE_EXECUTABLE}")
    set(CAMELCRUSHER_AU_UI_BUNDLE_RUNTIME_BRIDGE
        "${CAMELCRUSHER_AU_UI_BUNDLE_FRAMEWORKS_DIR}/libcamelcrusher_runtime_bridge.dylib")
    set(CAMELCRUSHER_AU_UI_BUNDLE_STAMP
        "${CMAKE_CURRENT_BINARY_DIR}/${CAMELCRUSHER_AU_EXTENSION_EXECUTABLE}UI.bundle.stamp")
    set(CAMELCRUSHER_AU_UI_INFO_PLIST
        "${CMAKE_CURRENT_BINARY_DIR}/${CAMELCRUSHER_AU_EXTENSION_EXECUTABLE}UI-Info.plist")

    set(CAMELCRUSHER_AU_INFO_DISPLAY_NAME "${CAMELCRUSHER_AU_EXTENSION_DISPLAY_NAME}")
    set(CAMELCRUSHER_AU_BUNDLE_IDENTIFIER "${CAMELCRUSHER_AU_BUNDLE_IDENTIFIER}")
    set(CAMELCRUSHER_AU_EXTENSION_POINT_IDENTIFIER "com.apple.AudioUnit-UI")
    set(CAMELCRUSHER_AU_PRINCIPAL_CLASS "CamelCrusherRecalledViewController")
    set(CAMELCRUSHER_AU_FACTORY_CLASS "CamelCrusherRecalledViewController")
    set(CAMELCRUSHER_AU_SHORT_VERSION "${CAMELCRUSHER_RELEASE_VERSION_STRING}")
    set(CAMELCRUSHER_AU_BUILD_VERSION "${CAMELCRUSHER_RELEASE_BUILD}")
    set(CAMELCRUSHER_AU_COMPONENT_VERSION "${CAMELCRUSHER_RELEASE_BUILD}")

    configure_file(
        src/modern_au/Info.plist.in
        "${CAMELCRUSHER_AU_UI_INFO_PLIST}"
        @ONLY
    )

    add_custom_command(
        OUTPUT "${CAMELCRUSHER_AU_BUNDLE_STAMP}"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${CAMELCRUSHER_AU_BUNDLE_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${CAMELCRUSHER_AU_BUNDLE_CONTENTS_DIR}/MacOS"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${CAMELCRUSHER_AU_BUNDLE_RESOURCES_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${CAMELCRUSHER_AU_BUNDLE_FRAMEWORKS_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:camelcrusher_modern_au_extension_bin>"
                "${CAMELCRUSHER_AU_BUNDLE_BINARY}"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:camelcrusher_runtime_bridge>"
                "${CAMELCRUSHER_AU_BUNDLE_RUNTIME_BRIDGE}"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${CAMELCRUSHER_AU_INFO_PLIST}"
                "${CAMELCRUSHER_AU_BUNDLE_CONTENTS_DIR}/Info.plist"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${camelcrusher_ui_background}"
                "${CAMELCRUSHER_AU_BUNDLE_RESOURCES_DIR}/Background.png"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${camelcrusher_ui_knob}"
                "${CAMELCRUSHER_AU_BUNDLE_RESOURCES_DIR}/Knob.png"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${camelcrusher_ui_on_button}"
                "${CAMELCRUSHER_AU_BUNDLE_RESOURCES_DIR}/OnButton.png"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${camelcrusher_ui_phat_button}"
                "${CAMELCRUSHER_AU_BUNDLE_RESOURCES_DIR}/PhatButton.png"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${camelcrusher_ui_random_button}"
                "${CAMELCRUSHER_AU_BUNDLE_RESOURCES_DIR}/ButtonRandom.png"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${camelcrusher_ui_selector_default}"
                "${CAMELCRUSHER_AU_BUNDLE_RESOURCES_DIR}/SelectorPreset-Default.png"
        COMMAND "${CMAKE_COMMAND}" -E touch
                "${CAMELCRUSHER_AU_BUNDLE_STAMP}"
        DEPENDS
            camelcrusher_modern_au_extension_bin
            camelcrusher_runtime_bridge
            "${CAMELCRUSHER_AU_INFO_PLIST}"
        VERBATIM
    )

    add_custom_target(camelcrusher_modern_au_bundle
        DEPENDS "${CAMELCRUSHER_AU_BUNDLE_STAMP}"
    )

    add_custom_command(
        OUTPUT "${CAMELCRUSHER_AU_UI_BUNDLE_STAMP}"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${CAMELCRUSHER_AU_UI_BUNDLE_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${CAMELCRUSHER_AU_UI_BUNDLE_CONTENTS_DIR}/MacOS"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${CAMELCRUSHER_AU_UI_BUNDLE_RESOURCES_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${CAMELCRUSHER_AU_UI_BUNDLE_FRAMEWORKS_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:camelcrusher_modern_au_extension_bin>"
                "${CAMELCRUSHER_AU_UI_BUNDLE_BINARY}"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:camelcrusher_runtime_bridge>"
                "${CAMELCRUSHER_AU_UI_BUNDLE_RUNTIME_BRIDGE}"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${CAMELCRUSHER_AU_UI_INFO_PLIST}"
                "${CAMELCRUSHER_AU_UI_BUNDLE_CONTENTS_DIR}/Info.plist"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${camelcrusher_ui_background}"
                "${CAMELCRUSHER_AU_UI_BUNDLE_RESOURCES_DIR}/Background.png"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${camelcrusher_ui_knob}"
                "${CAMELCRUSHER_AU_UI_BUNDLE_RESOURCES_DIR}/Knob.png"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${camelcrusher_ui_on_button}"
                "${CAMELCRUSHER_AU_UI_BUNDLE_RESOURCES_DIR}/OnButton.png"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${camelcrusher_ui_phat_button}"
                "${CAMELCRUSHER_AU_UI_BUNDLE_RESOURCES_DIR}/PhatButton.png"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${camelcrusher_ui_random_button}"
                "${CAMELCRUSHER_AU_UI_BUNDLE_RESOURCES_DIR}/ButtonRandom.png"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${camelcrusher_ui_selector_default}"
                "${CAMELCRUSHER_AU_UI_BUNDLE_RESOURCES_DIR}/SelectorPreset-Default.png"
        COMMAND "${CMAKE_COMMAND}" -E touch
                "${CAMELCRUSHER_AU_UI_BUNDLE_STAMP}"
        DEPENDS
            camelcrusher_modern_au_extension_bin
            camelcrusher_runtime_bridge
            "${CAMELCRUSHER_AU_UI_INFO_PLIST}"
        VERBATIM
    )

    add_custom_target(camelcrusher_modern_au_ui_bundle
        DEPENDS "${CAMELCRUSHER_AU_UI_BUNDLE_STAMP}"
    )

    set(CAMELCRUSHER_AU_HOST_APP_EXECUTABLE "${CAMELCRUSHER_AU_HOST_APP_EXECUTABLE}")
    set(CAMELCRUSHER_AU_HOST_APP_IDENTIFIER "${CAMELCRUSHER_AU_HOST_APP_IDENTIFIER}")
    set(CAMELCRUSHER_AU_HOST_APP_INFO_PLIST
        "${CMAKE_CURRENT_BINARY_DIR}/${CAMELCRUSHER_AU_HOST_APP_EXECUTABLE}-Info.plist")
    set(CAMELCRUSHER_AU_HOST_APP_BUNDLE_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/${CAMELCRUSHER_AU_HOST_APP_EXECUTABLE}.app")
    set(CAMELCRUSHER_AU_HOST_APP_PLUGIN_DIR
        "${CAMELCRUSHER_AU_HOST_APP_BUNDLE_DIR}/Contents/PlugIns")
    set(CAMELCRUSHER_AU_HOST_APP_EMBEDDED_BUNDLE_DIR
        "${CAMELCRUSHER_AU_HOST_APP_PLUGIN_DIR}/${CAMELCRUSHER_AU_EXTENSION_EXECUTABLE}UI.appex")
    set(CAMELCRUSHER_AU_HOST_APP_BINARY
        "${CAMELCRUSHER_AU_HOST_APP_BUNDLE_DIR}/Contents/MacOS/${CAMELCRUSHER_AU_HOST_APP_EXECUTABLE}")
    set(CAMELCRUSHER_AU_HOST_APP_EMBEDDED_BINARY
        "${CAMELCRUSHER_AU_HOST_APP_EMBEDDED_BUNDLE_DIR}/Contents/MacOS/${CAMELCRUSHER_AU_BUNDLE_EXECUTABLE}")
    set(CAMELCRUSHER_AU_HOST_APP_EMBEDDED_RUNTIME_BRIDGE
        "${CAMELCRUSHER_AU_HOST_APP_EMBEDDED_BUNDLE_DIR}/Contents/Frameworks/libcamelcrusher_runtime_bridge.dylib")
    set(CAMELCRUSHER_AU_HOST_APP_CONTAINER_STAMP
        "${CMAKE_CURRENT_BINARY_DIR}/${CAMELCRUSHER_AU_HOST_APP_EXECUTABLE}.container.stamp")
    set(CAMELCRUSHER_AU_HOST_APP_ENTITLEMENTS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/modern_au/HostApp.entitlements")
    set(CAMELCRUSHER_AU_EXTENSION_ENTITLEMENTS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/modern_au/AudioUnitExtension.entitlements")

    configure_file(
        src/modern_au/HostInfo.plist.in
        "${CAMELCRUSHER_AU_HOST_APP_INFO_PLIST}"
        @ONLY
    )

    add_executable(camelcrusher_modern_au_host_app MACOSX_BUNDLE
        src/modern_au/CamelCrusherRecalledHostMain.mm
    )

    target_link_libraries(camelcrusher_modern_au_host_app
        PRIVATE
            ${FOUNDATION_FRAMEWORK}
            ${APPKIT_FRAMEWORK}
    )

    target_compile_options(camelcrusher_modern_au_host_app PRIVATE "-fobjc-arc")

    set_target_properties(camelcrusher_modern_au_host_app PROPERTIES
        OUTPUT_NAME "${CAMELCRUSHER_AU_HOST_APP_EXECUTABLE}"
        MACOSX_BUNDLE_INFO_PLIST "${CAMELCRUSHER_AU_HOST_APP_INFO_PLIST}"
    )

    set(CAMELCRUSHER_AU_HOST_SIGN_COMMANDS)
    if(CODESIGN_EXECUTABLE)
        list(APPEND CAMELCRUSHER_AU_HOST_SIGN_COMMANDS
            COMMAND "${CODESIGN_EXECUTABLE}" --force --sign
                    "${CAMELCRUSHER_AU_CODESIGN_IDENTITY}" --timestamp=none
                    "${CAMELCRUSHER_AU_HOST_APP_EMBEDDED_RUNTIME_BRIDGE}"
            COMMAND "${CODESIGN_EXECUTABLE}" --force --sign
                    "${CAMELCRUSHER_AU_CODESIGN_IDENTITY}" --timestamp=none
                    --entitlements "${CAMELCRUSHER_AU_EXTENSION_ENTITLEMENTS}"
                    "${CAMELCRUSHER_AU_HOST_APP_EMBEDDED_BINARY}"
            COMMAND "${CODESIGN_EXECUTABLE}" --force --sign
                    "${CAMELCRUSHER_AU_CODESIGN_IDENTITY}" --timestamp=none
                    --entitlements "${CAMELCRUSHER_AU_EXTENSION_ENTITLEMENTS}"
                    "${CAMELCRUSHER_AU_HOST_APP_EMBEDDED_BUNDLE_DIR}"
            COMMAND "${CODESIGN_EXECUTABLE}" --force --sign
                    "${CAMELCRUSHER_AU_CODESIGN_IDENTITY}" --timestamp=none
                    --entitlements "${CAMELCRUSHER_AU_HOST_APP_ENTITLEMENTS}"
                    "${CAMELCRUSHER_AU_HOST_APP_BINARY}"
            COMMAND "${CODESIGN_EXECUTABLE}" --force --sign
                    "${CAMELCRUSHER_AU_CODESIGN_IDENTITY}" --timestamp=none
                    --entitlements "${CAMELCRUSHER_AU_HOST_APP_ENTITLEMENTS}"
                    "${CAMELCRUSHER_AU_HOST_APP_BUNDLE_DIR}"
        )
    endif()

    add_custom_command(
        OUTPUT "${CAMELCRUSHER_AU_HOST_APP_CONTAINER_STAMP}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${CAMELCRUSHER_AU_HOST_APP_PLUGIN_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
                "${CAMELCRUSHER_AU_HOST_APP_EMBEDDED_BUNDLE_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CAMELCRUSHER_AU_UI_BUNDLE_DIR}"
                "${CAMELCRUSHER_AU_HOST_APP_EMBEDDED_BUNDLE_DIR}"
        ${CAMELCRUSHER_AU_HOST_SIGN_COMMANDS}
        COMMAND "${CMAKE_COMMAND}" -E touch
                "${CAMELCRUSHER_AU_HOST_APP_CONTAINER_STAMP}"
        DEPENDS
            camelcrusher_modern_au_host_app
            camelcrusher_modern_au_ui_bundle
            "${CAMELCRUSHER_AU_UI_BUNDLE_STAMP}"
            "${CAMELCRUSHER_AU_HOST_APP_INFO_PLIST}"
        VERBATIM
    )

    add_custom_target(camelcrusher_modern_au_container_app
        DEPENDS "${CAMELCRUSHER_AU_HOST_APP_CONTAINER_STAMP}"
    )

    add_executable(modern_au_smoke
        tests/modern_au_smoke.mm
    )

    target_link_libraries(modern_au_smoke
        PRIVATE
            camelcrusher_core
            camelcrusher_modern_au
            ${FOUNDATION_FRAMEWORK}
            ${AVFOUNDATION_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
    )

    target_compile_options(modern_au_smoke PRIVATE "-fobjc-arc")
    target_link_options(modern_au_smoke PRIVATE
        "-Wl,-rpath,@loader_path"
    )

    add_test(NAME modern_au_smoke COMMAND modern_au_smoke)

    add_executable(modern_au_bundle_smoke
        tests/modern_au_bundle_smoke.mm
    )

    add_dependencies(modern_au_bundle_smoke
        camelcrusher_modern_au_ui_bundle
    )

    target_link_libraries(modern_au_bundle_smoke
        PRIVATE
            ${FOUNDATION_FRAMEWORK}
            ${AVFOUNDATION_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
    )

    target_compile_options(modern_au_bundle_smoke PRIVATE "-fobjc-arc")
    target_compile_definitions(modern_au_bundle_smoke
        PRIVATE
            CAMELCRUSHER_AU_BUNDLE_PATH="${CAMELCRUSHER_AU_UI_BUNDLE_DIR}"
    )
    target_link_options(modern_au_bundle_smoke PRIVATE
        "-Wl,-rpath,@loader_path"
    )

    add_test(NAME modern_au_bundle_smoke COMMAND modern_au_bundle_smoke)

    add_executable(modern_au_container_smoke
        tests/modern_au_container_smoke.mm
    )

    add_dependencies(modern_au_container_smoke
        camelcrusher_modern_au_container_app
    )

    target_link_libraries(modern_au_container_smoke
        PRIVATE
            ${FOUNDATION_FRAMEWORK}
            ${AVFOUNDATION_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
    )

    target_compile_options(modern_au_container_smoke PRIVATE "-fobjc-arc")
    target_compile_definitions(modern_au_container_smoke
        PRIVATE
            CAMELCRUSHER_AU_HOST_APP_PATH="${CAMELCRUSHER_AU_HOST_APP_BUNDLE_DIR}"
            CAMELCRUSHER_AU_EMBEDDED_BUNDLE_PATH="${CAMELCRUSHER_AU_HOST_APP_EMBEDDED_BUNDLE_DIR}"
    )
    target_link_options(modern_au_container_smoke PRIVATE
        "-Wl,-rpath,@loader_path"
    )

    add_test(NAME modern_au_container_smoke COMMAND modern_au_container_smoke)

    add_executable(modern_au_component_probe
        tools/modern_au_component_probe.mm
    )

    target_link_libraries(modern_au_component_probe
        PRIVATE
            ${FOUNDATION_FRAMEWORK}
            ${AVFOUNDATION_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
    )

    target_compile_options(modern_au_component_probe PRIVATE "-fobjc-arc")

    add_executable(modern_au_view_probe
        tools/modern_au_view_probe.mm
    )

    target_link_libraries(modern_au_view_probe
        PRIVATE
            ${FOUNDATION_FRAMEWORK}
            ${APPKIT_FRAMEWORK}
            ${AVFOUNDATION_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
            ${COREAUDIOKIT_FRAMEWORK}
    )

    target_compile_options(modern_au_view_probe PRIVATE "-fobjc-arc")

    add_executable(modern_au_component_instantiate
        tools/modern_au_component_instantiate.mm
    )

    target_link_libraries(modern_au_component_instantiate
        PRIVATE
            ${FOUNDATION_FRAMEWORK}
            ${AVFOUNDATION_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
            ${COREAUDIOKIT_FRAMEWORK}
    )

    target_compile_options(modern_au_component_instantiate PRIVATE "-fobjc-arc")

    add_executable(render_mac_editor_preview
        tools/render_mac_editor_preview.mm
        src/shared_ui/CamelCrusherMacEditorSurface.mm
    )

    target_include_directories(render_mac_editor_preview
        PRIVATE
            include
            src/core
            src/shared_ui
    )

    target_link_libraries(render_mac_editor_preview
        PRIVATE
            camelcrusher_core
            ${FOUNDATION_FRAMEWORK}
            ${APPKIT_FRAMEWORK}
    )

    target_compile_definitions(render_mac_editor_preview
        PRIVATE
            CAMELCRUSHER_RECALLED_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
            CAMELCRUSHER_BRAND_URL="${CAMELCRUSHER_SUPPORT_URL}"
    )

    target_compile_options(render_mac_editor_preview PRIVATE "-fobjc-arc")
endif()

find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME fixture_extractor_unit
        COMMAND "${Python3_EXECUTABLE}" tests/test_fixture_extractor.py
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    add_test(
        NAME modern_state_tool_unit
        COMMAND "${Python3_EXECUTABLE}" tests/test_modern_state_tool.py
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    set_tests_properties(modern_state_tool_unit PROPERTIES
        ENVIRONMENT "CAMELCRUSHER_BRIDGE_LIB=$<TARGET_FILE:camelcrusher_runtime_bridge>"
    )
endif()

# Copyright 2022-present Contributors to the colorpicker project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/colorpicker

cmake_minimum_required (VERSION 3.27)
set (project_name "Colorpicker")

project (${project_name})

# packages
set (qt6_modules Core PrintSupport Gui Widgets)
find_package (Qt6 COMPONENTS ${qt6_modules} CONFIG REQUIRED)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package (OpenCV CONFIG REQUIRED)
find_package (Lcms2 REQUIRED)

# sources
set (app_sources
    colorpicker.cpp
    colorpicker.mm
    colorpicker.h
    colorwheel.cpp
    colorwheel.h
    dragger.h
    dragger.cpp
    editor.h
    editor.cpp
    eventfilter.h
    eventfilter.cpp
    icctransform.h
    icctransform.cpp
    label.h
    label.cpp
    mac.mm
    mac.h
    main.cpp
    picker.h
    picker.cpp
    about.ui
    editor.ui
    colorpicker.ui
    colorpicker.qrc
)

# iccprofiles
file (GLOB app_iccprofiles
    "iccprofiles/*.icc" 
)

# resources
file (GLOB app_resources
    "resources/*.icns" 
    "resources/*.icc"
    "resources/*.css"
    "resources/App*.png"
)

# bundle
set (bundle_sources
    "${CMAKE_SOURCE_DIR}/resources/MacOSXBundle.plist.in"
)

if (APPLE)
    set (MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.mikaelsundell.colorpicker")
    set (MACOSX_BUNDLE_EXECUTABLE_NAME ${project_name})
    set (MACOSX_BUNDLE_INFO_STRING ${project_name})
    set (MACOSX_BUNDLE_BUNDLE_NAME ${project_name})
    set (MACOSX_BUNDLE_ICON_FILE AppIcon.icns)
    set (MACOSX_BUNDLE_LONG_VERSION_STRING "1.0.61")
    set (MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.6")
    set (MACOSX_BUNDLE_BUNDLE_VERSION "1.0.61")
    set (MACOSX_BUNDLE_COPYRIGHT "Copyright 2022-present Contributors to the ${project_name} project")
    set (MACOSX_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET})
    set (CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "-o linker-signed")
    set_source_files_properties(${app_iccprofiles} PROPERTIES MACOSX_PACKAGE_LOCATION "ICCProfiles")
    set_source_files_properties(${app_resources} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    add_executable (${project_name} MACOSX_BUNDLE ${app_sources} ${app_resources} ${app_iccprofiles})
    # definitions
    add_definitions(-DMACOSX_BUNDLE_GUI_IDENTIFIER="${MACOSX_BUNDLE_GUI_IDENTIFIER}")
    add_definitions(-DMACOSX_BUNDLE_COPYRIGHT="${MACOSX_BUNDLE_COPYRIGHT}")
    add_definitions(-DMACOSX_BUNDLE_LONG_VERSION_STRING="${MACOSX_BUNDLE_LONG_VERSION_STRING}")
    add_definitions(-DGITHUBURL="https://github.com/mikaelsundell/colorpicker")
    set_target_properties(${project_name} PROPERTIES
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${MACOSX_BUNDLE_GUI_IDENTIFIER}"
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET}
        MACOSX_BUNDLE_INFO_PLIST ${bundle_sources}
    )
    if (NOT PROVISIONING_PROFILE STREQUAL "")
        set_target_properties(${project_name} PROPERTIES
            XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${PROVISIONING_PROFILE}"
        )
    endif()
    set_target_properties(${project_name} PROPERTIES
        OUTPUT_NAME ${project_name}
    )
    target_compile_options (${project_name} PRIVATE -Wno-deprecated-register)
    target_include_directories (${project_name} PRIVATE ${LCMS2_INCLUDE_DIR})
    target_link_libraries (${project_name} 
        Qt6::Core Qt6::PrintSupport Qt6::Gui Qt6::Widgets
        opencv_core
        opencv_imgproc
        opencv_imgcodecs
        ${LCMS2_LIBRARY}
        "-framework CoreFoundation"
        "-framework AppKit")
else ()
    message (WARNING "${project_name} is a Mac program, will not be built.")
endif ()

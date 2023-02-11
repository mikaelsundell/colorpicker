# Copyright 2022-present Contributors to the colorpicker project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/colorpicker

cmake_minimum_required( VERSION 3.0 )
set( project_name "Colorpicker" )

project( ${project_name} )

# packages
set (qt5_modules Core Gui Widgets OpenGL)
find_package (Qt5 COMPONENTS ${qt5_modules})
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTOUIC ON)
if (Qt5_POSITION_INDEPENDENT_CODE)
    set (CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
find_package( Lcms2 REQUIRED )

# sources
set (app_sources
    colorpicker.cpp
    colorpicker.mm
    colorpicker.h
    lcms2.cpp
    lcms2.h
    mac.mm
    mac.h
    main.cpp
    colorpicker.ui
)

# resources
file( GLOB app_resources
    "resources/*.icns" 
    "resources/*.icc"
)

# bundle
set (bundle_sources
    "resources/MacOSXBundle.plist.in"
)

if (APPLE)
    if (Qt5_FOUND)
        set (MACOSX_BUNDLE_EXECUTABLE_NAME ${project_name})
        set (MACOSX_BUNDLE_INFO_STRING ${project_name})
        set (MACOSX_BUNDLE_BUNDLE_NAME ${project_name})
        set (MACOSX_BUNDLE_ICON_FILE AppIcon.icns)
        set (MACOSX_BUNDLE_LONG_VERSION_STRING "1.0")
        set (MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")
        set (MACOSX_BUNDLE_BUNDLE_VERSION "1.0")
        set (MACOSX_BUNDLE_COPYRIGHT "Copyright 2022-present Contributors to the ${project_name} project")
        set (MACOSX_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET})
        set (CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "-o linker-signed")
        set_source_files_properties(${app_resources} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        # executable
        add_executable (${project_name} MACOSX_BUNDLE ${app_sources} ${app_resources})
        set_target_properties(${project_name} PROPERTIES
            OUTPUT_NAME ${project_name}
            MACOSX_FRAMEWORK_INFO_PLIST ${bundle_sources})
        target_compile_options (${project_name} PRIVATE -Wno-deprecated-register)
        target_include_directories (${project_name} PRIVATE ${LCMS2_INCLUDE_DIR})
        target_link_libraries (${project_name} 
            Qt5::Core Qt5::Gui Qt5::Widgets 
            ${LCMS2_LIBRARY}
            "-framework CoreFoundation"
            "-framework AppKit")
    else ()
        message (WARNING "Qt not found, ${project_name} program will not be built.")
    endif ()
else ()
    message (WARNING "${project_name} is a Mac program, will not be built.")
endif ()

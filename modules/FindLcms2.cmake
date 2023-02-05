# Find lcms2 headers and libraries.
#
# This module can take the following variables to define
# custom search locations:
#
#   LCMS2_ROOT
#   LCMS2_LOCATION
#
# This module defines the following variables:
#
#   LCMS2_FOUND         True if Lcms2 was found
#   LCMS2_INCLUDE_DIRS  Where to find Lcms2 header files
#   LCMS2_LIBRARIES     List of Lcms2 libraries to link against

include (FindPackageHandleStandardArgs)

find_path (LCMS2_INCLUDE_DIR NAMES lcms2.h
           HINTS ${LCMS2_ROOT}
                 ${LCMS2_LOCATION}
                 /usr/local/include
                 /usr/include
)

find_library (LCMS2_LIBRARY NAMES lcms2 lcms2.2
              PATH_SUFFIXES lib64 lib
              HINTS ${LCMS2_ROOT}
                    ${LCMS2_LOCATION}
                    /usr/local
                    /usr
)

find_package_handle_standard_args (Lcms2 
    DEFAULT_MSG
    LCMS2_INCLUDE_DIR
    LCMS2_LIBRARY
)

if (LCMS2_FOUND)
    set (LCMS2_INCLUDE_DIR ${LCMS2_INCLUDE_DIR})
    set (LCMS2_LIBRARY ${LCMS2_LIBRARY})
else ()
    set (LCMS2_INCLUDE_DIR)
    set (LCMS2_LIBRARY)
endif ()

mark_as_advanced (
    LCMS2_INCLUDE_DIR
    LCMS2_LIBRARY
)

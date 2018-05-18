# Output variables:
#  MEDIDA_INCLUDE_DIR : e.g., /usr/include/.
#  MEDIDA_LIBRARY     : Library path of MEDIDA library
#  MEDIDA_FOUND       : True if found.

find_path(MEDIDA_INCLUDE_DIR NAME medida/medida.h
  HINTS $ENV{HOME}/local/include /opt/local/include /usr/local/include /usr/include
)

find_library(MEDIDA_LIBRARY NAME medida
  HINTS $ENV{HOME}/local/lib64 $ENV{HOME}/local/lib /usr/local/lib64 /usr/local/lib /opt/local/lib64 /opt/local/lib /usr/lib64 /usr/lib
)

if (MEDIDA_INCLUDE_DIR AND MEDIDA_LIBRARY)
    set(MEDIDA_FOUND TRUE)
    message(STATUS "Found MEDIDA library: inc=${MEDIDA_INCLUDE_DIR}, lib=${MEDIDA_LIBRARY}")
else ()
    set(MEDIDA_FOUND FALSE)
    message(STATUS "WARNING: MEDIDA library not found.")
endif ()

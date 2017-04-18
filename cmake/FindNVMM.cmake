# Output variables:
#  NVMM_INCLUDE_DIR : e.g., /usr/include/.
#  NVMM_LIBRARY     : Library path of NVMM library
#  NVMM_FOUND       : True if found.

find_path(NVMM_INCLUDE_DIR NAME nvmm/memory_manager.h
  HINTS $ENV{HOME}/local/include /opt/local/include /usr/local/include /usr/include
)

find_library(NVMM_LIBRARY NAME nvmm
  HINTS $ENV{HOME}/local/lib64 $ENV{HOME}/local/lib /usr/local/lib64 /usr/local/lib /opt/local/lib64 /opt/local/lib /usr/lib64 /usr/lib
)

if (NVMM_INCLUDE_DIR AND NVMM_LIBRARY)
    set(NVMM_FOUND TRUE)
    message(STATUS "Found NVMM library: inc=${NVMM_INCLUDE_DIR}, lib=${NVMM_LIBRARY}")
else ()
    set(NVMM_FOUND FALSE)
    message(STATUS "WARNING: NVMM library not found.")
endif ()

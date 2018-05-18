# Find the numa policy library.
# Output variables:
#  QUARTZ_INCLUDE_DIR : e.g., /usr/include/.
#  QUARTZ_LIBRARY     : Library path of quartz library
#  QUARTZ_FOUND       : True if found.
FIND_PATH(QUARTZ_INCLUDE_DIR NAME quartz
  HINTS $ENV{HOME}/local/include /opt/local/include /usr/local/include /usr/include)

FIND_LIBRARY(QUARTZ_LIBRARY NAME nvmemul
  HINTS $ENV{HOME}/local/lib64 $ENV{HOME}/local/lib /usr/local/lib64 /usr/local/lib /opt/local/lib64 /opt/local/lib /usr/lib64 /usr/lib
)

IF (QUARTZ_INCLUDE_DIR AND QUARTZ_LIBRARY)
    SET(QUARTZ_FOUND TRUE)
    MESSAGE(STATUS "Found Quartz library: inc=${QUARTZ_INCLUDE_DIR}, lib=${QUARTZ_LIBRARY}")
ELSE ()
    SET(QUARTZ_FOUND FALSE)
    MESSAGE(STATUS "WARNING: Quartz library not found.")
    MESSAGE(STATUS "WARNING: Quartz library available here: https://github.com/HewlettPackard/quartz")
ENDIF ()

find_package(PackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(QUARTZ REQUIRED_VARS QUARTZ_LIBRARY QUARTZ_INCLUDE_DIR)

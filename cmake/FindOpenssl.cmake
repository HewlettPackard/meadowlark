# Output variables:
#  OPENSSL_INCLUDE_DIR : e.g., /usr/include/.
#  OPENSSL_LIBRARY     : Library path of OPENSSL library
#  OPENSSL_FOUND       : True if found.

find_path(OPENSSL_INCLUDE_DIR NAME openssl/ssl.h
  HINTS $ENV{HOME}/local/include /opt/local/include /usr/local/include /usr/include
)

find_library(OPENSSL_LIBRARY NAME ssl
  HINTS $ENV{HOME}/local/lib64 $ENV{HOME}/local/lib /usr/local/lib64 /usr/local/lib /opt/local/lib64 /opt/local/lib /usr/lib64 /usr/lib /usr/lib/x86_64-linux-gnu
)

if (OPENSSL_INCLUDE_DIR AND OPENSSL_LIBRARY)
    set(OPENSSL_FOUND TRUE)
    message(STATUS "Found OPENSSL library: inc=${OPENSSL_INCLUDE_DIR}, lib=${OPENSSL_LIBRARY}")
else ()
    set(OPENSSL_FOUND FALSE)
    message(STATUS "WARNING: OPENSSL library not found.")
	MESSAGE(STATUS "Try: 'sudo yum install openssl openssl-devel' (or sudo apt-get install libssl-dev)")
endif ()

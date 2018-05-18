# 
# (c) Copyright 2016 Hewlett Packard Enterprise Development LP
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Find the holodeck mint library.
# Output variables:
#  RADIXTREE_INCLUDE_DIR : e.g., /usr/include/.
#  RADIXTREE_LIBRARY     : Library path of RADIXTREE library
#  RADIXTREE_FOUND       : True if found.

find_path(KVS_CLIENT_INCLUDE_DIR NAME kvs_client/kvs_client.h
  HINTS $ENV{HOME}/local/include /opt/local/include /usr/local/include /usr/include
)

find_library(KVS_CLIENT_LIBRARY NAME kvs_client
  HINTS $ENV{HOME}/local/lib64 $ENV{HOME}/local/lib /usr/local/lib64 /usr/local/lib /opt/local/lib64 /opt/local/lib /usr/lib64 /usr/lib
)

if (KVS_CLIENT_INCLUDE_DIR AND KVS_CLIENT_LIBRARY)
    set(KVS_CLIENT_FOUND TRUE)
    message(STATUS "Found KVS_CLIENT library: inc=${KVS_CLIENT_INCLUDE_DIR}, lib=${KVS_CLIENT_LIBRARY}")
else ()
    set(KVS_CLIENT_FOUND FALSE)
    message(STATUS "WARNING: KVS_CLIENT library not found.")
endif ()

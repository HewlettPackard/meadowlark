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

find_path(RADIXTREE_INCLUDE_DIR NAME radixtree/kvs.h
  HINTS $ENV{HOME}/local/include /opt/local/include /usr/local/include /usr/include
)

find_library(RADIXTREE_LIBRARY NAME radixtree
  HINTS $ENV{HOME}/local/lib64 $ENV{HOME}/local/lib /usr/local/lib64 /usr/local/lib /opt/local/lib64 /opt/local/lib /usr/lib64 /usr/lib
)

if (RADIXTREE_INCLUDE_DIR AND RADIXTREE_LIBRARY)
    set(RADIXTREE_FOUND TRUE)
    message(STATUS "Found RADIXTREE library: inc=${RADIXTREE_INCLUDE_DIR}, lib=${RADIXTREE_LIBRARY}")
else ()
    set(RADIXTREE_FOUND FALSE)
    message(STATUS "WARNING: RADIXTREE library not found.")
endif ()

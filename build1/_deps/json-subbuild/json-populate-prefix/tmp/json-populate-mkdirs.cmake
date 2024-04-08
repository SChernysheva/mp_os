# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/labs/laba5/mp_os/build1/_deps/json-src"
  "C:/labs/laba5/mp_os/build1/_deps/json-build"
  "C:/labs/laba5/mp_os/build1/_deps/json-subbuild/json-populate-prefix"
  "C:/labs/laba5/mp_os/build1/_deps/json-subbuild/json-populate-prefix/tmp"
  "C:/labs/laba5/mp_os/build1/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp"
  "C:/labs/laba5/mp_os/build1/_deps/json-subbuild/json-populate-prefix/src"
  "C:/labs/laba5/mp_os/build1/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/labs/laba5/mp_os/build1/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/labs/laba5/mp_os/build1/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()

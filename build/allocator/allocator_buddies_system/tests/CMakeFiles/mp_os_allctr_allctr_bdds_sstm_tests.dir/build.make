# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /mnt/c/labs/laba5/mp_os

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/c/labs/laba5/mp_os/build

# Include any dependencies generated for this target.
include allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/compiler_depend.make

# Include the progress variables for this target.
include allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/progress.make

# Include the compile flags for this target's objects.
include allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/flags.make

allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.o: allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/flags.make
allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.o: /mnt/c/labs/laba5/mp_os/allocator/allocator_buddies_system/tests/allocator_buddies_system_tests.cpp
allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.o: allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/mnt/c/labs/laba5/mp_os/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.o"
	cd /mnt/c/labs/laba5/mp_os/build/allocator/allocator_buddies_system/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.o -MF CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.o.d -o CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.o -c /mnt/c/labs/laba5/mp_os/allocator/allocator_buddies_system/tests/allocator_buddies_system_tests.cpp

allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.i"
	cd /mnt/c/labs/laba5/mp_os/build/allocator/allocator_buddies_system/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /mnt/c/labs/laba5/mp_os/allocator/allocator_buddies_system/tests/allocator_buddies_system_tests.cpp > CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.i

allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.s"
	cd /mnt/c/labs/laba5/mp_os/build/allocator/allocator_buddies_system/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /mnt/c/labs/laba5/mp_os/allocator/allocator_buddies_system/tests/allocator_buddies_system_tests.cpp -o CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.s

# Object files for target mp_os_allctr_allctr_bdds_sstm_tests
mp_os_allctr_allctr_bdds_sstm_tests_OBJECTS = \
"CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.o"

# External object files for target mp_os_allctr_allctr_bdds_sstm_tests
mp_os_allctr_allctr_bdds_sstm_tests_EXTERNAL_OBJECTS =

allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0: allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/allocator_buddies_system_tests.cpp.o
allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0: allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/build.make
allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0: lib/libgtest_main.a
allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0: common/libmp_os_cmmn.a
allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0: logger/client_logger/libmp_os_lggr_clnt_lggr.a
allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0: allocator/allocator/libmp_os_allctr_allctr.a
allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0: allocator/allocator_buddies_system/libmp_os_allctr_allctr_bdds_sstm.a
allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0: lib/libgtest.a
allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0: common/libmp_os_cmmn.a
allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0: allocator/allocator/libmp_os_allctr_allctr.a
allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0: logger/logger/libmp_os_lggr_lggr.a
allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0: /usr/lib/x86_64-linux-gnu/librt.so
allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0: allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/mnt/c/labs/laba5/mp_os/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable mp_os_allctr_allctr_bdds_sstm_tests"
	cd /mnt/c/labs/laba5/mp_os/build/allocator/allocator_buddies_system/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/link.txt --verbose=$(VERBOSE)
	cd /mnt/c/labs/laba5/mp_os/build/allocator/allocator_buddies_system/tests && $(CMAKE_COMMAND) -E cmake_symlink_executable mp_os_allctr_allctr_bdds_sstm_tests-1.0 mp_os_allctr_allctr_bdds_sstm_tests

allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests: allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests-1.0

# Rule to build all files generated by this target.
allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/build: allocator/allocator_buddies_system/tests/mp_os_allctr_allctr_bdds_sstm_tests
.PHONY : allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/build

allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/clean:
	cd /mnt/c/labs/laba5/mp_os/build/allocator/allocator_buddies_system/tests && $(CMAKE_COMMAND) -P CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/cmake_clean.cmake
.PHONY : allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/clean

allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/depend:
	cd /mnt/c/labs/laba5/mp_os/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/c/labs/laba5/mp_os /mnt/c/labs/laba5/mp_os/allocator/allocator_buddies_system/tests /mnt/c/labs/laba5/mp_os/build /mnt/c/labs/laba5/mp_os/build/allocator/allocator_buddies_system/tests /mnt/c/labs/laba5/mp_os/build/allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : allocator/allocator_buddies_system/tests/CMakeFiles/mp_os_allctr_allctr_bdds_sstm_tests.dir/depend


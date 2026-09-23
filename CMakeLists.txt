cmake_minimum_required(VERSION 3.16)
project(EmbeddedSystemBasics LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
include(CTest)

option(ENABLE_TSAN "Instrument thread examples with ThreadSanitizer (Linux GCC/Clang)" OFF)
option(ENABLE_ASAN_UBSAN "Instrument examples with AddressSanitizer and UBSan (Linux GCC/Clang)" OFF)

if(ENABLE_TSAN AND ENABLE_ASAN_UBSAN)
    message(FATAL_ERROR "Use separate build directories for TSAN and ASAN/UBSAN.")
endif()
if(ENABLE_TSAN OR ENABLE_ASAN_UBSAN)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
        message(FATAL_ERROR "Sanitizer builds require Linux with GCC or Clang.")
    endif()
endif()

function(add_example name source is_thread)
    add_executable(${name} ${source})
    if(MSVC)
        target_compile_options(${name} PRIVATE /W4 /utf-8)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    if(is_thread)
        target_link_libraries(${name} PRIVATE Threads::Threads)
    endif()
    if(ENABLE_TSAN AND is_thread)
        target_compile_options(${name} PRIVATE -g -O1 -fno-omit-frame-pointer -fsanitize=thread)
        target_link_options(${name} PRIVATE -fsanitize=thread)
    elseif(ENABLE_ASAN_UBSAN)
        target_compile_options(${name} PRIVATE -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined)
        target_link_options(${name} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

add_example(std_thread 01_thread/std_thread.cpp TRUE)
add_example(thread_sync 01_thread/thread_sync.cpp TRUE)

# POSIX examples target Linux; Windows still builds the two standard C++ examples.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_example(pthread_demo 01_thread/pthread_demo.cpp TRUE)
    add_example(fork_demo 02_process/fork_demo.cpp FALSE)
    add_example(exec_demo 02_process/exec_demo.cpp FALSE)
    add_example(pipe_ipc 02_process/pipe_ipc.cpp FALSE)
else()
    message(STATUS "Linux examples skipped: pthread_demo, fork_demo, exec_demo, pipe_ipc")
endif()

if(BUILD_TESTING)
    find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)

    function(add_example_test name target check)
        add_test(NAME ${name}
            COMMAND "${Python3_EXECUTABLE}" -X utf8
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_examples.py"
                "${check}" "$<TARGET_FILE:${target}>" ${ARGN})
        set_tests_properties(${name} PROPERTIES TIMEOUT 10)
        if(ENABLE_ASAN_UBSAN)
            set_tests_properties(${name} PROPERTIES
                ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
        elseif(ENABLE_TSAN)
            set_tests_properties(${name} PROPERTIES ENVIRONMENT "TSAN_OPTIONS=halt_on_error=1")
        endif()
    endfunction()

    add_example_test(std_thread std_thread std_thread)
    add_example_test(thread_sync thread_sync thread_sync)
    set_tests_properties(std_thread thread_sync PROPERTIES LABELS "thread")

    if(TARGET pthread_demo)
        add_example_test(pthread_demo pthread_demo pthread_demo)
        set_tests_properties(pthread_demo PROPERTIES LABELS "thread")
        add_example_test(fork_demo fork_demo fork_demo)
        add_example_test(exec_demo exec_demo exec_demo)
        add_example_test(exec_missing_command exec_demo exec_missing)
        add_example_test(exec_nonzero_status exec_demo exec_nonzero)
        add_example_test(exec_signal_status exec_demo exec_signal)
        set_tests_properties(fork_demo exec_demo exec_missing_command
            exec_nonzero_status exec_signal_status PROPERTIES LABELS "process")

        add_example_test(pipe_default pipe_ipc pipe_default)
        foreach(length IN ITEMS 0 127 128 129 4096 65537)
            add_example_test(pipe_length_${length} pipe_ipc pipe_length ${length})
            set_tests_properties(pipe_length_${length} PROPERTIES LABELS "pipe")
        endforeach()
        add_example_test(pipe_text pipe_ipc pipe_text)
        add_example_test(pipe_usage pipe_ipc pipe_usage)
        set_tests_properties(pipe_default pipe_text pipe_usage PROPERTIES LABELS "pipe")
    endif()
endif()

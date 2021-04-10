# Main CMakeLists

cmake_minimum_required(VERSION 3.13)

project(miraipp LANGUAGES CXX VERSION 0.1.3)

set(CMAKE_SCRIPTS "${PROJECT_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SCRIPTS})

option(BUILD_SHARED_LIBS "Build Mirai++ as a shared library" OFF)
option(MIRAI_BUILD_DOCS "Build Mirai++ documentation" OFF)
option(MIRAI_BUILD_EXAMPLES "Build code examples" ON)
option(MIRAI_INSTALL "Install Mirai++" ON)

function (target_set_options TGT)
    # Set C++20
    target_compile_features(${TGT} PRIVATE cxx_std_20)
    set_target_properties(${TGT} PROPERTIES
        CXX_EXTENSIONS OFF
        INTERPROCEDURAL_OPTIMIZATION ON)

    # MSVC settings
    if (MSVC)
        # Force MSVC to use utf-8 encoding regardless of whether the BOM exists
        target_compile_options(${TGT} PRIVATE "/utf-8")
        # /Zc:__cplusplus is needed for MSVC to produce correct value for the __cplusplus macro
        # If this option is not on, __cplusplus will always expands to 199711L
        if (MSVC_VERSION GREATER_EQUAL 1914)
            target_compile_options(${TGT} PUBLIC
                "/Zc:__cplusplus"
                "/Zc:externConstexpr")
        endif ()
    endif ()

    # Asio requires _WIN32_WINNT to be set
    if (WIN32)
        if (${CMAKE_SYSTEM_VERSION} EQUAL 10) # Windows 10
            set(WIN32_WINNT 0x0A00)
        elseif (${CMAKE_SYSTEM_VERSION} EQUAL 6.3) # Windows 8.1
            set(WIN32_WINNT 0x0603)
        elseif (${CMAKE_SYSTEM_VERSION} EQUAL 6.2) # Windows 8
            set(WIN32_WINNT 0x0602)
        elseif (${CMAKE_SYSTEM_VERSION} EQUAL 6.1) # Windows 7
            set(WIN32_WINNT 0x0601)
        elseif (${CMAKE_SYSTEM_VERSION} EQUAL 6.0) # Windows Vista
            set(WIN32_WINNT 0x0600)
        else () # Windows XP (5.1)
            set(WIN32_WINNT 0x0501)
        endif ()
        target_compile_definitions(${TGT} PRIVATE _WIN32_WINNT=${WIN32_WINNT})
    endif ()

    # Warnings and errors settings
    # Use highest reasonable warning level, and treat warnings as errors
    if (MSVC) # Visual Studio
        if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]") # If default /W3 presents
            string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}") # Change /W3 to /W4
        else ()
            target_compile_options(${TGT} PRIVATE /W4) # Add /W4 directly
        endif ()
        target_compile_options(${TGT} PRIVATE /WX) # Treat warnings as errors
        # Treat all header files specified by angle brackets to be system headers, and ignore all those warnings
        target_compile_options(${TGT} PRIVATE 
            /experimental:external /external:W0 /external:anglebrackets)
    else () # Not Visual Studio, assuming gcc or clang
        target_compile_options(${TGT} PRIVATE
            -Wall -Wextra -pedantic -Wundef -Werror
            -Wno-missing-field-initializers # Aggregate init with base class would be impossible with this warning
            # TODO: gcc trunk currently confuses itself in coroutine code, remove this later when gcc behaves well
            -Wno-unused-value)
    endif ()
endfunction ()

add_subdirectory(lib)

if (MIRAI_BUILD_DOCS)
    add_subdirectory(docs)
endif ()
if (MIRAI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

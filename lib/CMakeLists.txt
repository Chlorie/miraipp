add_library(miraipp)
add_library(miraipp::miraipp ALIAS miraipp)

target_set_output_dir(miraipp)

function (add_headers TGT)
    set(headers ${ARGN})
    list(TRANSFORM headers PREPEND "include/mirai/")
    set(${TGT} ${headers} PARENT_SCOPE)
endfunction ()
function (add_sources TGT)
    set(sources ${ARGN})
    list(TRANSFORM sources PREPEND "src/")
    set(${TGT} ${sources} PARENT_SCOPE)
endfunction ()

add_headers(HEADERS
    "core.h"
    "event.h"
    "message.h"
    "mirai.h"

    "core/bot.h"
    "core/common.h"
    "core/config_types.h"
    "core/exceptions.h"
    "core/export.h"
    "core/format.h"
    "core/info_types.h"
    "core/net_client.h"
    "detail/ex_utils.h"
    "detail/json_fwd.h"
    "detail/filter/filter_queue.h"
    "detail/filter/next_event.h"
    "event/event.h"
    "event/event_base.h"
    "event/event_bases.h"
    "event/event_types.h"
    "event/event_types_fwd.h"
    "message/forwarded_message.h"
    "message/message.h"
    "message/segment.h"
    "message/segment_types.h"
    "message/segment_types_fwd.h"
    "message/sent_message.h"
)
add_sources(SOURCES
    "core/bot.cpp"
    "core/config_types.cpp"
    "core/exceptions.cpp"
    "core/info_types.cpp"
    "core/net_client.cpp"
    "detail/json.h"
    "detail/multipart_builder.h"
    "detail/multipart_builder.cpp"
    "detail/filter/filter_queue.cpp"
    "event/event.cpp"
    "event/event_bases.cpp"
    "event/event_types.cpp"
    "message/forwarded_message.cpp"
    "message/message.cpp"
    "message/segment.cpp"
    "message/segment_types.cpp"
    "message/sent_message.cpp"
)

find_package(clu CONFIG REQUIRED)
find_package(Boost REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Threads)
find_package(simdjson CONFIG REQUIRED)
find_package(unifex CONFIG REQUIRED)

target_sources(miraipp PRIVATE ${HEADERS} ${SOURCES})
target_set_options(miraipp)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(miraipp
        PRIVATE MPP_EXPORT_SHARED
        PUBLIC MPP_BUILD_SHARED)
endif ()

target_include_directories(miraipp
PRIVATE
    ${Boost_INCLUDE_DIRS} # Boost only used in implementation
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(miraipp
PRIVATE
    ${Boost_LIBRARIES}
    simdjson::simdjson
PUBLIC
    clu
    fmt::fmt
    unifex::unifex
)

if (MIRAI_INSTALL)
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/miraippConfigVersion.cmake"
        VERSION ${PACKAGE_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(TARGETS miraipp
        EXPORT miraippTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include
    )

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/miraippConfig.cmake.in"
        "${PROJECT_BINARY_DIR}/miraippConfig.cmake"
        INSTALL_DESTINATION lib/cmake/miraipp
    )

    export(TARGETS miraipp NAMESPACE miraipp:: FILE miraippConfig.cmake)
    install(EXPORT miraippTargets NAMESPACE miraipp:: DESTINATION lib/cmake/miraipp)
    install(FILES "${PROJECT_BINARY_DIR}/miraippConfigVersion.cmake"
                  "${PROJECT_BINARY_DIR}/miraippConfig.cmake"
            DESTINATION lib/cmake/miraipp)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/lib/include/ DESTINATION include)
endif ()

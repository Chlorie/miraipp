add_library(miraipp)

set(LIB_NAME miraipp)

function (add_headers VAR)
    set(headers ${ARGN})
    list(TRANSFORM headers PREPEND "include/mirai/")
    set(${VAR} ${headers} PARENT_SCOPE)
endfunction ()
function (add_sources VAR)
    set(sources ${ARGN})
    list(TRANSFORM sources PREPEND "src/")
    set(${VAR} ${sources} PARENT_SCOPE)
endfunction ()

add_headers(HEADERS
    "core/bot.h"
    "core/common.h"
    "core/config_types.h"
    "core/exceptions.h"
    "core/format.h"
    "core/info_types.h"
    "core/net_client.h"
    "detail/json.h"
    "detail/multipart_builder.h"
    "event/event.h"
    "event/event_base.h"
    "event/event_bases.h"
    "event/event_types.h"
    "event/event_types_fwd.h"
    "message/message.h"
    "message/segment.h"
    "message/segment_types.h"
    "message/segment_types_fwd.h"
    "message/sent_message.h"
    "pattern/pattern.h"
    "pattern/pattern_matcher_queue.h"
    "pattern/pattern_types.h"
)
add_sources(SOURCES
    "core/bot.cpp"
    "core/config_types.cpp"
    "core/exceptions.cpp"
    "core/info_types.cpp"
    "core/net_client.cpp"
    "detail/multipart_builder.cpp"
    "event/event.cpp"
    "event/event_bases.cpp"
    "event/event_types.cpp"
    "message/message.cpp"
    "message/segment.cpp"
    "message/segment_types.cpp"
    "message/sent_message.cpp"
    "pattern/pattern_matcher_queue.cpp"
)

find_package(clu CONFIG REQUIRED)
find_package(Boost REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Threads)
find_package(simdjson CONFIG REQUIRED)

target_sources(miraipp PRIVATE ${HEADERS} ${SOURCES})
target_set_options(miraipp)
target_include_directories(miraipp
PRIVATE
    ${Boost_INCLUDE_DIRS} # Boost only used in implementation
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(miraipp PUBLIC
    clu
    ${Boost_LIBRARIES}
    fmt::fmt
    simdjson::simdjson
)

if (MIRAI_INSTALL)
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/${LIB_NAME}ConfigVersion.cmake"
        VERSION ${PACKAGE_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(TARGETS ${LIB_NAME}
        EXPORT ${LIB_NAME}Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include
    )

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${LIB_NAME}Config.cmake.in"
        "${PROJECT_BINARY_DIR}/${LIB_NAME}Config.cmake"
        INSTALL_DESTINATION lib/cmake/${LIB_NAME}
    )

    install(EXPORT ${LIB_NAME}Targets DESTINATION lib/cmake/${LIB_NAME})
    install(FILES "${PROJECT_BINARY_DIR}/${LIB_NAME}ConfigVersion.cmake"
                  "${PROJECT_BINARY_DIR}/${LIB_NAME}Config.cmake"
            DESTINATION lib/cmake/${LIB_NAME})
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/lib/include/ DESTINATION include)
endif ()

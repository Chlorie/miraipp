add_library(miraipp)

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
    "core/exceptions.h"
    "core/format.h"
    "detail/json.h"
    "detail/net_client.h"
    "message/message.h"
    "message/segment.h"
    "message/segment_types.h"
)
add_sources(SOURCES
    "core/bot.cpp"
    "core/exceptions.cpp"
    "detail/net_client.cpp"
    "message/message.cpp"
    "message/segment.cpp"
    "message/segment_types.cpp"
)

find_package(clu REQUIRED)
find_package(Boost REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Threads)
find_package(simdjson CONFIG REQUIRED)

target_sources(miraipp PUBLIC ${HEADERS} PRIVATE ${SOURCES})
target_set_options(miraipp)
target_include_directories(miraipp PUBLIC
    ${Boost_INCLUDE_DIRS}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(miraipp PUBLIC
    clu
    ${Boost_LIBRARIES}
    fmt::fmt
    simdjson::simdjson
)

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
    "core/info_types.h"
    "detail/json.h"
    "detail/multipart_builder.h"
    "detail/net_client.h"
    "event/event.h"
    "event/event_bases.h"
    "event/event_types.h"
    "message/message.h"
    "message/segment.h"
    "message/segment_types.h"
    "message/sent_message.h"
)
add_sources(SOURCES
    "core/bot.cpp"
    "core/exceptions.cpp"
    "core/info_types.cpp"
    "detail/multipart_builder.cpp"
    "detail/net_client.cpp"
    "event/event.cpp"
    "event/event_bases.cpp"
    "event/event_types.cpp"
    "message/message.cpp"
    "message/segment.cpp"
    "message/segment_types.cpp"
    "message/sent_message.cpp"
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

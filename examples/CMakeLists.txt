# Examples

add_executable(playground "playground.cpp")
target_set_options(playground)

add_executable(sync_repeater "sync_repeater.cpp")
target_set_options(sync_repeater)

find_package(clu REQUIRED)
find_package(Boost REQUIRED)

target_include_directories(playground PUBLIC ${Boost_INCLUDE_DIRS})

target_link_libraries(playground PRIVATE clu ${Boost_LIBRARIES} miraipp)

target_link_libraries(sync_repeater PRIVATE clu miraipp)

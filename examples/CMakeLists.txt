# Examples

add_executable(playground "playground.cpp")
target_set_options(playground)

find_package(clu REQUIRED)
find_package(Boost REQUIRED)

target_include_directories(playground PUBLIC ${Boost_INCLUDE_DIRS})

target_link_libraries(playground PRIVATE clu ${Boost_LIBRARIES} miraipp)

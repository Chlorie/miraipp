# Examples

add_executable(playground "playground.cpp")
target_set_options(playground)

add_executable(sync_repeater "sync_repeater.cpp")
target_set_options(sync_repeater)

target_link_libraries(playground PRIVATE miraipp::miraipp)
target_link_libraries(sync_repeater PRIVATE miraipp::miraipp)

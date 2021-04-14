# Examples

function (add_example TGT)
    add_executable(${TGT} "${TGT}.cpp")
    target_set_options(${TGT})
    target_set_output_dir(${TGT})
    target_link_libraries(${TGT} PRIVATE miraipp::miraipp)
endfunction ()

add_example(playground)
add_example(sync_repeater)

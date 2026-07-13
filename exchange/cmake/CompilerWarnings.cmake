# Applies a strict, consistent warning set to a target.
# Usage: exchange_set_warnings(my_target)
function(exchange_set_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4         # high warning level
            /permissive- # strict standards conformance
            /w14640     # thread-unsafe static member init
        )
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wnon-virtual-dtor
            -Woverloaded-virtual
        )
    endif()
endfunction()
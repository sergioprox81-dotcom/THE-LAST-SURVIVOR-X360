
cmake_minimum_required(VERSION 3.25)
project(the_last_survivor_x360 C)

add_executable(the_last_survivor
    main.c
)

set_target_properties(the_last_survivor PROPERTIES
    C_STANDARD 17
    C_EXTENSIONS ON
)

xsdk_add_shaders(the_last_survivor
    cube.vert
    cube.frag
    ui.vert
    ui.frag
)
)

xsdk_add_xex(the_last_survivor
    TITLE_ID 0x53565331
    NAME "THE LAST SURVIVOR"
    VERSION 1.0.0.0
)

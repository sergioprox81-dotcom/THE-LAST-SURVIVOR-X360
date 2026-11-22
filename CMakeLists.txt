
cmake_minimum_required(VERSION 3.20)
project(the_last_survivor_x360 C)

add_executable(the_last_survivor
    main.c
)

xsdk_add_xex(the_last_survivor
    TITLE_ID 0x53565331
    NAME "THE LAST SURVIVOR"
    VERSION 1.0.0.0
)

cmake_minimum_required(VERSION 3.20)
project(the_last_survivor_x360 C)

add_executable(the_last_survivor
    src/main.c
)

# La configuración específica de Xbox 360 se añadirá cuando se confirme
# el toolchain exacto instalado en el runner.

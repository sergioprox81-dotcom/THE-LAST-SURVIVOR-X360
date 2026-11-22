cmake_minimum_required(VERSION 3.20)
project(the_last_survivor_x360 C)

# Este proyecto se integra con el SDK xsdk.
# Comprueba la sintaxis exacta de xsdk_add_xex en la versión instalada.
add_executable(the_last_survivor src/main.c)
target_include_directories(the_last_survivor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# El SDK define xsdk_add_xex() al cargar su configuración/toolchain.
if(COMMAND xsdk_add_xex)
    xsdk_add_xex(the_last_survivor TITLE_ID 0x53565331 NAME "THE LAST SURVIVOR")
else()
    message(FATAL_ERROR "No se ha cargado xsdk. Sigue docs/GETTING_STARTED.md y samples/template del repositorio xsdk.")
endif()

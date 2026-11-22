# THE LAST SURVIVOR X360 — proyecto inicial

Proyecto original de supervivencia y terror para Xbox 360 RGH/JTAG.

## Estado real
Este paquete es una base de código fuente, no un juego terminado ni un XEX compilado.
No incluye assets de terceros ni contenido de The Last of Us.

## Estructura
- src/main.c: punto de entrada de la aplicación.
- CMakeLists.txt: configuración inicial de CMake.
- GAME_DESIGN.md: alcance de la primera demo.
- INSTALL_AURORA.md: notas de instalación cuando exista un XEX válido.

## Importante
La compilación para Xbox 360 requiere un toolchain/SDK compatible y sus bibliotecas.
No se debe afirmar que el proyecto genera un XEX hasta que la compilación termine
correctamente y el ejecutable resultante se pruebe en hardware o emulador compatible.

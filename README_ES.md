# THE LAST SURVIVOR X360 — DEMO 0.1 (base de desarrollo)

**Objetivo:** juego original de supervivencia para Xbox 360 RGH/JTAG, con estética postapocalíptica.
**Destino:** Xbox 360 RGH con Aurora, disco interno.
**Estado de este paquete:** código inicial de arranque/pantalla y estructura. **No es todavía un juego 3D ni contiene un `.xex` compilado.**

## Qué incluye
- `src/main.c`: arranque del título con xsdk, lectura básica del mando y pantalla de prueba.
- `CMakeLists.txt`: proyecto para el SDK comunitario `xenosworks/xsdk`.
- `GAME_DESIGN.md`: alcance y controles previstos.
- `INSTALL_AURORA.md`: cómo copiar el `.xex` cuando exista una compilación válida.

## Importante sobre compilar desde Android
El SDK xsdk documenta compilación en Linux x86-64 (o Linux dentro de WSL en Windows). No hay en este paquete un compilador Android→Xbox 360 listo para usar. Desde un móvil Android se puede editar y subir el proyecto a un entorno Linux remoto, pero la compilación debe hacerse allí. No instales archivos `.xex` de fuentes desconocidas.

SDK y documentación:
https://github.com/xenosworks/xsdk
Guía de inicio:
https://github.com/xenosworks/xsdk/blob/main/docs/GETTING_STARTED.md

## Compilar
En un entorno Linux x86-64 con xsdk instalado y `~/xsdk-gcc` configurado, integra este directorio en el árbol del SDK o sigue la plantilla `samples/template` del repositorio. La macro `xsdk_add_xex` y las opciones de CMake pueden cambiar entre revisiones; verifica la plantilla de la misma versión del SDK antes de compilar.

## Próximo hito
1. Compilar y probar esta pantalla de arranque en la consola.
2. Añadir una escena 3D sencilla (suelo, paredes y cámara).
3. Añadir personaje original, movimiento con stick izquierdo y cámara con stick derecho.
4. Añadir enemigo, colisiones, vida y objetivo de escape.

No se usan personajes, música, mapas ni recursos extraídos de The Last of Us. Es una obra original inspirada en el género survival horror.

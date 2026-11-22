// THE LAST SURVIVOR X360 — Demo 0.1
// Pantalla de arranque y prueba de mando basada en la API documentada de xsdk.
// Aún no implementa escena 3D ni mecánicas de juego.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"
#include "xsdk/text.h"

enum {
    DISPLAY_BUFFERS = 2,
    SWAP_INTERVAL = 1,
    FRAME_TIMEOUT_MS = 5000
};

int main(void) {
    uint32_t width = 0;
    uint32_t height = 0;

    xsdk_log_write("THE LAST SURVIVOR: starting demo 0.1");

    NTSTATUS status = xsdk_gpu_open();
    if (!NT_SUCCESS(status)) {
        xsdk_log_write("ERROR xsdk_gpu_open: 0x%08X", (unsigned)status);
        return 1;
    }

    status = xsdk_present_open(XSDK_DISPLAY_720P, DISPLAY_BUFFERS,
                               SWAP_INTERVAL, &width, &height);
    if (!NT_SUCCESS(status)) {
        xsdk_log_write("ERROR xsdk_present_open: 0x%08X", (unsigned)status);
        xsdk_gpu_close();
        return 1;
    }

    status = xsdk_text_open();
    if (!NT_SUCCESS(status)) {
        xsdk_log_write("ERROR xsdk_text_open: 0x%08X", (unsigned)status);
        xsdk_present_close();
        xsdk_gpu_close();
        return 1;
    }

    bool running = true;
    while (running) {
        INPUT_STATE input = {0};
        const uint32_t input_result = XamInputGetState(0, 0, &input);

        if (input_result == ERROR_SUCCESS &&
            (input.gamepad.buttons & INPUT_GAMEPAD_BACK)) {
            running = false;
        }

        uint32_t address = 0;
        status = xsdk_present_begin(&address, FRAME_TIMEOUT_MS);
        if (!NT_SUCCESS(status)) {
            xsdk_log_write("ERROR xsdk_present_begin: 0x%08X", (unsigned)status);
            break;
        }

        // Fondo oscuro. El resolve limpia el target para el siguiente frame.
        xsdk_gpu_set_target(width, height);

        xsdk_text_draw(72.0f, 72.0f, 3, 0xFFE5E5E5,
                       "THE LAST SURVIVOR");
        xsdk_text_draw(76.0f, 132.0f, 2, 0xFF8FA6A0,
                       "XBOX 360 RGH - DEMO 0.1");
        xsdk_text_draw(76.0f, 220.0f, 2, 0xFFFFFFFF,
                       "BASE DE DESARROLLO INICIADA");
        xsdk_text_draw(76.0f, 278.0f, 1, 0xFFB8C2C0,
                       "SIGUIENTE: ESCENARIO 3D Y CAMARA EN TERCERA PERSONA");
        xsdk_text_draw(76.0f, 340.0f, 1, 0xFFB8C2C0,
                       "Pulsa BACK para salir a Aurora.");

        xsdk_gpu_resolve(address, width, height, 0xFF101817);
        xsdk_present_end();
    }

    xsdk_text_close();
    xsdk_present_close();
    status = xsdk_gpu_close();
    if (!NT_SUCCESS(status)) {
        xsdk_log_write("WARNING xsdk_gpu_close: 0x%08X", (unsigned)status);
    }

    xsdk_log_write("THE LAST SURVIVOR: returned to dashboard");
    return 0;
}

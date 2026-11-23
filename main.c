/*
 * THE LAST SURVIVOR X360
 * Primera escena visible basada en la plantilla pública de xsdk.
 * Muestra una pantalla oscura y permanece abierta hasta pulsar BACK.
 */

#include <stdbool.h>
#include <stdint.h>

#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"
#include "xsdk/splash.h"

enum {
    SPLASH_MS = 1500,
    BUFFERS = 2,
    SWAP_INTERVAL = 1,
    BEGIN_TIMEOUT_MS = 5000,
    CLEAR_ARGB = 0xFF101820
};

static void run_game(uint32_t width, uint32_t height)
{
    for (;;) {
        INPUT_STATE input = { 0 };

        if (XamInputGetState(0, 0, &input) == ERROR_SUCCESS &&
            (input.gamepad.buttons & INPUT_GAMEPAD_BACK)) {
            break;
        }

        uint32_t address = 0;
        const NTSTATUS status = xsdk_present_begin(&address, BEGIN_TIMEOUT_MS);

        if (!NT_SUCCESS(status)) {
            xsdk_log_write("THE LAST SURVIVOR: present_begin fallo: 0x%08X",
                           (unsigned)status);
            break;
        }

        xsdk_gpu_resolve(address, width, height, CLEAR_ARGB);
        xsdk_present_end();
    }
}

int main(void)
{
    xsdk_log_write("THE LAST SURVIVOR X360: primera escena");

    NTSTATUS status = xsdk_splash_show(SPLASH_MS);
    if (!NT_SUCCESS(status)) {
        xsdk_log_write("No se pudo mostrar el logo inicial: 0x%08X",
                       (unsigned)status);
    }

    status = xsdk_gpu_open();
    if (!NT_SUCCESS(status)) {
        xsdk_log_write("No se pudo iniciar la GPU: 0x%08X",
                       (unsigned)status);
        return 1;
    }

    uint32_t width = 0;
    uint32_t height = 0;

    status = xsdk_present_open(XSDK_DISPLAY_720P, BUFFERS,
                               SWAP_INTERVAL, &width, &height);
    if (!NT_SUCCESS(status)) {
        xsdk_log_write("No se pudo abrir la pantalla: 0x%08X",
                       (unsigned)status);
        xsdk_gpu_close();
        return 1;
    }

    xsdk_log_write("Pantalla lista: %u x %u",
                   (unsigned)width, (unsigned)height);

    run_game(width, height);

    xsdk_present_close();
    status = xsdk_gpu_close();

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("No se pudo cerrar la GPU: 0x%08X",
                       (unsigned)status);
    }

    xsdk_log_write("THE LAST SURVIVOR: fin");
    return 0;
}


/*
 * THE LAST SURVIVOR X360
 * Primera prueba gráfica: triángulo RGB.
 */

#include <stdint.h>

#include "the_last_survivor_shaders.h"
#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"
#include "xsdk/shader.h"
#include "xsdk/splash.h"

enum {
    SPLASH_MS = 1500,
    BUFFERS = 2,
    SWAP_INTERVAL = 1,
    BEGIN_TIMEOUT_MS = 5000,
    CLEAR_ARGB = 0xFF101820,
    CORNERS = 3,
    VERTEX_FLOATS = 5,
    STRIDE_BYTES = VERTEX_FLOATS * 4,
    VERTICES_BYTES = CORNERS * STRIDE_BYTES
};

static const XsdkVertexElement layout[] = {
    {
        .usage = XSDK_USAGE_POSITION,
        .format = XSDK_VERTEX_FLOAT2,
        .offset_bytes = 0
    },
    {
        .usage = XSDK_USAGE_COLOR,
        .format = XSDK_VERTEX_FLOAT3,
        .offset_bytes = 8
    }
};

static void write_vertices(volatile uint32_t *vertices)
{
    const float data[CORNERS][VERTEX_FLOATS] = {
        {  0.0f,  0.75f, 1.0f, 0.0f, 0.0f },
        { -0.75f, -0.75f, 0.0f, 1.0f, 0.0f },
        {  0.75f, -0.75f, 0.0f, 0.0f, 1.0f }
    };

    uint32_t words[VERTICES_BYTES / 4];

    for (uint32_t i = 0; i < CORNERS * VERTEX_FLOATS; i++) {
        const float *value = &data[0][0] + i;
        __builtin_memcpy(&words[i], value, sizeof(uint32_t));
    }

    for (uint32_t i = 0; i < VERTICES_BYTES / 4; i++) {
        vertices[i] = words[i];
    }

    xsdk_gpu_invalidate(xsdk_gpu_address(vertices), VERTICES_BYTES);
}

static void run_game(
    const XsdkShader *vertex,
    const XsdkShader *pixel,
    volatile uint32_t *vertices,
    uint32_t width,
    uint32_t height)
{
    write_vertices(vertices);

    for (;;) {
        INPUT_STATE input = { 0 };

        if (XamInputGetState(0, 0, &input) == ERROR_SUCCESS &&
            (input.gamepad.buttons & INPUT_GAMEPAD_BACK)) {
            break;
        }

        uint32_t address = 0;
        NTSTATUS status = xsdk_present_begin(
            &address, BEGIN_TIMEOUT_MS
        );

        if (!NT_SUCCESS(status)) {
            xsdk_log_write(
                "Error al preparar la pantalla: 0x%08X",
                (unsigned)status
            );
            break;
        }

        xsdk_gpu_set_target(width, height);
        xsdk_gpu_set_shaders(vertex, pixel);

        xsdk_built_shader_set_literals(&triangle_vert);
        xsdk_built_shader_set_literals(&triangle_frag);

        xsdk_gpu_set_vertices(
            0, xsdk_gpu_address(vertices), VERTICES_BYTES
        );

        xsdk_gpu_draw(XSDK_PRIMITIVE_TRIANGLE_LIST, CORNERS);

        xsdk_gpu_resolve(
            address, width, height, CLEAR_ARGB
        );

        xsdk_present_end();
    }
}

int main(void)
{
    xsdk_log_write("THE LAST SURVIVOR: prueba del triangulo RGB");

    NTSTATUS status = xsdk_splash_show(SPLASH_MS);

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Aviso: no se pudo mostrar el logo");
    }

    status = xsdk_gpu_open();

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Error al iniciar GPU: 0x%08X",
                       (unsigned)status);
        return 1;
    }

    XsdkShader vertex = { .microcode = NULL };
    XsdkShader pixel = { .microcode = NULL };

    status = xsdk_shader_load(
        &vertex, triangle_vert.container,
        triangle_vert.container_bytes
    );

    if (NT_SUCCESS(status)) {
        status = xsdk_shader_load(
            &pixel, triangle_frag.container,
            triangle_frag.container_bytes
        );
    }

    if (NT_SUCCESS(status)) {
        status = xsdk_shader_set_vertex_layout(
            &vertex,
            layout,
            sizeof(layout) / sizeof(layout[0]),
            (const uint32_t[]){ STRIDE_BYTES }
        );
    }

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Error al cargar shaders: 0x%08X",
                       (unsigned)status);
        xsdk_shader_free(&pixel);
        xsdk_shader_free(&vertex);
        xsdk_gpu_close();
        return 1;
    }

    volatile uint32_t *vertices = xsdk_gpu_alloc(VERTICES_BYTES);

    if (!vertices) {
        xsdk_log_write("Error al reservar memoria grafica");
        xsdk_shader_free(&pixel);
        xsdk_shader_free(&vertex);
        xsdk_gpu_close();
        return 1;
    }

    uint32_t width = 0;
    uint32_t height = 0;

    status = xsdk_present_open(
        XSDK_DISPLAY_720P,
        BUFFERS,
        SWAP_INTERVAL,
        &width,
        &height
    );

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Error al abrir pantalla: 0x%08X",
                       (unsigned)status);
        xsdk_gpu_free((void *)vertices);
        xsdk_shader_free(&pixel);
        xsdk_shader_free(&vertex);
        xsdk_gpu_close();
        return 1;
    }

    xsdk_log_write("Pantalla lista: %u x %u",
                   (unsigned)width, (unsigned)height);

    run_game(&vertex, &pixel, vertices, width, height);

    xsdk_present_close();
    xsdk_gpu_free((void *)vertices);
    xsdk_shader_free(&pixel);
    xsdk_shader_free(&vertex);

    status = xsdk_gpu_close();

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Error al cerrar GPU: 0x%08X",
                       (unsigned)status);
    }

    xsdk_log_write("THE LAST SURVIVOR: prueba terminada");
    return 0;
}


#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "the_last_survivor_shaders.h"
#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"
#include "xsdk/shader.h"
#include "xsdk/splash.h"
#include "xsdk/texture.h"

enum {
    SPLASH_MS = 2000,
    BUFFERS = 2,
    SWAP_INTERVAL = 1,
    BEGIN_TIMEOUT_MS = 5000,
    CLEAR_ARGB = 0xFF202020,
    FACES = 6,
    VERTICES = FACES * 4,
    INDICES = FACES * 6,
    VERTEX_FLOATS = 8,
    STRIDE_BYTES = VERTEX_FLOATS * 4,
    COLOR_OFFSET_BYTES = 12,
    UV_OFFSET_BYTES = 24,
    VERTICES_BYTES = VERTICES * STRIDE_BYTES,
    INDICES_BYTES = INDICES * 2,
    TEXTURE_SIZE = 64,
    CHECK_TEXELS = 8
};

static const int32_t STICK_DEAD_ZONE = 8000;
static const float TURN_PER_FRAME = 0.05f;
static const float CAMERA_DISTANCE = 3.0f;
static const float FIELD_OF_VIEW = 1.0f;
static const float NEAR_PLANE = 0.1f;
static const float FAR_PLANE = 10.0f;

static const float faces[FACES][3][3] = {
    {{ 1, 0, 0}, { 0, 0,-1}, { 0, 1, 0}},
    {{-1, 0, 0}, { 0, 0, 1}, { 0, 1, 0}},
    {{ 0, 1, 0}, { 1, 0, 0}, { 0, 0,-1}},
    {{ 0,-1, 0}, { 1, 0, 0}, { 0, 0, 1}},
    {{ 0, 0, 1}, { 1, 0, 0}, { 0, 1, 0}},
    {{ 0, 0,-1}, {-1, 0, 0}, { 0, 1, 0}}
};

static const float face_colours[FACES][3] = {
    {0, 1, 0},
    {1, 1, 0},
    {0, 1, 1},
    {1, 0, 1},
    {1, 0, 0},
    {0, 0, 1}
};

static const XsdkVertexElement layout[] = {
    {
        .usage = XSDK_USAGE_POSITION,
        .format = XSDK_VERTEX_FLOAT3,
        .offset_bytes = 0
    },
    {
        .usage = XSDK_USAGE_COLOR,
        .format = XSDK_VERTEX_FLOAT3,
        .offset_bytes = COLOR_OFFSET_BYTES
    },
    {
        .usage = XSDK_USAGE_TEXCOORD,
        .format = XSDK_VERTEX_FLOAT2,
        .offset_bytes = UV_OFFSET_BYTES
    }
};

typedef struct {
    float m[16];
} Matrix;

typedef struct {
    XsdkShader vertex;
    XsdkShader pixel;
    XsdkTexture texture;
    XsdkSamplerState sampler;
    XsdkDepthStencilState depth;
    volatile uint32_t *vertices;
    volatile uint32_t *indices;
} Cube;

static void write_gpu(
    volatile uint32_t *memory,
    const void *data,
    uint32_t size_bytes)
{
    for (uint32_t i = 0; i < size_bytes / 4; i++) {
        uint32_t word;
        memcpy(&word, (const uint8_t *)data + i * 4, 4);
        memory[i] = word;
    }

    xsdk_gpu_invalidate(
        xsdk_gpu_address(memory),
        size_bytes
    );
}

static bool make_mesh(Cube *cube)
{
    float vertices[VERTICES][VERTEX_FLOATS];
    uint16_t indices[INDICES];

    for (uint32_t f = 0; f < FACES; f++) {
        for (uint32_t c = 0; c < 4; c++) {
            const float across = c & 1 ? 1.0f : -1.0f;
            const float up = c & 2 ? 1.0f : -1.0f;
            float *v = vertices[f * 4 + c];

            for (uint32_t axis = 0; axis < 3; axis++) {
                v[axis] = 0.5f * (
                    faces[f][0][axis] +
                    across * faces[f][1][axis] +
                    up * faces[f][2][axis]
                );

                v[3 + axis] = face_colours[f][axis];
            }

            v[6] = c & 1 ? 1.0f : 0.0f;
            v[7] = c & 2 ? 0.0f : 1.0f;
        }

        const uint16_t base = (uint16_t)(f * 4);

        const uint16_t quad[6] = {
            base, base + 1, base + 2,
            base + 2, base + 1, base + 3
        };

        memcpy(&indices[f * 6], quad, sizeof quad);
    }

    cube->vertices = xsdk_gpu_alloc(VERTICES_BYTES);
    cube->indices = xsdk_gpu_alloc(INDICES_BYTES);

    if (!cube->vertices || !cube->indices) {
        xsdk_log_write("Error reservando memoria del cubo");
        return false;
    }

    write_gpu(cube->vertices, vertices, VERTICES_BYTES);
    write_gpu(cube->indices, indices, INDICES_BYTES);

    return true;
}

static bool make_texture(Cube *cube)
{
    NTSTATUS status = xsdk_texture_create(
        &cube->texture,
        XSDK_FORMAT_A8R8G8B8,
        TEXTURE_SIZE,
        TEXTURE_SIZE,
        1,
        true
    );

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Error creando textura: 0x%08X", (unsigned)status);
        return false;
    }

    static uint32_t texels[TEXTURE_SIZE][TEXTURE_SIZE];

    for (uint32_t y = 0; y < TEXTURE_SIZE; y++) {
        for (uint32_t x = 0; x < TEXTURE_SIZE; x++) {
            texels[y][x] =
                (x / CHECK_TEXELS + y / CHECK_TEXELS) % 2
                ? 0xFF808080
                : 0xFFFFFFFF;
        }
    }

    xsdk_texture_upload(
        &cube->texture,
        0,
        texels,
        sizeof texels[0]
    );

    xsdk_gpu_invalidate(
        cube->texture.address,
        cube->texture.size_bytes
    );

    const XsdkSampler sampler = {
        .mag_filter = XSDK_FILTER_LINEAR,
        .min_filter = XSDK_FILTER_LINEAR,
        .mip_filter = XSDK_FILTER_POINT,
        .address_u = XSDK_ADDRESS_WRAP,
        .address_v = XSDK_ADDRESS_WRAP
    };

    xsdk_texture_sampler_state(&cube->sampler, &sampler);

    return true;
}

static bool make_cube(Cube *cube)
{
    NTSTATUS status = xsdk_shader_load(
        &cube->vertex,
        cube_vert.container,
        cube_vert.container_bytes
    );

    if (NT_SUCCESS(status)) {
        status = xsdk_shader_load(
            &cube->pixel,
            cube_frag.container,
            cube_frag.container_bytes
        );
    }

    if (NT_SUCCESS(status)) {
        status = xsdk_shader_set_vertex_layout(
            &cube->vertex,
            layout,
            sizeof layout / sizeof layout[0],
            (const uint32_t[]){ STRIDE_BYTES }
        );
    }

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Error cargando shaders: 0x%08X", (unsigned)status);
        return false;
    }

    const XsdkDepthStencil depth = {
        .depth_test = true,
        .depth_write = true,
        .depth_compare = XSDK_COMPARE_LESS_EQUAL
    };

    xsdk_gpu_depth_stencil_state(&cube->depth, &depth);

    return make_mesh(cube) && make_texture(cube);
}

static Matrix multiply(const Matrix *a, const Matrix *b)
{
    Matrix product;

    for (uint32_t column = 0; column < 4; column++) {
        for (uint32_t row = 0; row < 4; row++) {
            float sum = 0.0f;

            for (uint32_t k = 0; k < 4; k++) {
                sum += a->m[k * 4 + row] *
                       b->m[column * 4 + k];
            }

            product.m[column * 4 + row] = sum;
        }
    }

    return product;
}

static Matrix cube_matrix(float yaw, float pitch, float aspect)
{
    const float f = 1.0f / tanf(FIELD_OF_VIEW / 2.0f);
    const float fx = f / aspect;
    const float fz = FAR_PLANE / (NEAR_PLANE - FAR_PLANE);

    const Matrix perspective = {{
        fx, 0, 0, 0,
        0, f, 0, 0,
        0, 0, fz, -1,
        0, 0, NEAR_PLANE * fz, 0
    }};

    const Matrix away = {{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, -CAMERA_DISTANCE, 1
    }};

    const float cp = cosf(pitch);
    const float sp = sinf(pitch);

    const Matrix turn_x = {{
        1, 0, 0, 0,
        0, cp, sp, 0,
        0, -sp, cp, 0,
        0, 0, 0, 1
    }};

    const float cy = cosf(yaw);
    const float sy = sinf(yaw);

    const Matrix turn_y = {{
        cy, 0, -sy, 0,
        0, 1, 0, 0,
        sy, 0, cy, 0,
        0, 0, 0, 1
    }};

    const Matrix turned = multiply(&turn_x, &turn_y);
    const Matrix placed = multiply(&away, &turned);

    return multiply(&perspective, &placed);
}

static float stick(int16_t value)
{
    return value > STICK_DEAD_ZONE ||
           value < -STICK_DEAD_ZONE
           ? value / 32768.0f
           : 0.0f;
}

static void draw(
    const Cube *cube,
    const Matrix *matrix,
    uint32_t address,
    uint32_t width,
    uint32_t height)
{
    xsdk_gpu_set_target(width, height);
    xsdk_gpu_set_depth_stencil(&cube->depth);
    xsdk_gpu_set_shaders(&cube->vertex, &cube->pixel);

    xsdk_built_shader_set_literals(&cube_vert);
    xsdk_built_shader_set_literals(&cube_frag);

    xsdk_gpu_set_vertex_constants(
        CUBE_VERT_PUSH,
        matrix->m,
        CUBE_VERT_PUSH_REGISTERS
    );

    xsdk_gpu_set_texture(
        CUBE_FRAG_SAMPLER_0_0,
        &cube->texture,
        &cube->sampler
    );

    xsdk_gpu_set_vertices(
        0,
        xsdk_gpu_address(cube->vertices),
        VERTICES_BYTES
    );

    xsdk_gpu_draw_indexed(
        XSDK_PRIMITIVE_TRIANGLE_LIST,
        xsdk_gpu_address(cube->indices),
        INDICES,
        false
    );

    xsdk_gpu_resolve(
        address,
        width,
        height,
        CLEAR_ARGB
    );
}

static void run_game(
    const Cube *cube,
    uint32_t width,
    uint32_t height)
{
    float yaw = 0.0f;
    float pitch = 0.0f;

    for (;;) {
        INPUT_STATE input = { 0 };

        if (XamInputGetState(0, 0, &input) == ERROR_SUCCESS &&
            (input.gamepad.buttons & INPUT_GAMEPAD_BACK)) {
            break;
        }

        yaw += TURN_PER_FRAME * stick(input.gamepad.thumb_lx);
        pitch -= TURN_PER_FRAME * stick(input.gamepad.thumb_ly);

        uint32_t address;

        NTSTATUS status = xsdk_present_begin(
            &address,
            BEGIN_TIMEOUT_MS
        );

        if (!NT_SUCCESS(status)) {
            xsdk_log_write("Error preparando pantalla: 0x%08X", (unsigned)status);
            return;
        }

        const Matrix matrix = cube_matrix(
            yaw,
            pitch,
            (float)width / (float)height
        );

        draw(cube, &matrix, address, width, height);
        xsdk_present_end();
    }
}

int main(void)
{
    xsdk_log_write("THE LAST SURVIVOR: cubo 3D");

    NTSTATUS status = xsdk_splash_show(SPLASH_MS);

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Aviso: no se pudo mostrar el logo");
    }

    status = xsdk_gpu_open();

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Error iniciando GPU: 0x%08X", (unsigned)status);
        return 1;
    }

    static Cube cube;

    if (!make_cube(&cube)) {
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
        xsdk_log_write("Error abriendo pantalla: 0x%08X", (unsigned)status);
        return 1;
    }

    xsdk_log_write(
        "Pantalla lista: %u x %u",
        (unsigned)width,
        (unsigned)height
    );

    run_game(&cube, width, height);

    xsdk_present_close();

    xsdk_texture_free(&cube.texture);
    xsdk_gpu_free((void *)cube.indices);
    xsdk_gpu_free((void *)cube.vertices);

    xsdk_shader_free(&cube.pixel);
    xsdk_shader_free(&cube.vertex);

    status = xsdk_gpu_close();

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Error cerrando GPU: 0x%08X", (unsigned)status);
    }

    xsdk_log_write("THE LAST SURVIVOR: cubo 3D terminado");

    return 0;
}

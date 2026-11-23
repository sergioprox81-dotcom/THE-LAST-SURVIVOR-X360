
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
    SPLASH_MS = 1500,
    BUFFERS = 2,
    SWAP_INTERVAL = 1,
    BEGIN_TIMEOUT_MS = 5000,
    CLEAR_ARGB = 0xFF202020,
    BOX_COUNT = 8,
    FACES = 6,
    VERTICES_PER_BOX = 24,
    INDICES_PER_BOX = 36,
    VERTICES = BOX_COUNT * VERTICES_PER_BOX,
    INDICES = BOX_COUNT * INDICES_PER_BOX,
    VERTEX_FLOATS = 8,
    STRIDE_BYTES = VERTEX_FLOATS * 4,
    COLOR_OFFSET_BYTES = 12,
    UV_OFFSET_BYTES = 24,
    VERTICES_BYTES = VERTICES * STRIDE_BYTES,
    INDICES_BYTES = INDICES * 2,
    TEXTURE_SIZE = 8
};

static const float MOVE_SPEED = 0.045f;
static const float CAMERA_DISTANCE = 6.0f;
static const float FOV = 1.0f;
static const float NEAR_PLANE = 0.1f;
static const float FAR_PLANE = 100.0f;

static const float face_normals[FACES][3] = {
    { 1, 0, 0}, {-1, 0, 0},
    { 0, 1, 0}, { 0,-1, 0},
    { 0, 0, 1}, { 0, 0,-1}
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
} Game;

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

static void add_box(
    float vertices[VERTICES][VERTEX_FLOATS],
    uint16_t indices[INDICES],
    uint32_t box,
    float x, float y, float z,
    float sx, float sy, float sz,
    float r, float g, float b)
{
    static const float corners[4][2] = {
        {-1,-1}, {1,-1}, {-1,1}, {1,1}
    };

    static const uint16_t quad[6] = {
        0,1,2, 2,1,3
    };

    for (uint32_t face = 0; face < FACES; face++) {
        uint32_t base = box * VERTICES_PER_BOX + face * 4;

        for (uint32_t c = 0; c < 4; c++) {
            float *v = vertices[base + c];

            float a = corners[c][0];
            float d = corners[c][1];

            switch (face) {
                case 0:
                    v[0] = x + sx;
                    v[1] = y + d * sy;
                    v[2] = z + a * sz;
                    break;
                case 1:
                    v[0] = x - sx;
                    v[1] = y + d * sy;
                    v[2] = z - a * sz;
                    break;
                case 2:
                    v[0] = x + a * sx;
                    v[1] = y + sy;
                    v[2] = z - d * sz;
                    break;
                case 3:
                    v[0] = x + a * sx;
                    v[1] = y - sy;
                    v[2] = z + d * sz;
                    break;
                case 4:
                    v[0] = x + a * sx;
                    v[1] = y + d * sy;
                    v[2] = z + sz;
                    break;
                default:
                    v[0] = x - a * sx;
                    v[1] = y + d * sy;
                    v[2] = z - sz;
                    break;
            }

            v[3] = r;
            v[4] = g;
            v[5] = b;
            v[6] = c & 1 ? 1.0f : 0.0f;
            v[7] = c & 2 ? 0.0f : 1.0f;
        }

        uint16_t start = (uint16_t)base;

        for (uint32_t i = 0; i < 6; i++) {
            indices[box * INDICES_PER_BOX + face * 6 + i] =
                start + quad[i];
        }
    }
}

static void build_character(Game *game, float px, float pz)
{
    float vertices[VERTICES][VERTEX_FLOATS];
    uint16_t indices[INDICES];

    memset(vertices, 0, sizeof vertices);
    memset(indices, 0, sizeof indices);

    /* Suelo */
    add_box(vertices, indices, 0,
        -px, -0.18f, -pz,
        30.0f, 0.15f, 30.0f,
        0.22f, 0.24f, 0.22f);

    /* Torso */
    add_box(vertices, indices, 1,
        0, 1.15f, 0,
        0.30f, 0.48f, 0.18f,
        0.25f, 0.32f, 0.22f);

    /* Cabeza */
    add_box(vertices, indices, 2,
        0, 1.85f, 0,
        0.19f, 0.22f, 0.18f,
        0.72f, 0.52f, 0.35f);

    /* Brazo izquierdo */
    add_box(vertices, indices, 3,
        -0.39f, 1.18f, 0,
        0.11f, 0.40f, 0.12f,
        0.25f, 0.32f, 0.22f);

    /* Brazo derecho */
    add_box(vertices, indices, 4,
        0.39f, 1.18f, 0,
        0.11f, 0.40f, 0.12f,
        0.25f, 0.32f, 0.22f);

    /* Pierna izquierda */
    add_box(vertices, indices, 5,
        -0.16f, 0.48f, 0,
        0.13f, 0.38f, 0.14f,
        0.12f, 0.14f, 0.18f);

    /* Pierna derecha */
    add_box(vertices, indices, 6,
        0.16f, 0.48f, 0,
        0.13f, 0.38f, 0.14f,
        0.12f, 0.14f, 0.18f);

    /* Caja del escenario */
    add_box(vertices, indices, 7,
        2.5f - px, 0.45f, 3.0f - pz,
        0.65f, 0.45f, 0.65f,
        0.28f, 0.20f, 0.13f);

    write_gpu(game->vertices, vertices, VERTICES_BYTES);
    write_gpu(game->indices, indices, INDICES_BYTES);
}

static Matrix multiply(const Matrix *a, const Matrix *b)
{
    Matrix out;

    for (uint32_t c = 0; c < 4; c++) {
        for (uint32_t r = 0; r < 4; r++) {
            float sum = 0;

            for (uint32_t k = 0; k < 4; k++) {
                sum += a->m[k * 4 + r] *
                       b->m[c * 4 + k];
            }

            out.m[c * 4 + r] = sum;
        }
    }

    return out;
}

static Matrix camera_matrix(float aspect)
{
    float f = 1.0f / tanf(FOV * 0.5f);
    float q = FAR_PLANE / (NEAR_PLANE - FAR_PLANE);

    const Matrix projection = {{
        f / aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, q, -1,
        0, 0, NEAR_PLANE * q, 0
    }};

    const Matrix camera = {{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, -1.7f, -CAMERA_DISTANCE, 1
    }};

    return multiply(&projection, &camera);
}

static bool make_game(Game *game)
{
    NTSTATUS status = xsdk_shader_load(
        &game->vertex,
        cube_vert.container,
        cube_vert.container_bytes
    );

    if (NT_SUCCESS(status)) {
        status = xsdk_shader_load(
            &game->pixel,
            cube_frag.container,
            cube_frag.container_bytes
        );
    }

    if (NT_SUCCESS(status)) {
        status = xsdk_shader_set_vertex_layout(
            &game->vertex,
            layout,
            sizeof layout / sizeof layout[0],
            (const uint32_t[]){ STRIDE_BYTES }
        );
    }

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Error cargando shaders: 0x%08X",
            (unsigned)status);
        return false;
    }

    game->vertices = xsdk_gpu_alloc(VERTICES_BYTES);
    game->indices = xsdk_gpu_alloc(INDICES_BYTES);

    if (!game->vertices || !game->indices) {
        xsdk_log_write("Error reservando memoria 3D");
        return false;
    }

    NTSTATUS texture_status = xsdk_texture_create(
        &game->texture,
        XSDK_FORMAT_A8R8G8B8,
        TEXTURE_SIZE,
        TEXTURE_SIZE,
        1,
        true
    );

    if (!NT_SUCCESS(texture_status)) {
        xsdk_log_write("Error creando textura");
        return false;
    }

    static uint32_t texels[TEXTURE_SIZE][TEXTURE_SIZE];

    for (uint32_t y = 0; y < TEXTURE_SIZE; y++) {
        for (uint32_t x = 0; x < TEXTURE_SIZE; x++) {
            texels[y][x] = 0xFFFFFFFF;
        }
    }

    xsdk_texture_upload(
        &game->texture, 0, texels, sizeof texels[0]);

    xsdk_gpu_invalidate(
        game->texture.address,
        game->texture.size_bytes
    );

    const XsdkSampler sampler = {
        .mag_filter = XSDK_FILTER_LINEAR,
        .min_filter = XSDK_FILTER_LINEAR,
        .mip_filter = XSDK_FILTER_POINT,
        .address_u = XSDK_ADDRESS_WRAP,
        .address_v = XSDK_ADDRESS_WRAP
    };

    xsdk_texture_sampler_state(&game->sampler, &sampler);

    const XsdkDepthStencil depth = {
        .depth_test = true,
        .depth_write = true,
        .depth_compare = XSDK_COMPARE_LESS_EQUAL
    };

    xsdk_gpu_depth_stencil_state(&game->depth, &depth);

    return true;
}

static void draw_game(
    Game *game,
    const Matrix *matrix,
    uint32_t address,
    uint32_t width,
    uint32_t height)
{
    xsdk_gpu_set_target(width, height);
    xsdk_gpu_set_depth_stencil(&game->depth);
    xsdk_gpu_set_shaders(&game->vertex, &game->pixel);

    xsdk_built_shader_set_literals(&cube_vert);
    xsdk_built_shader_set_literals(&cube_frag);

    xsdk_gpu_set_vertex_constants(
        CUBE_VERT_PUSH,
        matrix->m,
        CUBE_VERT_PUSH_REGISTERS
    );

    xsdk_gpu_set_texture(
        CUBE_FRAG_SAMPLER_0_0,
        &game->texture,
        &game->sampler
    );

    xsdk_gpu_set_vertices(
        0,
        xsdk_gpu_address(game->vertices),
        VERTICES_BYTES
    );

    xsdk_gpu_draw_indexed(
        XSDK_PRIMITIVE_TRIANGLE_LIST,
        xsdk_gpu_address(game->indices),
        INDICES,
        false
    );

    xsdk_gpu_resolve(address, width, height, CLEAR_ARGB);
}

static void run_game(Game *game, uint32_t width, uint32_t height)
{
    float player_x = 0.0f;
    float player_z = 0.0f;

    const Matrix camera = camera_matrix(
        (float)width / (float)height);

    for (;;) {
        INPUT_STATE input = {0};

        if (XamInputGetState(0, 0, &input) == ERROR_SUCCESS) {
            if (input.gamepad.buttons & INPUT_GAMEPAD_BACK) {
                break;
            }

            float x = input.gamepad.thumb_lx / 32768.0f;
            float z = input.gamepad.thumb_ly / 32768.0f;

            if (x > 0.20f || x < -0.20f) {
                player_x += x * MOVE_SPEED;
            }

            if (z > 0.20f || z < -0.20f) {
                player_z += z * MOVE_SPEED;
            }
        }

        build_character(game, player_x, player_z);

        uint32_t address;

        NTSTATUS status = xsdk_present_begin(
            &address, BEGIN_TIMEOUT_MS);

        if (!NT_SUCCESS(status)) {
            xsdk_log_write("Error preparando pantalla");
            return;
        }

        draw_game(
            game, &camera, address, width, height);

        xsdk_present_end();
    }
}

int main(void)
{
    xsdk_log_write("THE LAST SURVIVOR: protagonista 3D");

    NTSTATUS status = xsdk_splash_show(SPLASH_MS);

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Aviso: no se pudo mostrar el logo");
    }

    status = xsdk_gpu_open();

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Error iniciando GPU");
        return 1;
    }

    static Game game;

    if (!make_game(&game)) {
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
        xsdk_log_write("Error abriendo pantalla");
        return 1;
    }

    xsdk_log_write("Protagonista 3D iniciado");

    run_game(&game, width, height);

    xsdk_present_close();

    xsdk_texture_free(&game.texture);
    xsdk_gpu_free((void *)game.indices);
    xsdk_gpu_free((void *)game.vertices);
    xsdk_shader_free(&game.pixel);
    xsdk_shader_free(&game.vertex);

    xsdk_gpu_close();

    xsdk_log_write("THE LAST SURVIVOR terminado");

    return 0;
}

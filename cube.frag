
#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_color;
layout(location = 2) in vec3 v_position;

layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 0) uniform sampler2D u_texture;

void main()
{
    /* Textura procedural: grano de hormigon y vetas de madera. */
    vec3 p = v_position * 2.7;
    float grain = sin(p.x * 17.0 + sin(p.z * 9.0)) *
                  sin(p.y * 13.0 + p.z * 7.0);
    float speckle = fract(sin(dot(floor(p * 3.0),
                                  vec3(12.9898, 78.233, 37.719))) *
                          43758.5453);
    float texture_detail = 0.91 + grain * 0.035 + speckle * 0.07;

    /* Linterna alrededor del protagonista. */
    float distance_to_player = distance(v_position, vec3(0.0, 1.35, 0.0));
    float flashlight = 1.0 - smoothstep(1.8, 7.5, distance_to_player);
    float ambient = 0.16;
    float lighting = ambient + flashlight * 0.88;

    vec3 surface = v_color * texture(u_texture, v_uv).rgb;
    surface *= texture_detail;
    surface *= lighting;

    /* Tono frio de survival horror. */
    surface *= vec3(0.78, 0.86, 1.0);
    o_color = vec4(surface, 1.0);
}

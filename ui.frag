
#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_color;

layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 0) uniform sampler2D u_texture;

void main()
{
    float alpha = texture(u_texture, v_uv).a;
    o_color = vec4(v_color, alpha);
}

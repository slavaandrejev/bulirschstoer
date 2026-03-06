#version 450 core

#extension GL_EXT_geometry_shader : enable
#extension GL_OES_geometry_shader : enable

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

uniform mat4  transform;
uniform float radius;

out vec2 local_pos;

void main()
{
    vec2 center = gl_in[0].gl_Position.xy;

    local_pos = vec2(1.0, 1.0);
    gl_Position = transform * vec4(center + vec2(radius, radius), 0.0, 1.0);
    EmitVertex();

    local_pos = vec2(1.0, -1.0);
    gl_Position = transform * vec4(center + vec2(radius, -radius), 0.0, 1.0);
    EmitVertex();

    local_pos = vec2(-1.0, 1.0);
    gl_Position = transform * vec4(center + vec2(-radius, radius), 0.0, 1.0);
    EmitVertex();

    local_pos = vec2(-1.0, -1.0);
    gl_Position = transform * vec4(center + vec2(-radius, -radius), 0.0, 1.0);
    EmitVertex();

    EndPrimitive();
}
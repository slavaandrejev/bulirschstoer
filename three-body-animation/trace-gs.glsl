#version 450 core

#extension GL_EXT_geometry_shader : enable
#extension GL_OES_geometry_shader : enable

layout (lines_adjacency) in;
layout (triangle_strip, max_vertices = 4) out;

uniform mat4  transform;
uniform float half_thickness;

in float vs_birth_time[];
out float gs_birth_time;
out float gs_edge_dist;

void main()
{
    vec2 p0 = gl_in[0].gl_Position.xy; // previous
    vec2 p1 = gl_in[1].gl_Position.xy; // start of current segment
    vec2 p2 = gl_in[2].gl_Position.xy; // end of current segment
    vec2 p3 = gl_in[3].gl_Position.xy; // next

    vec2 dir1 = normalize(p2 - p1);
    vec2 n1   = vec2(-dir1.y, dir1.x);

    vec2 miter1;
    float length1;
    if (distance(p1, p0) < 1e-5) {
        // Start cap detected: draw a flat, square end
        miter1 = n1;
        length1 = half_thickness;
    } else {
        // Normal miter joint
        vec2 dir0 = normalize(p1 - p0);
        miter1 = normalize(vec2(-dir0.y, dir0.x) + n1);
        float dot1 = dot(miter1, n1);
        length1 = half_thickness / max(dot1, 0.1);
    }

    vec2 miter2;
    float length2;
    if (distance(p3, p2) < 1e-5) {
        // End cap detected: draw a flat, square end
        miter2 = n1;
        length2 = half_thickness;
    } else {
        // Normal miter joint
        vec2 dir2 = normalize(p3 - p2);
        miter2 = normalize(n1 + vec2(-dir2.y, dir2.x));
        float dot2 = dot(miter2, n1);
        length2 = half_thickness / max(dot2, 0.1);
    }

    gs_edge_dist = 1.0;
    gs_birth_time = vs_birth_time[1];
    gl_Position = transform * vec4(p1 + miter1 * length1, 0.0, 1.0);
    EmitVertex();

    gs_edge_dist = -1.0;
    gl_Position = transform * vec4(p1 - miter1 * length1, 0.0, 1.0);
    EmitVertex();

    gs_edge_dist = 1.0;
    gs_birth_time = vs_birth_time[2];
    gl_Position = transform * vec4(p2 + miter2 * length2, 0.0, 1.0);
    EmitVertex();

    gs_edge_dist = -1.0;
    gl_Position = transform * vec4(p2 - miter2 * length2, 0.0, 1.0);
    EmitVertex();

    EndPrimitive();
}

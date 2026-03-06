#version 450 core

#extension GL_OES_standard_derivatives : enable

in vec2 local_pos;

out vec4 FragColor;

uniform vec3  ball_color;
uniform vec3  outline_color;
uniform float outline_thickness;

void main()
{
    // Distance from the center of the ball
    float dist = length(local_pos);

    // fwidth gives us hardware-level anti-aliasing so the edges
    // are perfectly smooth, never pixelated.
    float delta = fwidth(dist);

    // 1. Calculate the color (Mix fill color with outline color)
    float outline_edge = 1.0 - outline_thickness;
    float outline_mask = smoothstep(outline_edge - delta, outline_edge, dist);
    vec3 color = mix(ball_color, outline_color, outline_mask);

    // 2. Calculate the transparency (Clip the square into a circle)
    float alpha = 1.0 - smoothstep(1.0 - delta, 1.0, dist);

    FragColor = vec4(color, alpha);
}

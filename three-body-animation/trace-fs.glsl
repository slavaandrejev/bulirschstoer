#version 450 core

#extension GL_OES_standard_derivatives : enable

out vec4 FragColor;

in float gs_birth_time;
in float gs_edge_dist;

uniform vec3  line_color;
uniform float current_time;
uniform float fade_duration;

void main()
{
    // fading with time
    float age = current_time - gs_birth_time;
    float age_alpha = 1.0 - (age / fade_duration);
    age_alpha = clamp(age_alpha, 0.0, 1.0);

    // anti-aliasing
    float dist     = abs(gs_edge_dist); // 0.0 at center, 1.0 at edges
    float delta    = length(vec2(dFdx(dist), dFdy(dist)));
    float aa_alpha = 1.0 - smoothstep(1.0 - delta, 1.0, dist);

    FragColor = vec4(line_color, age_alpha * aa_alpha);
}

#version 450 core

layout (location = 0) in vec2  aPos;
layout (location = 1) in float birth_time;

out float vs_birth_time;

void main()
{
    gl_Position = vec4(aPos, 0.0f, 1.0f);
    vs_birth_time = birth_time;
}

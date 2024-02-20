#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 projection;
uniform mat4 transform;
uniform vec2 velocity; // New uniform for velocity

void main()
{
    gl_Position = transform * projection * vec4(sin(aPos.x), sin(aPos.y), 0.0, 1.0);
}

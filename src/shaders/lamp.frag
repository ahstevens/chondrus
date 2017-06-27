#version 330 core
out vec4 color;

uniform vec3 col;

void main()
{
    color = vec4(col.r, col.g, col.b, 1.0f);
}
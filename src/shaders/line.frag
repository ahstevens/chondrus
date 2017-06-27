#version 330 core
in vec3 col;
out vec4 color;

void main()
{
    color = vec4(col.r, col.g, col.b, 1.0f);
}
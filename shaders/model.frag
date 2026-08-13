#version 330 core

in vec3 fragPos;
in vec3 normal;
in vec2 uv;

out vec4 color;

uniform sampler2D tex;

void main() {
    color = texture(tex, uv);
}
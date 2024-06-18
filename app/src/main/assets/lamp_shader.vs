#version 300 es
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 inUV;

out vec2 fragUV;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(position, 1.0f);
    fragUV = vec2(inUV.x, inUV.y);
}
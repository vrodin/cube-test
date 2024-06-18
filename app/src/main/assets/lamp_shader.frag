#version 300 es
precision mediump float;
in vec2 fragUV;

out vec4 color;

uniform sampler2D texture1;

void main()
{
    color = texture(texture1, fragUV);
}
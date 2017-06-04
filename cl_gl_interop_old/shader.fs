#version 330 core

in vec2 uv;

uniform sampler2D rayimg;

out vec4 color;

void main()
{
  color = texture(rayimg, uv);
}


#version 330 core

in vec4 vert;

uniform mat4 projection;
uniform mat4 cam_inv;


void main(void)
{
  mat4 cam_to_ndc = mat4(
    vec4(0.5/1920, 0, 0, 0),
    vec4(0, 0.5/1080, 0, 0),
    vec4(0,0,1,0),
    vec4(0,0,0,1));
  gl_Position = cam_to_ndc * cam_inv * vert;
}


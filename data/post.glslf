#version 300 es
precision mediump float;

in vec2 screen_coord;
uniform sampler2D colour;
uniform sampler2D depth;

layout (location = 0) out vec4 out_colour;

void main(void) {
  out_colour = texture(depth, screen_coord);
}

#version 300 es
precision mediump float;

in vec4 vColour;
in vec3 vNormal;

layout (location = 0) out vec4 colour0;
//layout (location = 1) out vec4 colour1;

void main(void) {
  colour0 = vColour + vec4(vNormal, 1.0) * 0.5;
}

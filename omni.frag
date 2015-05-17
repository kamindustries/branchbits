#version 120

uniform float lighting;
uniform float texture;
uniform sampler2D texture0;

varying vec4 fcolor;
// varying vec3 fnormal, flightDir, feyeVec;

// varying vec4 color;
// varying vec3 normal, lightDir, eyeVec;

void main() {
  // yes, doing some dumb thing to use 'texture' and 'lighting'
  // they should be in omnin context.
  // but they will always be 0

  vec4 colorMixed;
  if (texture > 0.0) {
    vec4 textureColor = texture2D(texture0, gl_TexCoord[0].st);
    // colorMixed = mix(color, textureColor, texture);
    colorMixed = mix(fcolor, textureColor, texture);
  } else {
    // colorMixed = color;
    colorMixed = fcolor;
  }

  gl_FragColor = mix(fcolor, colorMixed, lighting);
}
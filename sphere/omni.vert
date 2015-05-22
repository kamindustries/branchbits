#version 120

// @omni_eye: the eye parallax distance.
//  This will be zero for mono, and positive/negative for right/left
// eyes.
//  Pass this uniform to the shader in the OmniStereoDrawable
// callback
uniform float omni_eye;

// @omni_radius: the radius of the allosphere in OpenGL units.
//  This will be infinity for the original layout (we default to 1e10).
uniform float omni_radius;

// @omni_face: the GL_TEXTURE_CUBE_MAP face being rendered.
//  For a typical forward-facing view, this should == 5.
//  Pass this uniform to the shader in the OmniStereoDrawable
// callback
uniform int omni_face;

// @omni_near: the near clipping plane.
uniform float omni_near;

// @omni_far: the far clipping plane.
uniform float omni_far;

// omni_render(vertex)
// @vertex: the eye-space vertex to be rendered.
//  Typically gl_Position = omni_render(gl_ModelViewMatrix *
// gl_Vertex);
vec4 omni_render(in vec4 vertex) {
  float l = length(vertex.xz);
  vec3 vn = normalize(vertex.xyz);
  // Precise formula.
  float displacement = omni_eye *
    (omni_radius * omni_radius -
       sqrt(l * l * omni_radius * omni_radius +
            omni_eye * omni_eye * (omni_radius * omni_radius - l * l))) /
    (omni_radius * omni_radius - omni_eye * omni_eye);
  // Approximation, safe if omni_radius / omni_eye is very large, which is true for the allosphere.
  // float displacement = omni_eye * (1.0 - l / omni_radius);
  // Displace vertex.
  vertex.xz += vec2(displacement * vn.z, displacement * -vn.x);

  // convert eye-space into cubemap-space:
  // GL_TEXTURE_CUBE_MAP_POSITIVE_X
  if (omni_face == 0) {
    vertex.xyz = vec3(-vertex.z, -vertex.y, -vertex.x);
  }
      // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
      else if (omni_face == 1) {
    vertex.xyz = vec3(vertex.z, -vertex.y, vertex.x);
  }
      // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
      else if (omni_face == 2) {
    vertex.xyz = vec3(vertex.x, vertex.z, -vertex.y);
  }
      // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
      else if (omni_face == 3) {
    vertex.xyz = vec3(vertex.x, -vertex.z, vertex.y);
  }
      // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
      else if (omni_face == 4) {
    vertex.xyz = vec3(vertex.x, -vertex.y, -vertex.z);
  }
      // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
      else {
    vertex.xyz = vec3(-vertex.x, -vertex.y, vertex.z);
  }
  // convert into screen-space:
  // simplified perspective projection since fovy = 90 and aspect = 1
  vertex.zw = vec2((vertex.z * (omni_far + omni_near) +
                    vertex.w * omni_far * omni_near * 2.) /
                       (omni_near - omni_far),
                   -vertex.z);
  return vertex;
}

// varying vec4 color;
// varying vec3 normal, lightDir; //, eyeVec;

void main() {
  vec4 Cd = gl_Color;
  // Cd.g = 1. - (gl_Vertex.z * 0.00025)+0.1;

  // color = Cd;
  vec4 vertex = gl_ModelViewMatrix * gl_Vertex;


  gl_FrontColor = Cd;
  gl_BackColor = Cd;
  
  // normal = gl_NormalMatrix * gl_Normal;
  // vec3 V = vertex.xyz;
  // eyeVec = normalize(-V);
  // lightDir = normalize(vec3(gl_LightSource[0].position.xyz - V));
  gl_TexCoord[0] = gl_MultiTexCoord0;
  gl_Position = omni_render(vertex);
}
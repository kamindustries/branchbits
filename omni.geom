#version 120
#extension GL_EXT_geometry_shader4 : enable // it is extension in 120

varying in vec4 color[];
// varying in vec3 normal[], lightDir[], eyeVec[];

varying out vec4 fcolor;
// varying out vec3 fnormal, flightDir;//, feyeVec;

// dividing 2PI by 8, 9 values for rotating fully
uniform float sin_lkup[] = float[](
  0.00000000,
  0.70710678,
  1.00000000,
  0.70710678,
  0.00000000,
  -0.70710678,
  -1.00000000,
  -0.70710678,
  0.00000000
);

uniform float cos_lkup[] = float[](
  1.00000000,
  0.70710678,
  0.00000000,
  -0.70710678,
  -1.00000000,
  -0.70710678,
  0.00000000,
  0.70710678,
  1.00000000
);

void main(){
  vec3 dir = gl_PositionIn[0].xyz - gl_PositionIn[1].xyz;
  // arbitrary dir to gen cross product
  vec3 other_dir = vec3(-1.2,1.7,-2.4) - gl_PositionIn[1].xyz;

  vec3 axis1 = normalize(cross(dir, other_dir));
  vec3 axis2 = normalize(cross(dir, axis1));
  
  // inlcude i == 8 for closing the loop
  for (int i = 0; i <= 8; i++) {
    for (int j = 0; j < 2; j++) {
      vec4 p = vec4(axis1 * cos_lkup[i] + axis2 * sin_lkup[i], 0.5);

      fcolor = color[j];
      gl_Position = gl_PositionIn[j] + p;
      EmitVertex();
    }
  }

  EndPrimitive();
}
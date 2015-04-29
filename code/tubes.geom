#version 120
#extension GL_EXT_geometry_shader4 : enable // it is extension in 120

uniform float spriteRadius;

float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main(){
  vec4 v0 = vec4(gl_PositionIn[0].xyz, 0.5);
  vec4 v1 = vec4(gl_PositionIn[1].xyz, 0.5);

  vec3 rand_dir = vec3(-1.,1.,-2);

  vec3 axis1 = normalize(cross(v0.xyz - v1.xyz, rand_dir)) * spriteRadius;
  vec3 axis2 = normalize(cross(v0.xyz - v1.xyz, axis1)) * spriteRadius;


  //screen-aligned axes
  // vec3 axis1 = vec3(  gl_ModelViewMatrix[0][0],
  //     gl_ModelViewMatrix[1][0],
  //     gl_ModelViewMatrix[2][0]) * spriteRadius * .5;

  // vec3 axis2 = vec3(  gl_ModelViewMatrix[0][1],
  //     gl_ModelViewMatrix[1][1],
  //     gl_ModelViewMatrix[2][1]) * spriteRadius * .5;

  vec4 p0 = vec4(-axis1 - axis2, 0.5);
  vec4 p1 = vec4( axis1 - axis2, 0.5);
  vec4 p2 = vec4( axis1 + axis2, 0.5);
  vec4 p3 = vec4(-axis1 + axis2, 0.5);

  gl_FrontColor = gl_FrontColorIn[0];
  gl_Position = gl_ModelViewProjectionMatrix * (v0 + p0);
  EmitVertex();
  gl_FrontColor = gl_FrontColorIn[1];
  gl_Position = gl_ModelViewProjectionMatrix * (v1 + p0);
  EmitVertex();

  gl_FrontColor = gl_FrontColorIn[0];
  gl_Position = gl_ModelViewProjectionMatrix * (v0 + p1);
  EmitVertex();
  gl_FrontColor = gl_FrontColorIn[1];
  gl_Position = gl_ModelViewProjectionMatrix * (v1 + p1);
  EmitVertex();

  gl_FrontColor = gl_FrontColorIn[0];
  gl_Position = gl_ModelViewProjectionMatrix * (v0 + p2);
  EmitVertex();
  gl_FrontColor = gl_FrontColorIn[1];
  gl_Position = gl_ModelViewProjectionMatrix * (v1 + p2);
  EmitVertex();

  gl_FrontColor = gl_FrontColorIn[0];
  gl_Position = gl_ModelViewProjectionMatrix * (v0 + p3);
  EmitVertex();
  gl_FrontColor = gl_FrontColorIn[1];
  gl_Position = gl_ModelViewProjectionMatrix * (v1 + p3);
  EmitVertex();

  gl_FrontColor = gl_FrontColorIn[0];
  gl_Position = gl_ModelViewProjectionMatrix * (v0 + p0);
  EmitVertex();
  gl_FrontColor = gl_FrontColorIn[1];
  gl_Position = gl_ModelViewProjectionMatrix * (v1 + p0);
  EmitVertex();

  EndPrimitive();

  // for(int i = 0; i < gl_VerticesIn; ++i){
  //   // copy color
  //   gl_FrontColor = gl_FrontColorIn[i];

  //   vec4 p = gl_ModelViewProjectionMatrix * vec4(gl_PositionIn[i].xyz, 0.5);

  //   gl_TexCoord[0] = vec4(0, 0, 0, 1);
  //   gl_Position =  p + pxy;
  //   EmitVertex();

  //   gl_TexCoord[0] = vec4(1, 0, 0, 1);
  //   gl_Position =  p + pXy;
  //   EmitVertex();

  //   gl_TexCoord[0] = vec4(0, 1, 0, 1);
  //   gl_Position =  p + pxY;
  //   EmitVertex();

  //   gl_TexCoord[0] = vec4(1, 1, 0, 1);
  //   gl_Position =  p + pXY;
  //   EmitVertex();

  //   EndPrimitive();
  // }

  //  EndPrimitive();
  // gl_Position = gl_PositionIn[0];
}
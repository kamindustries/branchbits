#version 120
#extension GL_EXT_geometry_shader4 : enable

uniform float spriteRadius;

float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main(){
  vec4 v0 = gl_ModelViewProjectionMatrix * vec4(gl_PositionIn[0].xyz, 0.5);
  vec4 v1 = gl_ModelViewProjectionMatrix * vec4(gl_PositionIn[1].xyz, 0.5);

  vec3 rand_dir = vec3(0.,1.,0);

  vec3 axis1 = normalize(cross(v0.xyz, rand_dir)) * spriteRadius * .5;
  vec3 axis2 = normalize(cross(v0.xyz, axis1)) * spriteRadius * .5;


  //screen-aligned axes
  // vec3 axis1 = vec3(  gl_ModelViewMatrix[0][0],
  //     gl_ModelViewMatrix[1][0],
  //     gl_ModelViewMatrix[2][0]) * spriteRadius * .5;

  // vec3 axis2 = vec3(  gl_ModelViewMatrix[0][1],
  //     gl_ModelViewMatrix[1][1],
  //     gl_ModelViewMatrix[2][1]) * spriteRadius * .5;

  // vec3 axis1_2 = vec3(  gl_ModelViewMatrix[0][0],
  //     gl_ModelViewMatrix[1][0],
  //     gl_ModelViewMatrix[2][0]) * spriteRadius * .6;

  // vec3 axis2_2 = vec3(  gl_ModelViewMatrix[0][1],
  //     gl_ModelViewMatrix[1][1],
  //     gl_ModelViewMatrix[2][1]) * spriteRadius * .6;

  // gl_ModelViewProjectionMatrix
  vec4 p0 = /*gl_ModelViewProjectionMatrix * */vec4(-axis1 - axis2, 0.5);
  vec4 p1 = /*gl_ModelViewProjectionMatrix * */vec4( axis1 - axis2, 0.5);
  vec4 p2 = /*gl_ModelViewProjectionMatrix * */vec4( axis1 + axis2, 0.5);
  vec4 p3 = /*gl_ModelViewProjectionMatrix * */vec4(-axis1 + axis2, 0.5);


  // // halo points 
  // vec4 px0 = gl_ModelViewProjectionMatrix * (vec4(-axis1_2 - axis2_2, 0.5));
  // vec4 pX0 = gl_ModelViewProjectionMatrix * (vec4( axis1_2 - axis2_2, 0.5));
  // vec4 px0 = gl_ModelViewProjectionMatrix * (vec4(-axis1_2 + axis2_2, 0.5));
  // vec4 pX0 = gl_ModelViewProjectionMatrix * (vec4( axis1_2 + axis2_2, 0.5));

  // for(int i = 0; i < gl_VerticesIn; ++i){
  // copy color
  gl_FrontColor = gl_FrontColorIn[0];



  vec4 push_back = vec4(0,0,.01,0);

  
  gl_TexCoord[0] = vec4(0, 0, 0, 1);
  gl_Position =  v0 + p0;
  EmitVertex();
  gl_TexCoord[0] = vec4(0, 0, 0, 1);
  gl_Position =  v1 + p0;
  EmitVertex();

  gl_TexCoord[0] = vec4(1, 0, 0, 1);
  gl_Position =  v0 + p1;
  EmitVertex();
  gl_TexCoord[0] = vec4(1, 0, 0, 1);
  gl_Position =  v1 + p1;
  EmitVertex();

  gl_TexCoord[0] = vec4(0, 1, 0, 1);
  gl_Position =  v0 + p2;
  EmitVertex();
  gl_TexCoord[0] = vec4(0, 1, 0, 1);
  gl_Position =  v1 + p2;
  EmitVertex();

  gl_TexCoord[0] = vec4(1, 1, 0, 1);
  gl_Position =  v0 + p3;
  EmitVertex();
  gl_TexCoord[0] = vec4(1, 1, 0, 1);
  gl_Position =  v1 + p3;
  EmitVertex();

  gl_TexCoord[0] = vec4(1, 1, 0, 1);
  gl_Position =  v0 + p0;
  EmitVertex();
  gl_TexCoord[0] = vec4(1, 1, 0, 1);
  gl_Position =  v1 + p0;
  EmitVertex();

  EndPrimitive();
  
  // DRAW HALO

  // gl_FrontColor = gl_FrontColorIn[i]*10.;

  // gl_TexCoord[0] = vec4(0, 0, 0, 1);
  // gl_Position =  p + pxy2 + push_back;
  // EmitVertex();

  // gl_TexCoord[0] = vec4(1, 0, 0, 1);
  // gl_Position =  p + pXy2 + push_back;
  // EmitVertex();

  // gl_TexCoord[0] = vec4(0, 1, 0, 1);
  // gl_Position =  p + pxY2 + push_back;
  // EmitVertex();

  // gl_TexCoord[0] = vec4(1, 1, 0, 1);
  // gl_Position =  p + pXY2 + push_back;
  // EmitVertex();

  // for (int i = 0; i < 20; i++){
  //  gl_TexCoord[0] = vec4(1, 1, 0, 1);

  // // RANDOM POINTS
  //  float i_f = float(i);

  //  float rand1 = rand(vec2(i_f,i_f+1.))*spriteRadius * 1.;
  //  float rand2 = rand(vec2(i_f+2.,i_f+3.))*spriteRadius * 1.;
  //  float rand3 = rand(vec2(i_f+4.,i_f+5.))*spriteRadius * 1.;
  //  gl_Position = p + vec4(rand1,rand2,rand3,0.5);
  //  EmitVertex();
  // }

  // EndPrimitive();
  // }

//  EndPrimitive();
  // gl_Position = gl_PositionIn[0];
}

void main(){
  vec4 Cd = gl_Color;
  Cd.g = 1. - (gl_Vertex.z * 0.00045)+0.0;
  // Cd.g = (pow(gl_Vertex.z, 1.2) * .00005)*.4;

  gl_FrontColor = Cd;
  gl_Position = gl_Vertex;
  // gl_FrontColor = gl_Color;
}

void main(){
  vec4 Cd = gl_Color; 
  // control depth coloring
  // Cd.g = .2;
  // Cd.g = 1. - (gl_Vertex.z * 0.00065)+.01;
  // Cd.g = (pow(gl_Vertex.z, 1.2) * .5)*.4;
  gl_FrontColor = Cd;
  gl_Position = gl_Vertex;
}
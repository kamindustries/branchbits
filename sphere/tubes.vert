void main(){
  vec4 Cd = gl_Color; 
  gl_FrontColor = Cd;
  gl_Position = gl_Vertex;
}
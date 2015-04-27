uniform sampler2D texSampler0;

void main(){
  // gl_FragColor = texture2D(texSampler0, gl_TexCoord[0].xy) * gl_Color;
  gl_FragColor = gl_Color;
}
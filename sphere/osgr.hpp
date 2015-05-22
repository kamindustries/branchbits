#ifndef AL_OMNI_STEREO_GRAPHICS_RENDERER_H
#define AL_OMNI_STEREO_GRAPHICS_RENDERER_H

#include "allocore/al_Allocore.hpp"
#include "allocore/io/al_Window.hpp"
#include "allocore/protocol/al_OSC.hpp"
#include "alloutil/al_FPS.hpp"
#include "alloutil/al_OmniStereo.hpp"
#include "alloutil/al_Simulator.hpp"  // DEVICE_SERVER_PORT_CONNECTION_US

namespace al {

/*----------------------------------------------------------------------------*\
   COPY of OmniStereoGrphicsRenderer.
   Parts that are only different are noted
\*----------------------------------------------------------------------------*/
class OmniStereoGraphicsRenderer1 : public Window,
                                    public FPS,
                                    public OmniStereo::Drawable {
 public:
  OmniStereoGraphicsRenderer1();
  virtual ~OmniStereoGraphicsRenderer1();

  virtual void onDraw(Graphics& gl) {}
  virtual void onAnimate(al_sec dt) {}
  virtual bool onCreate();
  virtual bool onFrame();
  virtual void onDrawOmni(OmniStereo& omni);
  virtual std::string vertexCode();
  virtual std::string fragmentCode();
// -----------------------------------------------------------------------------
  virtual std::string geometryCode();
// -----------------------------------------------------------------------------

//   virtual const char* vertexCodeCh();
//   virtual const char* fragmentCodeCh();
// // -------const char*-----------------------------------------------------------
//   virtual const char* geometryCodeCh();
// -----------------------------------------------------------------------------

  void start();
  void initWindow(const Window::Dim& dims = Window::Dim(800, 400),
                  const std::string title = "OmniStereoGraphicsRenderer1",
                  double fps = 60,
                  Window::DisplayMode mode = Window::DEFAULT_BUF);
  void initOmni(std::string path = "");

  const Lens& lens() const { return mLens; }
  Lens& lens() { return mLens; }

  const Graphics& graphics() const { return mGraphics; }
  Graphics& graphics() { return mGraphics; }

  ShaderProgram& shader() { return mShader; }

  OmniStereo& omni() { return mOmni; }

  const std::string& hostName() const { return mHostName; }

  bool omniEnable() const { return bOmniEnable; }
  void omniEnable(bool b) { bOmniEnable = b; }

  osc::Send& oscSend() { return mOSCSend; }

 protected:
  const Nav& nav() const { return mNav; }
  Nav& nav() { return mNav; }

  OmniStereo mOmni;

  Lens mLens;
  Graphics mGraphics;

  osc::Send mOSCSend;
  Pose pose;

  ShaderProgram mShader;

  std::string mHostName;

  bool bOmniEnable;

  Nav mNav;
  NavInputControl mNavControl;
  StandardWindowKeyControls mStdControls;
};

inline void OmniStereoGraphicsRenderer1::start() {
  if (mOmni.activeStereo()) {
    Window::displayMode(Window::displayMode() | Window::STEREO_BUF);
  }

  create();

  if (mOmni.fullScreen()) {
    fullScreen(true);
    cursorHide(true);
  }

  Main::get().start();
}

inline OmniStereoGraphicsRenderer1::~OmniStereoGraphicsRenderer1() {}

inline OmniStereoGraphicsRenderer1::OmniStereoGraphicsRenderer1()
    : mNavControl(mNav), mOSCSend(12001) {

  bOmniEnable = true;
  mHostName = Socket::hostName();

  lens().near(0.01).far(40).eyeSep(0.03);
  nav().smooth(0.8);

  initWindow();
  initOmni();

  Window::append(mStdControls);
  Window::append(mNavControl);
}

inline void OmniStereoGraphicsRenderer1::initOmni(std::string path) {
  mOmni.configure(path, mHostName);
  if (mOmni.activeStereo()) {
    mOmni.mode(OmniStereo::ACTIVE).stereo(true);
  }
}

inline void OmniStereoGraphicsRenderer1::initWindow(const Window::Dim& dims,
                                                   const std::string title,
                                                   double fps,
                                                   Window::DisplayMode mode) {
  Window::dimensions(dims);
  Window::title(title);
  Window::fps(fps);
  Window::displayMode(mode);
}

inline bool OmniStereoGraphicsRenderer1::onCreate() {
  mOmni.onCreate();
  cout << "after mOmni.onCreate()" << endl;
  
  /*
  Shader vert, frag;
  std::string versionCode = "#version 120\n";
  vert.source(versionCode + OmniStereo::glsl() + vertexCode(), Shader::VERTEX).compile();
  vert.printLog();
  frag.source(versionCode + fragmentCode(), Shader::FRAGMENT).compile();
  frag.printLog();

// -----------------------------------------------------------------------------
  Shader geom;
  geom.source(geometryCode(), Shader::GEOMETRY).compile();
  geom.printLog();
// -----------------------------------------------------------------------------

  // mShader.attach(vert).attach(frag).link();
  mShader.attach(vert).attach(frag).attach(geom).link();
  mShader.printLog();
  */

  Shader shaderV, shaderF, shaderG;

  // in-code shader
  // shaderV.source(vertexCodeCh(), Shader::VERTEX).compile().printLog();
  // shaderF.source(fragmentCodeCh(), Shader::FRAGMENT).compile().printLog();
  // shaderG.source(geometryCodeCh(), Shader::GEOMETRY).compile().printLog();

  SearchPaths searchPaths;
  searchPaths.addSearchPath("./branchbits/sphere", false);

  File vPointSprite(searchPaths.find("omni.vert"), "r", true);
  File fPointSprite(searchPaths.find("omni.frag"), "r", true);
  File gPointSprite(searchPaths.find("omni.geom"), "r", true);
  
  shaderV.source(vPointSprite.readAll(), Shader::VERTEX).compile().printLog();
  shaderF.source(fPointSprite.readAll(), Shader::FRAGMENT).compile().printLog();
  shaderG.source(gPointSprite.readAll(), Shader::GEOMETRY).compile().printLog();

  mShader.attach(shaderV);
  mShader.attach(shaderF);

/* -------------------------------------------------------------------------- *\
    GEOMETRY SHADER INPUT/OUTPUT
    input : GL_POINTS, GL_LINES, GL_LINES_ADJACENCY_EXT, GL_TRIANGLES,
            GL_TRIANGLES_ADJACENCY_EXT
    output: GL_POINTS, GL_LINE_STRIP, GL_TRIANGLE_STRIP 
\* -------------------------------------------------------------------------- */
  mShader.setGeometryInputPrimitive(graphics().LINES);
  mShader.setGeometryOutputPrimitive(graphics().TRIANGLE_STRIP);
  mShader.setGeometryOutputVertices(18);
  mShader.attach(shaderG);

  mShader.link(false).printLog();

  mShader.begin();
  mShader.uniform("lighting", 0.0);
  mShader.uniform("texture", 0.0);
  mShader.end();

  mShader.validate();
  mShader.printLog();
  
  return true;
}

inline bool OmniStereoGraphicsRenderer1::onFrame() {
  FPS::onFrame();

  // if running on a laptop?
  nav().step();
  // Vec3d v = nav().pos();
  // Quatd q = nav().quat();
  // oscSend().send("/pose", v.x, v.y, v.z, q.x, q.y, q.z, q.w);
  // nav().print();

  onAnimate(dt);
  
  Viewport vp(width(), height());

  if (bOmniEnable) {
    mOmni.onFrame(*this, lens(), pose, vp);
  } else {
    mOmni.onFrameFront(*this, lens(), pose, vp);
  }
  return true;
}

inline void OmniStereoGraphicsRenderer1::onDrawOmni(OmniStereo& omni) {
  graphics().error("start onDrawOmni");
  mShader.begin();
  mOmni.uniforms(mShader);
  graphics().error("start onDraw");
  onDraw(graphics());
  graphics().error("end onDraw");
  mShader.end();
  graphics().error("end onDrawOmni");
}

inline std::string OmniStereoGraphicsRenderer1::vertexCode() {
  // XXX use c++11 string literals
  return R"(
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
)";
}

inline std::string OmniStereoGraphicsRenderer1::fragmentCode() {
  return R"(
#version 120

uniform float lighting;
uniform float texture;
uniform sampler2D texture0;

uniform float frame_num;
varying vec4 fcolor;
// varying vec3 fnormal, flightDir, feyeVec;

// varying vec4 color;
// varying vec3 normal, lightDir, eyeVec;

///////////////////////////////////////////////////////////////////////
// F U N C T I O N S
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// C O L O R   F U N C T I O N S
///////////////////////////////////////////////////////////////////////

// R G B  2  H S V
vec3 rgb2HSV(vec3 _col){
  vec3 hsv;
  float mini = 0.0;
  float maxi = 0.0;
  if (_col.r < _col.g) mini = _col.r;
    else mini = _col.g;
  if (_col.b < mini) mini = _col.b;
  if (_col.r > _col.g) maxi = _col.r;
    else maxi = _col.g;
  if (_col.b > maxi) maxi = _col.b;
  hsv.z = maxi; //VALUE
  float delta = maxi - mini; //delta
  if (maxi > 0.0) hsv.y = delta / maxi; //SATURATION
    else hsv.y = 0.0;
  if (_col.r >= maxi) hsv.x = (_col.g - _col.b) / delta;
  else if (_col.g >= maxi) hsv.x = 2.0 + (_col.b - _col.r)/delta;
  else hsv.x = 4.0 + (_col.r - _col.g) / delta;
  hsv.x *= 60.0;
  if (hsv.x < 0.0) hsv.x += 360.0;
  return hsv;
}

// // H S V  2  R G B
vec3 hsv2RGB(vec3 _hsv){
  float hh, p, q, t, ff;
  int i;
  vec3 rgb;
  if(_hsv.y <= 0.0){
    rgb.r = _hsv.z;
    rgb.g = _hsv.z;
    rgb.b = _hsv.z;
    return rgb;
  }
  hh = _hsv.x;
  if(hh >= 360.) hh = (hh/360.);
  hh /= 60.0;
  i = int(hh);
  ff = hh - float(i);
  p = _hsv.z * (1.0 - _hsv.y);
  q = _hsv.z * (1.0 - (_hsv.y * ff));
  t = _hsv.z * (1.0 - (_hsv.y * (1.0 - ff)));

  if (i == 0){
      rgb.r = _hsv.z;
      rgb.g = t;
      rgb.b = p;
      return rgb;
    }
  else if (i == 1){
      rgb.r = q;
      rgb.g = _hsv.z;
      rgb.b = p;
      return rgb;
    }
  else if (i == 2){
      rgb.r = p;
      rgb.g = _hsv.z;
      rgb.b = t;
      return rgb;
    }
  else if (i == 3){
      rgb.r = p;
      rgb.g = q;
      rgb.b = _hsv.z;
      return rgb;
    }
  else if (i == 4){
      rgb.r = t;
      rgb.g = p;
      rgb.b = _hsv.z;
      return rgb;
    }
  else if (i == 5){
      rgb.r = _hsv.z;
      rgb.g = p;
      rgb.b = q;
      return rgb;
    }
  else {
      rgb.r = _hsv.z;
      rgb.g = p;
      rgb.b = q;
    return rgb;
  }

}

vec3 rgb2DEF(vec3 _col){
  mat3 XYZ; // Adobe RGB (1998)
  XYZ[0] = vec3(0.5767309, 0.1855540, 0.1881852);
  XYZ[1] = vec3(0.2973769, 0.6273491, 0.0752741);
  XYZ[2] = vec3(0.0270343, 0.0706872, 0.9911085); 
  mat3 DEF;
  DEF[0] = vec3(0.2053, 0.7125, 0.4670);
  DEF[1] = vec3(1.8537, -1.2797, -0.4429);
  DEF[2] = vec3(-0.3655, 1.0120, -0.6104);

  vec3 xyz = _col.rgb * XYZ;
  vec3 def = xyz * DEF;
  return def;
}

vec3 def2RGB(vec3 _def){
  mat3 XYZ; 
  XYZ[0] = vec3(0.6712, 0.4955, 0.1540);
  XYZ[1] = vec3(0.7061, 0.0248, 0.5223);
  XYZ[2] = vec3(0.7689, -0.2556, -0.8645); 
  mat3 RGB; // Adobe RGB (1998)
  RGB[0] = vec3(2.0413690, -0.5649464, -0.3446944);
  RGB[1] = vec3(-0.9692660, 1.8760108, 0.0415560);
  RGB[2] = vec3(0.0134474, -0.1183897, 1.0154096);

  vec3 xyz = _def * XYZ;
  vec3 rgb = xyz * RGB;
  return rgb;
}
float getB(vec3 _def){
    float b = sqrt((_def.r*_def.r) + (_def.g*_def.g) + (_def.b*_def.b));
    // b *= .72; //normalize...not sure why i have to do this
    return b;
}
float getC(vec3 _def){
    vec3 def_D = vec3(1.,0.,0.);
    float C = atan(length(cross(_def,def_D)), dot(_def,def_D));
    return C;
}
float getH(vec3 _def){
    vec3 def_E_axis = vec3(0.,1.,0.);
    float H = atan(_def.z, _def.y) - atan(def_E_axis.z, def_E_axis.y) ;
    return H;
}
vec3 rgb2BCH(vec3 _col){
  vec3 DEF = rgb2DEF(_col);
  float B = getB(DEF);
  float C = getC(DEF);
  float H = getH(DEF);
  return vec3(B,C,H);
}
vec3 bch2RGB(vec3 _bch){
  vec3 def;
  def.x = _bch.x * cos(_bch.y);
  def.y = _bch.x * sin(_bch.y) * cos(_bch.z);
  def.z = _bch.x * sin(_bch.y) * sin(_bch.z);
  vec3 rgb = def2RGB(def);
  return rgb;
}
// END FUNCTIONS



void main() {
  // yes, doing some dumb thing to use 'texture' and 'lighting'
  // they should be in omnin context.
  // but they will always be 0

  vec4 Cd = gl_Color;

  float phase = abs(1.-Cd.r) * 1.;
  float phase_offset = frame_num * 0.01;
  float w_amp = 1.;
  float w_freq = 2.;
  // this matches sin to same one controlling brightness in frag
  Cd.r += pow(((sin((phase - phase_offset) * w_freq) + 1.) * w_amp), 3.); 

  Cd.r += pow(1.-gl_Color.r, 10.) * 0.5; // high power gives bright tips with nice falloff
  Cd.r *= Cd.g; // apply z-depth darkening
  if (Cd.r >= 1.) Cd.r = 1.;
  if (Cd.r <= 0.05) Cd.r = 0.05;

  // saturation
  Cd.g = 1.2-Cd.r;

  // hue
  Cd.b = pow(1.-gl_Color.r,10.) * 2.1459;
  Cd.b += Cd.r * 1.;
  Cd.b += frame_num * 0.001;

  Cd.rgb = bch2RGB(Cd.rgb);


  vec4 colorMixed;
  if (texture > 0.0) {
    vec4 textureColor = texture2D(texture0, gl_TexCoord[0].st);
    // colorMixed = mix(color, textureColor, texture);
    vec4 pink_color = vec4(1.,0.,1.,1.); // pink lets us know its in texture mode
    colorMixed = mix(pink_color, pink_color, texture);
  } else {
    // colorMixed = color;
    colorMixed = Cd;
  }

  gl_FragColor = mix(colorMixed, colorMixed, lighting);
}
)";
}

// -----------------------------------------------------------------------------
inline std::string OmniStereoGraphicsRenderer1::geometryCode() {
  return R"(
#version 120
#extension GL_EXT_geometry_shader4 : enable // it is extension in 120

uniform float frame_num;
// varying in vec4 color[];
// varying in vec3 normal[], lightDir[], eyeVec[];

// varying out vec4 fcolor;
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


float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

///////////////////////////////////////////////////////////////////////
// 3 D   N O I S E
///////////////////////////////////////////////////////////////////////
// GLSL textureless classic 3D noise "cnoise",
// with an RSL-style periodic variant "pnoise".
// Author:  Stefan Gustavson (stefan.gustavson@liu.se)
// Version: 2011-10-11
//
// Many thanks to Ian McEwan of Ashima Arts for the
// ideas for permutation and gradient selection.
//
// Copyright (c) 2011 Stefan Gustavson. All rights reserved.
// Distributed under the MIT license. See LICENSE file.
// https://github.com/ashima/webgl-noise
//

vec3 mod289(vec3 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x)
{
  return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

vec3 fade(vec3 t) {
  return t*t*t*(t*(t*6.0-15.0)+10.0);
}

// Classic Perlin noise
float cnoise(vec3 P)
{
  vec3 Pi0 = floor(P); // Integer part for indexing
  vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
  Pi0 = mod289(Pi0);
  Pi1 = mod289(Pi1);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 * (1.0 / 7.0);
  vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 * (1.0 / 7.0);
  vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
  return 2.2 * n_xyz;
}

// Classic Perlin noise, periodic variant
float pnoise(vec3 P, vec3 rep)
{
  vec3 Pi0 = mod(floor(P), rep); // Integer part, modulo period
  vec3 Pi1 = mod(Pi0 + vec3(1.0), rep); // Integer part + 1, mod period
  Pi0 = mod289(Pi0);
  Pi1 = mod289(Pi1);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 * (1.0 / 7.0);
  vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 * (1.0 / 7.0);
  vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
  return 2.2 * n_xyz;
}
///////////////////////////////////////////////////////////////////////
// END NOISE
///////////////////////////////////////////////////////////////////////




void main(){

  vec4 vtx[2];
  vtx[0] = vec4(gl_PositionIn[0].xyz, 0.5);
  vtx[1] = vec4(gl_PositionIn[1].xyz, 0.5);
  // do some wiggling
  // offset = animation speed
  vec3 offset = vec3(frame_num*.05);

  // get 1D noise for each point position
  // outside multiplier = amplitude
  // poisition multiplier = frequency
  // 2nd parameter = noise scale (i think)
  float n_amplitude = 0.;
  float noise_0 = n_amplitude * pnoise((vtx[0].xyz + offset) * 0.5, vec3(100.));
  float noise_1 = n_amplitude * pnoise((vtx[1].xyz + offset) * 0.5, vec3(100.));

  // get normalized point poisition. this is what direction we will perturb the original by
  // there's probably a smarter vector we should be using for this
  // houdini's noise function takes a vec3 and has the option to output either a float or vec3
  // all the noises I could find output floats. there's probably a trick to get proper vec3 noise
  vec3 v0_norm = normalize(vtx[0].xyz);
  vec3 v1_norm = normalize(vtx[1].xyz);

  // add it to original point position
  vtx[0].xyz += (v0_norm * noise_0);
  vtx[1].xyz += (v1_norm * noise_1);
  // // end noise

  // get radius from red channel
  vec4 Cd = gl_FrontColorIn[0];
  float radius[2] = float[2](1.,1.);
  float max_radius = 0.01;
  float min_radius = Cd.r * .04;
  radius[0] *= max_radius;
  if (radius[0] >= max_radius) radius[0] = max_radius;

  // scale diameter by sin wave

  float phase = abs(1.-Cd.r) * 1.;
  // float phase_offset = frame_num * 0.001;
  float phase_offset = frame_num * 0.01;
  float w_amp = .3;
  float w_freq = 2.;
  // this matches sin to same one controlling brightness in frag
  radius[0] *= pow(((sin((phase - phase_offset) * w_freq) + 1.) * w_amp), 3.); 
  radius[0] += min_radius;
  // taper end of this segment to match (sorta) the next one
  radius[1] = radius[0] * .98;



  vec3 dir = gl_PositionIn[0].xyz - gl_PositionIn[1].xyz;
  // arbitrary dir to gen cross product
  vec3 other_dir = vec3(-1.2,1.7,-2.4) - gl_PositionIn[1].xyz;

  vec3 axis1 = normalize(cross(dir, other_dir));
  vec3 axis2 = normalize(cross(dir, axis1));
  
  // inlcude i == 8 for closing the loop
  for (int i = 0; i <= 8; i++) {
    for (int j = 0; j < 2; j++) {
      vec4 p = vec4(axis1 * cos_lkup[i] + axis2 * sin_lkup[i], 0.5);

      // fcolor = color[j];
      gl_Position = gl_PositionIn[j] + p * radius[j];

      // assign z depth to green channel for use in fragment
      vec4 Ci = gl_FrontColorIn[j];
      Ci.g = 1.-(pow(gl_Position.z, 1.5) * .1)*.1;
      gl_FrontColor = Ci;

      EmitVertex();
    }
  }

  EndPrimitive();
}
)";
}

// //------------------------------------------------------------------------------------------

// inline const char* OmniStereoGraphicsRenderer1::vertexCodeCh() {
//   // XXX use c++11 string literals
//   return AL_STRINGYFY(
// #version 120

// // @omni_eye: the eye parallax distance.
// //  This will be zero for mono, and positive/negative for right/left
// // eyes.
// //  Pass this uniform to the shader in the OmniStereoDrawable
// // callback
// uniform float omni_eye;

// // @omni_radius: the radius of the allosphere in OpenGL units.
// //  This will be infinity for the original layout (we default to 1e10).
// uniform float omni_radius;

// // @omni_face: the GL_TEXTURE_CUBE_MAP face being rendered.
// //  For a typical forward-facing view, this should == 5.
// //  Pass this uniform to the shader in the OmniStereoDrawable
// // callback
// uniform int omni_face;

// // @omni_near: the near clipping plane.
// uniform float omni_near;

// // @omni_far: the far clipping plane.
// uniform float omni_far;

// // omni_render(vertex)
// // @vertex: the eye-space vertex to be rendered.
// //  Typically gl_Position = omni_render(gl_ModelViewMatrix *
// // gl_Vertex);
// vec4 omni_render(in vec4 vertex) {
//   float l = length(vertex.xz);
//   vec3 vn = normalize(vertex.xyz);
//   // Precise formula.
//   float displacement = omni_eye *
//     (omni_radius * omni_radius -
//        sqrt(l * l * omni_radius * omni_radius +
//             omni_eye * omni_eye * (omni_radius * omni_radius - l * l))) /
//     (omni_radius * omni_radius - omni_eye * omni_eye);
//   // Approximation, safe if omni_radius / omni_eye is very large, which is true for the allosphere.
//   // float displacement = omni_eye * (1.0 - l / omni_radius);
//   // Displace vertex.
//   vertex.xz += vec2(displacement * vn.z, displacement * -vn.x);

//   // convert eye-space into cubemap-space:
//   // GL_TEXTURE_CUBE_MAP_POSITIVE_X
//   if (omni_face == 0) {
//     vertex.xyz = vec3(-vertex.z, -vertex.y, -vertex.x);
//   }
//       // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
//       else if (omni_face == 1) {
//     vertex.xyz = vec3(vertex.z, -vertex.y, vertex.x);
//   }
//       // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
//       else if (omni_face == 2) {
//     vertex.xyz = vec3(vertex.x, vertex.z, -vertex.y);
//   }
//       // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
//       else if (omni_face == 3) {
//     vertex.xyz = vec3(vertex.x, -vertex.z, vertex.y);
//   }
//       // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
//       else if (omni_face == 4) {
//     vertex.xyz = vec3(vertex.x, -vertex.y, -vertex.z);
//   }
//       // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
//       else {
//     vertex.xyz = vec3(-vertex.x, -vertex.y, vertex.z);
//   }
//   // convert into screen-space:
//   // simplified perspective projection since fovy = 90 and aspect = 1
//   vertex.zw = vec2((vertex.z * (omni_far + omni_near) +
//                     vertex.w * omni_far * omni_near * 2.) /
//                        (omni_near - omni_far),
//                    -vertex.z);
//   return vertex;
// }

// // varying vec4 color;
// // varying vec3 normal, lightDir; //, eyeVec;

// void main() {
//   vec4 Cd = gl_Color;
//   // Cd.g = 1. - (gl_Vertex.z * 0.00025)+0.1;

//   // color = Cd;
//   vec4 vertex = gl_ModelViewMatrix * gl_Vertex;


//   gl_FrontColor = Cd;
//   gl_BackColor = Cd;
  
//   // normal = gl_NormalMatrix * gl_Normal;
//   // vec3 V = vertex.xyz;
//   // eyeVec = normalize(-V);
//   // lightDir = normalize(vec3(gl_LightSource[0].position.xyz - V));
//   gl_TexCoord[0] = gl_MultiTexCoord0;
//   gl_Position = omni_render(vertex);
// }
// );
// }

// inline std::string OmniStereoGraphicsRenderer1::fragmentCode() {
//   return AL_STRINGYFY(
// #version 120

// uniform float lighting;
// uniform float texture;
// uniform sampler2D texture0;

// uniform float frame_num;
// varying vec4 fcolor;
// // varying vec3 fnormal, flightDir, feyeVec;

// // varying vec4 color;
// // varying vec3 normal, lightDir, eyeVec;

// ///////////////////////////////////////////////////////////////////////
// // F U N C T I O N S
// ///////////////////////////////////////////////////////////////////////

// ///////////////////////////////////////////////////////////////////////
// // C O L O R   F U N C T I O N S
// ///////////////////////////////////////////////////////////////////////

// // R G B  2  H S V
// vec3 rgb2HSV(vec3 _col){
//   vec3 hsv;
//   float mini = 0.0;
//   float maxi = 0.0;
//   if (_col.r < _col.g) mini = _col.r;
//     else mini = _col.g;
//   if (_col.b < mini) mini = _col.b;
//   if (_col.r > _col.g) maxi = _col.r;
//     else maxi = _col.g;
//   if (_col.b > maxi) maxi = _col.b;
//   hsv.z = maxi; //VALUE
//   float delta = maxi - mini; //delta
//   if (maxi > 0.0) hsv.y = delta / maxi; //SATURATION
//     else hsv.y = 0.0;
//   if (_col.r >= maxi) hsv.x = (_col.g - _col.b) / delta;
//   else if (_col.g >= maxi) hsv.x = 2.0 + (_col.b - _col.r)/delta;
//   else hsv.x = 4.0 + (_col.r - _col.g) / delta;
//   hsv.x *= 60.0;
//   if (hsv.x < 0.0) hsv.x += 360.0;
//   return hsv;
// }

// // // H S V  2  R G B
// vec3 hsv2RGB(vec3 _hsv){
//   float hh, p, q, t, ff;
//   int i;
//   vec3 rgb;
//   if(_hsv.y <= 0.0){
//     rgb.r = _hsv.z;
//     rgb.g = _hsv.z;
//     rgb.b = _hsv.z;
//     return rgb;
//   }
//   hh = _hsv.x;
//   if(hh >= 360.) hh = (hh/360.);
//   hh /= 60.0;
//   i = int(hh);
//   ff = hh - float(i);
//   p = _hsv.z * (1.0 - _hsv.y);
//   q = _hsv.z * (1.0 - (_hsv.y * ff));
//   t = _hsv.z * (1.0 - (_hsv.y * (1.0 - ff)));

//   if (i == 0){
//       rgb.r = _hsv.z;
//       rgb.g = t;
//       rgb.b = p;
//       return rgb;
//     }
//   else if (i == 1){
//       rgb.r = q;
//       rgb.g = _hsv.z;
//       rgb.b = p;
//       return rgb;
//     }
//   else if (i == 2){
//       rgb.r = p;
//       rgb.g = _hsv.z;
//       rgb.b = t;
//       return rgb;
//     }
//   else if (i == 3){
//       rgb.r = p;
//       rgb.g = q;
//       rgb.b = _hsv.z;
//       return rgb;
//     }
//   else if (i == 4){
//       rgb.r = t;
//       rgb.g = p;
//       rgb.b = _hsv.z;
//       return rgb;
//     }
//   else if (i == 5){
//       rgb.r = _hsv.z;
//       rgb.g = p;
//       rgb.b = q;
//       return rgb;
//     }
//   else {
//       rgb.r = _hsv.z;
//       rgb.g = p;
//       rgb.b = q;
//     return rgb;
//   }

// }

// vec3 rgb2DEF(vec3 _col){
//   mat3 XYZ; // Adobe RGB (1998)
//   XYZ[0] = vec3(0.5767309, 0.1855540, 0.1881852);
//   XYZ[1] = vec3(0.2973769, 0.6273491, 0.0752741);
//   XYZ[2] = vec3(0.0270343, 0.0706872, 0.9911085); 
//   mat3 DEF;
//   DEF[0] = vec3(0.2053, 0.7125, 0.4670);
//   DEF[1] = vec3(1.8537, -1.2797, -0.4429);
//   DEF[2] = vec3(-0.3655, 1.0120, -0.6104);

//   vec3 xyz = _col.rgb * XYZ;
//   vec3 def = xyz * DEF;
//   return def;
// }

// vec3 def2RGB(vec3 _def){
//   mat3 XYZ; 
//   XYZ[0] = vec3(0.6712, 0.4955, 0.1540);
//   XYZ[1] = vec3(0.7061, 0.0248, 0.5223);
//   XYZ[2] = vec3(0.7689, -0.2556, -0.8645); 
//   mat3 RGB; // Adobe RGB (1998)
//   RGB[0] = vec3(2.0413690, -0.5649464, -0.3446944);
//   RGB[1] = vec3(-0.9692660, 1.8760108, 0.0415560);
//   RGB[2] = vec3(0.0134474, -0.1183897, 1.0154096);

//   vec3 xyz = _def * XYZ;
//   vec3 rgb = xyz * RGB;
//   return rgb;
// }
// float getB(vec3 _def){
//     float b = sqrt((_def.r*_def.r) + (_def.g*_def.g) + (_def.b*_def.b));
//     // b *= .72; //normalize...not sure why i have to do this
//     return b;
// }
// float getC(vec3 _def){
//     vec3 def_D = vec3(1.,0.,0.);
//     float C = atan(length(cross(_def,def_D)), dot(_def,def_D));
//     return C;
// }
// float getH(vec3 _def){
//     vec3 def_E_axis = vec3(0.,1.,0.);
//     float H = atan(_def.z, _def.y) - atan(def_E_axis.z, def_E_axis.y) ;
//     return H;
// }
// vec3 rgb2BCH(vec3 _col){
//   vec3 DEF = rgb2DEF(_col);
//   float B = getB(DEF);
//   float C = getC(DEF);
//   float H = getH(DEF);
//   return vec3(B,C,H);
// }
// vec3 bch2RGB(vec3 _bch){
//   vec3 def;
//   def.x = _bch.x * cos(_bch.y);
//   def.y = _bch.x * sin(_bch.y) * cos(_bch.z);
//   def.z = _bch.x * sin(_bch.y) * sin(_bch.z);
//   vec3 rgb = def2RGB(def);
//   return rgb;
// }
// // END FUNCTIONS



// void main() {
//   // yes, doing some dumb thing to use 'texture' and 'lighting'
//   // they should be in omnin context.
//   // but they will always be 0

//   vec4 Cd = gl_Color;

//   float phase = abs(1.-Cd.r) * 1.;
//   float phase_offset = frame_num * 0.01;
//   float w_amp = 1.;
//   float w_freq = 2.;
//   // this matches sin to same one controlling brightness in frag
//   Cd.r += pow(((sin((phase - phase_offset) * w_freq) + 1.) * w_amp), 3.); 

//   Cd.r += pow(1.-gl_Color.r, 10.) * 0.5; // high power gives bright tips with nice falloff
//   Cd.r *= Cd.g; // apply z-depth darkening
//   if (Cd.r >= 1.) Cd.r = 1.;
//   if (Cd.r <= 0.05) Cd.r = 0.05;

//   // saturation
//   Cd.g = 1.2-Cd.r;

//   // hue
//   Cd.b = pow(1.-gl_Color.r,10.) * 2.1459;
//   Cd.b += Cd.r * 1.;
//   Cd.b += frame_num * 0.001;

//   Cd.rgb = bch2RGB(Cd.rgb);


//   vec4 colorMixed;
//   if (texture > 0.0) {
//     vec4 textureColor = texture2D(texture0, gl_TexCoord[0].st);
//     // colorMixed = mix(color, textureColor, texture);
//     vec4 pink_color = vec4(1.,0.,1.,1.); // pink lets us know its in texture mode
//     colorMixed = mix(pink_color, pink_color, texture);
//   } else {
//     // colorMixed = color;
//     colorMixed = Cd;
//   }

//   gl_FragColor = mix(colorMixed, colorMixed, lighting);
// }
// );
// }

// // -----------------------------------------------------------------------------
// inline std::string OmniStereoGraphicsRenderer1::geometryCode() {
//   return AL_STRINGYFY(
// #version 120
// #extension GL_EXT_geometry_shader4 : enable // it is extension in 120

// uniform float frame_num;
// // varying in vec4 color[];
// // varying in vec3 normal[], lightDir[], eyeVec[];

// // varying out vec4 fcolor;
// // varying out vec3 fnormal, flightDir;//, feyeVec;

// // dividing 2PI by 8, 9 values for rotating fully
// uniform float sin_lkup[] = float[](
//   0.00000000,
//   0.70710678,
//   1.00000000,
//   0.70710678,
//   0.00000000,
//   -0.70710678,
//   -1.00000000,
//   -0.70710678,
//   0.00000000
// );

// uniform float cos_lkup[] = float[](
//   1.00000000,
//   0.70710678,
//   0.00000000,
//   -0.70710678,
//   -1.00000000,
//   -0.70710678,
//   0.00000000,
//   0.70710678,
//   1.00000000
// );


// float rand(vec2 co){
//   return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
// }

// ///////////////////////////////////////////////////////////////////////
// // 3 D   N O I S E
// ///////////////////////////////////////////////////////////////////////
// // GLSL textureless classic 3D noise "cnoise",
// // with an RSL-style periodic variant "pnoise".
// // Author:  Stefan Gustavson (stefan.gustavson@liu.se)
// // Version: 2011-10-11
// //
// // Many thanks to Ian McEwan of Ashima Arts for the
// // ideas for permutation and gradient selection.
// //
// // Copyright (c) 2011 Stefan Gustavson. All rights reserved.
// // Distributed under the MIT license. See LICENSE file.
// // https://github.com/ashima/webgl-noise
// //

// vec3 mod289(vec3 x)
// {
//   return x - floor(x * (1.0 / 289.0)) * 289.0;
// }

// vec4 mod289(vec4 x)
// {
//   return x - floor(x * (1.0 / 289.0)) * 289.0;
// }

// vec4 permute(vec4 x)
// {
//   return mod289(((x*34.0)+1.0)*x);
// }

// vec4 taylorInvSqrt(vec4 r)
// {
//   return 1.79284291400159 - 0.85373472095314 * r;
// }

// vec3 fade(vec3 t) {
//   return t*t*t*(t*(t*6.0-15.0)+10.0);
// }

// // Classic Perlin noise
// float cnoise(vec3 P)
// {
//   vec3 Pi0 = floor(P); // Integer part for indexing
//   vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
//   Pi0 = mod289(Pi0);
//   Pi1 = mod289(Pi1);
//   vec3 Pf0 = fract(P); // Fractional part for interpolation
//   vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
//   vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
//   vec4 iy = vec4(Pi0.yy, Pi1.yy);
//   vec4 iz0 = Pi0.zzzz;
//   vec4 iz1 = Pi1.zzzz;

//   vec4 ixy = permute(permute(ix) + iy);
//   vec4 ixy0 = permute(ixy + iz0);
//   vec4 ixy1 = permute(ixy + iz1);

//   vec4 gx0 = ixy0 * (1.0 / 7.0);
//   vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
//   gx0 = fract(gx0);
//   vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
//   vec4 sz0 = step(gz0, vec4(0.0));
//   gx0 -= sz0 * (step(0.0, gx0) - 0.5);
//   gy0 -= sz0 * (step(0.0, gy0) - 0.5);

//   vec4 gx1 = ixy1 * (1.0 / 7.0);
//   vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
//   gx1 = fract(gx1);
//   vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
//   vec4 sz1 = step(gz1, vec4(0.0));
//   gx1 -= sz1 * (step(0.0, gx1) - 0.5);
//   gy1 -= sz1 * (step(0.0, gy1) - 0.5);

//   vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
//   vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
//   vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
//   vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
//   vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
//   vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
//   vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
//   vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

//   vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
//   g000 *= norm0.x;
//   g010 *= norm0.y;
//   g100 *= norm0.z;
//   g110 *= norm0.w;
//   vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
//   g001 *= norm1.x;
//   g011 *= norm1.y;
//   g101 *= norm1.z;
//   g111 *= norm1.w;

//   float n000 = dot(g000, Pf0);
//   float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
//   float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
//   float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
//   float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
//   float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
//   float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
//   float n111 = dot(g111, Pf1);

//   vec3 fade_xyz = fade(Pf0);
//   vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
//   vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
//   float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
//   return 2.2 * n_xyz;
// }

// // Classic Perlin noise, periodic variant
// float pnoise(vec3 P, vec3 rep)
// {
//   vec3 Pi0 = mod(floor(P), rep); // Integer part, modulo period
//   vec3 Pi1 = mod(Pi0 + vec3(1.0), rep); // Integer part + 1, mod period
//   Pi0 = mod289(Pi0);
//   Pi1 = mod289(Pi1);
//   vec3 Pf0 = fract(P); // Fractional part for interpolation
//   vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
//   vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
//   vec4 iy = vec4(Pi0.yy, Pi1.yy);
//   vec4 iz0 = Pi0.zzzz;
//   vec4 iz1 = Pi1.zzzz;

//   vec4 ixy = permute(permute(ix) + iy);
//   vec4 ixy0 = permute(ixy + iz0);
//   vec4 ixy1 = permute(ixy + iz1);

//   vec4 gx0 = ixy0 * (1.0 / 7.0);
//   vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
//   gx0 = fract(gx0);
//   vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
//   vec4 sz0 = step(gz0, vec4(0.0));
//   gx0 -= sz0 * (step(0.0, gx0) - 0.5);
//   gy0 -= sz0 * (step(0.0, gy0) - 0.5);

//   vec4 gx1 = ixy1 * (1.0 / 7.0);
//   vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
//   gx1 = fract(gx1);
//   vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
//   vec4 sz1 = step(gz1, vec4(0.0));
//   gx1 -= sz1 * (step(0.0, gx1) - 0.5);
//   gy1 -= sz1 * (step(0.0, gy1) - 0.5);

//   vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
//   vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
//   vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
//   vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
//   vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
//   vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
//   vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
//   vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

//   vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
//   g000 *= norm0.x;
//   g010 *= norm0.y;
//   g100 *= norm0.z;
//   g110 *= norm0.w;
//   vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
//   g001 *= norm1.x;
//   g011 *= norm1.y;
//   g101 *= norm1.z;
//   g111 *= norm1.w;

//   float n000 = dot(g000, Pf0);
//   float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
//   float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
//   float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
//   float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
//   float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
//   float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
//   float n111 = dot(g111, Pf1);

//   vec3 fade_xyz = fade(Pf0);
//   vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
//   vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
//   float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
//   return 2.2 * n_xyz;
// }
// ///////////////////////////////////////////////////////////////////////
// // END NOISE
// ///////////////////////////////////////////////////////////////////////




// void main(){

//   vec4 vtx[2];
//   vtx[0] = vec4(gl_PositionIn[0].xyz, 0.5);
//   vtx[1] = vec4(gl_PositionIn[1].xyz, 0.5);
//   // do some wiggling
//   // offset = animation speed
//   vec3 offset = vec3(frame_num*.05);

//   // get 1D noise for each point position
//   // outside multiplier = amplitude
//   // poisition multiplier = frequency
//   // 2nd parameter = noise scale (i think)
//   float n_amplitude = 0.;
//   float noise_0 = n_amplitude * pnoise((vtx[0].xyz + offset) * 0.5, vec3(100.));
//   float noise_1 = n_amplitude * pnoise((vtx[1].xyz + offset) * 0.5, vec3(100.));

//   // get normalized point poisition. this is what direction we will perturb the original by
//   // there's probably a smarter vector we should be using for this
//   // houdini's noise function takes a vec3 and has the option to output either a float or vec3
//   // all the noises I could find output floats. there's probably a trick to get proper vec3 noise
//   vec3 v0_norm = normalize(vtx[0].xyz);
//   vec3 v1_norm = normalize(vtx[1].xyz);

//   // add it to original point position
//   vtx[0].xyz += (v0_norm * noise_0);
//   vtx[1].xyz += (v1_norm * noise_1);
//   // // end noise

//   // get radius from red channel
//   vec4 Cd = gl_FrontColorIn[0];
//   float radius[2] = float[2](1.,1.);
//   float max_radius = 0.01;
//   float min_radius = Cd.r * .04;
//   radius[0] *= max_radius;
//   if (radius[0] >= max_radius) radius[0] = max_radius;

//   // scale diameter by sin wave

//   float phase = abs(1.-Cd.r) * 1.;
//   // float phase_offset = frame_num * 0.001;
//   float phase_offset = frame_num * 0.01;
//   float w_amp = .3;
//   float w_freq = 2.;
//   // this matches sin to same one controlling brightness in frag
//   radius[0] *= pow(((sin((phase - phase_offset) * w_freq) + 1.) * w_amp), 3.); 
//   radius[0] += min_radius;
//   // taper end of this segment to match (sorta) the next one
//   radius[1] = radius[0] * .98;



//   vec3 dir = gl_PositionIn[0].xyz - gl_PositionIn[1].xyz;
//   // arbitrary dir to gen cross product
//   vec3 other_dir = vec3(-1.2,1.7,-2.4) - gl_PositionIn[1].xyz;

//   vec3 axis1 = normalize(cross(dir, other_dir));
//   vec3 axis2 = normalize(cross(dir, axis1));
  
//   // inlcude i == 8 for closing the loop
//   for (int i = 0; i <= 8; i++) {
//     for (int j = 0; j < 2; j++) {
//       vec4 p = vec4(axis1 * cos_lkup[i] + axis2 * sin_lkup[i], 0.5);

//       // fcolor = color[j];
//       gl_Position = gl_PositionIn[j] + p * radius[j];

//       // assign z depth to green channel for use in fragment
//       vec4 Ci = gl_FrontColorIn[j];
//       Ci.g = 1.-(pow(gl_Position.z, 1.5) * .1)*.1;
//       gl_FrontColor = Ci;

//       EmitVertex();
//     }
//   }

//   EndPrimitive();
// }
// );
// }
// //------------------------------------------------------------------------------

}  // al

#endif


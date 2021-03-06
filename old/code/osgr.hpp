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

  SearchPaths searchPaths;
  searchPaths.addSearchPath("./", true);
  searchPaths.addSearchPath("./branchbits/code", false);
  File vPointSprite(searchPaths.find("omni.vert"), "r", true);
  File fPointSprite(searchPaths.find("omni.frag"), "r", true);
  File gPointSprite(searchPaths.find("omni.geom"), "r", true);

  Shader shaderV, shaderF, shaderG;

  shaderV.source(vPointSprite.readAll(), Shader::VERTEX).compile().printLog();
  mShader.attach(shaderV);

  shaderF.source(fPointSprite.readAll(), Shader::FRAGMENT).compile().printLog();
  mShader.attach(shaderF);

  shaderG.source(gPointSprite.readAll(), Shader::GEOMETRY).compile().printLog();
  mShader.setGeometryInputPrimitive(graphics().LINES);
  mShader.setGeometryOutputPrimitive(graphics().TRIANGLE_STRIP);

/* -------------------------------------------------------------------------- *\
    GEOMETRY SHADER INPUT/OUTPUT
    input : GL_POINTS, GL_LINES, GL_LINES_ADJACENCY_EXT, GL_TRIANGLES,
            GL_TRIANGLES_ADJACENCY_EXT
    output: GL_POINTS, GL_LINE_STRIP, GL_TRIANGLE_STRIP 
\* -------------------------------------------------------------------------- */
  
  mShader.setGeometryOutputVertices(18);
  
  mShader.attach(shaderG);
  graphics().error("gl error 4"); //error?!

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
varying vec4 color;
varying vec3 normal, lightDir, eyeVec;
void main() {
  color = gl_Color;
  vec4 vertex = gl_ModelViewMatrix * gl_Vertex;
  normal = gl_NormalMatrix * gl_Normal;
  vec3 V = vertex.xyz;
  eyeVec = normalize(-V);
  lightDir = normalize(vec3(gl_LightSource[0].position.xyz - V));
  gl_TexCoord[0] = gl_MultiTexCoord0;
  gl_Position = omni_render(vertex);
}
)";
}

inline std::string OmniStereoGraphicsRenderer1::fragmentCode() {
  return R"(
uniform float lighting;
uniform float texture;
uniform sampler2D texture0;
varying vec4 color;
varying vec3 normal, lightDir, eyeVec;
void main() {
  vec4 colorMixed;
  if (texture > 0.0) {
    vec4 textureColor = texture2D(texture0, gl_TexCoord[0].st);
    colorMixed = mix(color, textureColor, texture);
  } else {
    colorMixed = color;
  }
  vec4 final_color = colorMixed * gl_LightSource[0].ambient;
  vec3 N = normalize(normal);
  vec3 L = lightDir;
  float lambertTerm = max(dot(N, L), 0.0);
  final_color += gl_LightSource[0].diffuse * colorMixed * lambertTerm;
  vec3 E = eyeVec;
  vec3 R = reflect(-L, N);
  float spec = pow(max(dot(R, E), 0.0), 0.9 + 1e-20);
  final_color += gl_LightSource[0].specular * spec;
  gl_FragColor = mix(colorMixed, final_color, lighting);
}
)";
}

// -----------------------------------------------------------------------------
inline std::string OmniStereoGraphicsRenderer1::geometryCode() {
  return R"(
#version 120
#extension GL_EXT_geometry_shader4 : enable // it is extension in 120

varying vec4 color[];
varying vec3 normal[], lightDir[], eyeVec[];

void main(){
  for (int i = 0; i < gl_VerticesIn; ++i){
    gl_Position = gl_PositionIn[i];
    EmitVertex();
  }
  EndPrimitive();
}
)";
}
//------------------------------------------------------------------------------

}  // al

#endif


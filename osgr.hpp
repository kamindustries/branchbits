#ifndef AL_OMNI_STEREO_GRAPHICS_RENDERER1_H
#define AL_OMNI_STEREO_GRAPHICS_RENDERER1_H

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
  virtual std::string geometryCode();

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
    : mNavControl(mNav), mOSCSend(12001), mOmni(2048, true) {

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
  
  Shader shaderV, shaderF, shaderG;

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
  nav().step();
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
  return "";
}

inline std::string OmniStereoGraphicsRenderer1::fragmentCode() {
  return "";
}

inline std::string OmniStereoGraphicsRenderer1::geometryCode() {
  return "";
}

}  // al

#endif


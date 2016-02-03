#pragma once

#include "allocore/al_Allocore.hpp"
#include "allocore/io/al_Window.hpp"
#include "alloutil/al_FPS.hpp"
#include "alloutil/al_OmniStereo.hpp"
#include "alloutil/al_Simulator.hpp"  // DEVICE_SERVER_PORT_CONNECTION_US

namespace al {

class BranchRenderer : public Window, public FPS, public OmniStereo::Drawable {
public:
  BranchRenderer();
  virtual ~BranchRenderer();

  virtual void onDraw(Graphics& gl) {}
  virtual void onAnimate(al_sec dt) {}
  virtual bool onCreate();
  virtual bool onFrame();
  virtual void onDrawOmni(OmniStereo& omni);

  void start();
  void initWindow(const Window::Dim& dims = Window::Dim(800, 400));
  void initOmni(std::string path = "");
  void initShaders();

  const Lens& lens() const { return mLens; }
  Lens& lens() { return mLens; }

  const Graphics& graphics() const { return mGraphics; }
  Graphics& graphics() { return mGraphics; }

  ShaderProgram& shader() { return mShader; }

  OmniStereo& omni() { return mOmni; }

  const std::string& hostName() const { return mHostName; }

  bool omniEnable() const { return bOmniEnable; }
  void omniEnable(bool b) { bOmniEnable = b; }

protected:
  const Nav& nav() const { return mNav; }
  Nav& nav() { return mNav; }

  OmniStereo mOmni;

  Lens mLens;
  Graphics mGraphics;

  Pose pose;

  ShaderProgram mShader;

  std::string mHostName;

  bool bOmniEnable;

  Nav mNav;
  NavInputControl mNavControl;
  StandardWindowKeyControls mStdControls;
};

inline void BranchRenderer::start() {
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

inline BranchRenderer::~BranchRenderer() {}

inline BranchRenderer::BranchRenderer()
: mNavControl(mNav), mOmni(2048, true) {

  bOmniEnable = true;
  mHostName = Socket::hostName();

  nav().smooth(0.8);

  initWindow();
  initOmni();

  Window::append(mStdControls);
  Window::append(mNavControl);
}

inline void BranchRenderer::initOmni(std::string path) {
  mOmni.configure(path, mHostName);
  if (mOmni.activeStereo()) {
    mOmni.mode(OmniStereo::ACTIVE).stereo(true);
  }
}

inline void BranchRenderer::initWindow(const Window::Dim& dims) {
  Window::dimensions(dims);
  Window::title("BranchRenderer");
  Window::fps(60);
  Window::displayMode(Window::DEFAULT_BUF);
}

inline void BranchRenderer::initShaders() {
  Shader shaderV, shaderF, shaderG;

  SearchPaths searchPaths;
  searchPaths.addSearchPath(".", true);

  File vPointSprite(searchPaths.find("omni.vert"), "r", true);
  File fPointSprite(searchPaths.find("omni.frag"), "r", true);
  File gPointSprite(searchPaths.find("omni.geom"), "r", true);
  
  shaderV.source(vPointSprite.readAll(), Shader::VERTEX).compile().printLog();
  shaderF.source(fPointSprite.readAll(), Shader::FRAGMENT).compile().printLog();
  shaderG.source(gPointSprite.readAll(), Shader::GEOMETRY).compile().printLog();

  mShader.attach(shaderV);
  mShader.attach(shaderF);

  // SHOULD MATCH WITH GEOMETRY SHADER
  mShader.setGeometryInputPrimitive(graphics().LINES);
  mShader.setGeometryOutputPrimitive(graphics().TRIANGLE_STRIP);
  mShader.setGeometryOutputVertices(18);
  mShader.attach(shaderG);

  mShader.link(false).printLog();

  mShader.validate();
  mShader.printLog();
}

inline bool BranchRenderer::onCreate() {
  mOmni.onCreate();
  initShaders();
  return true;
}

inline bool BranchRenderer::onFrame() {
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

inline void BranchRenderer::onDrawOmni(OmniStereo& omni) {
  graphics().error("start onDrawOmni");
  mShader.begin();
  mOmni.uniforms(mShader);
  graphics().error("start onDraw");
  onDraw(graphics());
  graphics().error("end onDraw");
  mShader.end();
  graphics().error("end onDrawOmni");
}

}  // al
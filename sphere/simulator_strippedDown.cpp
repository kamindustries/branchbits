/* 
  Stripped down version of simulator.cpp
  
  todo:
  - loop through branches to add vertices so you can properly update width
  - construct in such a way as to modulate timing for each branch growth

*/

#include <iostream>
#include <math.h>
#include <thread>
// #include <map>

#include "allocore/io/al_App.hpp"
#include "Cuttlebone/Cuttlebone.hpp"
// #include "allocore/graphics/al_Mesh.hpp"

#include "Grow.hpp"
#include "common.hpp"

#include "Gamma/Gamma.h"
#include "Gamma/SamplePlayer.h"

// #include "/Users/kurt/AlloProject/AlloSystem/alloutil/alloutil/al_Simulator.hpp"
#include "alloutil/al_Simulator.hpp"
#include "alloutil/al_AlloSphereAudioSpatializer.hpp"

using namespace al;
using namespace std;

std::string vertexCode();
std::string fragmentCode();
std::string geometryCode();

// Main app
struct SpaceCol : App, AlloSphereAudioSpatializer, InterfaceServerClient {
  State* state;
  cuttlebone::Maker<State, 9000> maker;

  Mesh m_leaf;
  Buffer<Mesh::Vertex> oldPos_tree;
  Mesh groundPlane;

  // Graphics gl;
  ShaderProgram shaderP;
  Shader shaderV, shaderF, shaderG;

  // thread stuff
  thread computeThread;
  bool threadDone = false;

  SoundSource tap;
  gam::SamplePlayer<> samplePlayer;

  SpaceCol() :  maker(Simulator::defaultBroadcastIP()),
                InterfaceServerClient(Simulator::defaultInterfaceServerIP()) {
    cout << "------------------" << endl;
    cout << "Space Colonization" << endl;
    cout << "------------------" << endl;
    cout << "" << endl;

    // Initialize State parameters
    state = new State;
    memset(state, 0, sizeof(State));
    InitState(state);
    state->pose = nav();
    
    // WINDOW & CAMERA
    // initial camera position and far clipping plane
    nav().pos(0, 0, 0);
    lens().far(1000);
    initWindow(Window::Dim(0, 0, 600, 400), "Pineal Portal", 60);

    // set up ground plane
    addSurface(groundPlane, 100, 100, 100, 100);
    groundPlane.generateNormals();
    groundPlane.primitive(Graphics::TRIANGLES);
    
    // INITIALIZE MESHES
    // m_leaf.primitive(Graphics::POINTS);
    m_leaf.primitive(Graphics::TRIANGLE_STRIP);
    // m_root.primitive(Graphics::POINTS);
    m_tree.primitive(Graphics::LINES);
    
    // for (int i=0; i<MAX_LEAVES; i++) {
    //   state->leafSkip[i] = 0;
    // }

    for (int i=0; i<LEAF_COUNT; i++){
      leaves.push_back(Vec3f(0,0,0));
      leaves[i].RandomizeTorus();

      state->leafPos[i] = leaves[i].Position;
      state->leafColor[i] = leafColorUnhit;

      m_leaf.vertex(state->leafPos[i]);
      m_leaf.color(state->leafColor[i]);
    }
    state->currentLeafSize = LEAF_COUNT;

    state->refreshLeaves = 1;
    state->refreshTree = 1;

    // A U D I O
    AlloSphereAudioSpatializer::initAudio();
    AlloSphereAudioSpatializer::initSpatialization();
    gam::Sync::master().spu(AlloSphereAudioSpatializer::audioIO().fps());
    scene()->addSource(tap);

    SearchPaths searchPaths;
    // searchPaths.addAppPaths();
    searchPaths.addSearchPath("./branchbits/sphere", false);
    cout << "Here are all the search paths:" << endl;
    searchPaths.print();

    const char *soundFileName = "branches_substrate_1.L.wav";

    string soundFilePath = searchPaths.find(soundFileName).filepath();
    if (soundFilePath == "") {
      cerr << "FAIL: your sound file " << soundFileName << " was not found in the file path." << endl;
      exit(1);
    }

    cout << "Full path to your sound file is " << soundFilePath << endl;
    if (!samplePlayer.load(soundFilePath.c_str())) {
      cerr << "FAIL: your sound file did not load." << endl;
      exit(1);
    }

    cout << "Loaded sound file; it has " << samplePlayer.size()  << " samples." << endl;
    if (samplePlayer.channels() != 1) {
      cerr << "FAIL: your sound file has " << samplePlayer.channels() 
           << " channels but I can only handle mono; sorry!"  << endl;
      exit(1);
    }

    // don't play in the beginning
    samplePlayer.reset();
    samplePlayer.phase(1.0);
  }

  ///////////////////////////////////////////////////////////////////////
  // O N C R E A T E
  ///////////////////////////////////////////////////////////////////////
  void onCreate(const ViewpointWindow& w) {
    cout<<"Loading shaders..."<<endl;

    shaderV.source(vertexCode(), Shader::VERTEX).compile().printLog();
    shaderF.source(fragmentCode(), Shader::FRAGMENT).compile().printLog();
    shaderG.source(geometryCode(), Shader::GEOMETRY).compile().printLog();

    // load shaders from files
    // SearchPaths searchPaths;
    // searchPaths.addSearchPath("./branchbits/sphere", false);
    
    // File vPointSprite(searchPaths.find("tubes.vert"), "r", true);
    // File fPointSprite(searchPaths.find("tubes.frag"), "r", true);
    // File gPointSprite(searchPaths.find("tubes.geom"), "r", true);

    // shaderV.source(vPointSprite.readAll(), Shader::VERTEX).compile().printLog();
    // shaderF.source(fPointSprite.readAll(), Shader::FRAGMENT).compile().printLog();
    // shaderG.source(gPointSprite.readAll(), Shader::GEOMETRY).compile().printLog();

    shaderP.attach(shaderF);
    shaderP.attach(shaderV);

    shaderP.setGeometryInputPrimitive(graphics().LINES);
    shaderP.setGeometryOutputPrimitive(graphics().TRIANGLE_STRIP);
    shaderP.setGeometryOutputVertices(18);
    shaderP.attach(shaderG);

    shaderP.link().printLog();

    const GLubyte* glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("glsl version: %s\n", glsl_version);

  }
  
  ///////////////////////////////////////////////////////////////////////
  // T H R E A D
  ///////////////////////////////////////////////////////////////////////
  void startThread() {
    oldPos_tree = m_tree.vertices(); // let's call it once after Trunk()
    computeThread = thread([&]() {
      while (threadDone == false ) {
        //growth iteration running away from anim step
        if (animStep >= growthIteration - growthBufferSteps) {
            Grow(state);
            oldPos_tree = m_tree.vertices(); // trxxee from last grow? ???
        }
      }
    });
  }

  ///////////////////////////////////////////////////////////////////////
  // A N I M A T E
  ///////////////////////////////////////////////////////////////////////
  virtual void onAnimate(double dt) {
    static bool waitingToAnimateNextStep = false;
    static float time(0);

    float safeToAnimate = 1.f;

    // if growing is done, message is sent to stop
    if (animPrepareToStop == true && animStep >= animStopOnStep - 1) {
      animToggle = 0;
    }
    else if (animPrepareToStop == true && animStep < animStopOnStep - 1) {
      safeToAnimate = .9;
    }
    else {
      // stop animation if anim step gets too close
      if (animStep >= growthIteration - 5) safeToAnimate = .5;  
      else if (animStep >= growthIteration - 2) safeToAnimate = .05;  
      else safeToAnimate = 1.f;
    }

    // refresh leaves- have to do this every time leaves change
    if (state->refreshLeaves == 1) {
      m_leaf.reset();
      for (int i=0; i<=LEAF_COUNT; i++) {
        m_leaf.vertex(state->leafPos[i]);
        m_leaf.color(state->leafColor[i]);

      }
      state->refreshLeaves = 0;
      state->refreshTree = 0;
    }

    dt *= animToggle;
    dt *= anim_speed;
    dt *= safeToAnimate;

    // if waiting for next anim step,
    // check if ready, if yes, procede and go to the for loop of tree vertices
    // else, just update state and wait for grow thread to go farther.
    if (waitingToAnimateNextStep) {
      if (animStep < growthIteration - 5) {
        animStep++; // and procede to next step
        waitingToAnimateNextStep = false;
      } else {
        for (int i=0; i < m_tree.vertices().size(); i++){
          state->treePos[i] = m_tree.vertices()[i];
          state->treeColor[i] = m_tree.colors()[i];
        }
        state->frame_num = frame_num;
        state->pose = nav();
        state->pSize = m_tree.vertices().size();
        state->cSize = m_tree.colors().size();
        maker.set(*state);
        return;
      }
    }
    
    time += dt;
    // animate based on two points starting in the same position. 
    // we will move one of them to its new position to form a branch.
    for (int i = 0; i < m_tree.vertices().size(); i+=2) {
      // animate only if growthIteration of branch matches animation step
      if (branchVec[i/2]->group != animStep) continue;

      // animation ends when time reaches 1
      if (time > 1) {
        m_tree.vertices()[i+1] = newPos_tree[i/2];
        continue;
      }

      m_tree.vertices()[i+1] = oldPos_tree[i+1] * (1-time) + newPos_tree[i/2] * time;
    }

    if (time > 1) {
      waitingToAnimateNextStep = true;
      time = 0;
    }

    // load results to state 
    for (int i=0; i < m_tree.vertices().size(); i++){
      state->treePos[i] = m_tree.vertices()[i];
      state->treeColor[i] = m_tree.colors()[i];
    }

    state->frame_num = frame_num;
    state->pose = nav();
    state->pSize = m_tree.vertices().size();
    state->cSize = m_tree.colors().size();
    maker.set(*state);
  }

  int frame_num = 0;
  virtual void onDraw(Graphics& g, const Viewpoint& v) {

    // draw groundplane
    if (state->drawGround == true){
      g.polygonMode(Graphics::LINE);
      g.pushMatrix();
        g.color(groundColor);
        g.translate(0,-4,0);
        g.rotate(90,1,0,0);
        g.draw(groundPlane);
      g.popMatrix();
    }

    // draw leaves
    if (state->drawLeaves == true){
      g.pointSize(2);
      g.polygonMode(Graphics::POINT);
      g.draw(m_leaf);

      // g.polygonMode(Graphics::POINT);
      // g.draw(m_root);

      // g.pointSize(15);
      // g.draw(m_tap);
    }

    // gl.matrixMode(Graphics::MODELVIEW);

    // gl.viewport(0,0, width(), height());
    // gl.matrixMode(gl.PROJECTION);
    // gl.loadMatrix(Matrix4d::perspective(45, 1., 0.1, 100));

    g.polygonMode(Graphics::FILL);
    // g.polygonMode(Graphics::POINT); // easier to debug when on

    shaderP.begin();
      shaderP.uniform("frame_num", float(frame_num));
      g.draw(m_tree);
    shaderP.end();

    frame_num++;
  }
  
  virtual void onSound(AudioIOData& io) {
    static cuttlebone::Stats fps("onSound()");
    fps(io.secondsPerBuffer());

    tap.pose(state->pose);

    while (io()) {
      // updateAudio(state->audioGain);
      float s = samplePlayer() / 2.0;
      tap.writeSample(s);
    }

    // set listener pose and render audio sources
    listener()->pose(state->pose);
    scene()->render(io);
  }

  virtual void onKeyDown(const ViewpointWindow&, const Keyboard& k){
    
    if (k.key() == 'm' ) {
      if(growthIteration == 0) startThread();
    }

    if (k.key() == ' ' ) {
      state->refreshLeaves = 1;

      if (animStep == 0) animStep++;
      if (animToggle == false) {animToggle = true; state->audioGain = 0.097; }
      else if (animToggle == true) {animToggle = false; state->audioGain = 0.0; }

      cout << "Anim Step: " << animStep << "?" << endl;
    }

    if (k.key() == 'v') {
      if (state->drawLeaves == true) state->drawLeaves = false;
      else if (state->drawLeaves == false) state->drawLeaves = true; state->refreshLeaves = 1;
    }
    if (k.key() == 'g') {
      if (state->drawGround == true) state->drawGround = false;
      else if (state->drawGround == false) state->drawGround = true;
    }
    if (k.key() == 'b') {
      if (state->drawBranches == true) state->drawBranches = false;
      else if (state->drawBranches == false) state->drawBranches = true;
    }
    
    if (k.key() == 'p' ) {
      state->print();
    }

    if (k.key() == Keyboard::RETURN) {
      if (samplePlayer.pos() >= samplePlayer.frames() - 1) {
        // samplePlayer is currently stopped at the end point, so restart
        samplePlayer.reset();
        cout << "Playing entire sample..." << endl;
      } else {
        // samplePlayer is currently playing, so shut it off
        samplePlayer.phase(1.0);
        cout << "Stop playing entire sample." << endl;
      }
    }

  }
};

int main() {
  SpaceCol app;
  app.AlloSphereAudioSpatializer::audioIO().start();  
  app.maker.start(); // it's important to call this.
  Trunk(app.state);
  app.startThread();
  app.start();
}

std::string vertexCode() {
  return R"(
void main(){
  vec4 Cd = gl_Color;
  Cd.g = 1. - (gl_Vertex.z * 0.00025)+0.1;
  // Cd.g = (pow(gl_Vertex.z, 1.2) * .00005)*.4;
  gl_FrontColor = Cd;
  // gl_FrontColor = gl_Color;
  gl_Position = gl_Vertex;
}
  )";
}

std::string fragmentCode() {
  return R"(
uniform sampler2D texSampler0;
uniform float frame_num;

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

// START THE SHADER FINALLY
void main(){

  vec4 Cd = gl_Color;

  float phase = abs(1.-Cd.r) * 1.;
  float phase_offset = frame_num * 0.01;
  float w_amp = 1.;
  float w_freq = 2.;
  // this matches sin to same one controlling brightness in frag
  Cd.r += pow(((sin((phase - phase_offset) * w_freq) + .5) * w_amp), 3.); 

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

  gl_FragColor = Cd;
  // gl_FragColor = texture2D(texSampler0, gl_TexCoord[0].xy) * gl_Color;
}
  )";
}

std::string geometryCode() {
  return R"(
#version 120
#extension GL_EXT_geometry_shader4 : enable // it is extension in 120

uniform float frame_num;

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
  // vec4 v0 = vec4(gl_PositionIn[0].xyz, 0.5);
  // vec4 v1 = vec4(gl_PositionIn[1].xyz, 0.5);
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

  vec3 rand_dir = vec3(-1.,1.,-2);


  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 2; j++){ // each incoming point
      float angle = (2 * 3.1459 * float(i)/8.);

      vec3 axis1 = normalize(cross(vtx[0].xyz - vtx[1].xyz, rand_dir)) * radius[j];
      vec3 axis2 = normalize(cross(vtx[0].xyz - vtx[1].xyz, axis1)) * radius[j];

      vec3 rot_cos = axis1 * cos(angle);
      vec3 rot_sin = axis2 * sin(angle);
      vec3 new_axis = (rot_cos + rot_sin);

      vec4 p = vec4(new_axis, 0.5);

      gl_Position = gl_ModelViewProjectionMatrix * (vtx[j] + p);

      // assign z depth to green channel for use in fragment
      vec4 Ci = gl_FrontColorIn[j];
      Ci.g = 1.-(pow(gl_Position.z, 1.5) * .1)*.1;
      gl_FrontColor = Ci;
      EmitVertex();
    }
    // Have to connect back to when angle = 0 to close the tube
    // otherwise there's a black hole on the end
    if (i == 7) {
      for (int j = 0; j < 2; j++){
        float angle = 0.;
        vec3 axis1 = normalize(cross(vtx[0].xyz - vtx[1].xyz, rand_dir)) * radius[j];
        vec3 axis2 = normalize(cross(vtx[0].xyz - vtx[1].xyz, axis1)) * radius[j];

        vec3 rot_cos = axis1 * cos(angle);
        vec3 rot_sin = axis2 * sin(angle);
        vec3 new_axis = (rot_cos + rot_sin);

        vec4 p = vec4(new_axis, 0.5);

        gl_Position = gl_ModelViewProjectionMatrix * (vtx[j] + p);

        // assign z depth to green channel for use in fragment
        vec4 Ci = gl_FrontColorIn[j];
        Ci.g = 1.-(pow(gl_Position.z, 1.5) * .1)*.1;
        gl_FrontColor = Ci;
        EmitVertex();
      }
    }
  }

  EndPrimitive();

}
  )";
}
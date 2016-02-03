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

// Main app
struct SpaceCol : App, AlloSphereAudioSpatializer, InterfaceServerClient {
  State* state;
  cuttlebone::Maker<State, 9000> maker;

  Mesh m_leaf;
  // Buffer<Mesh::Vertex> oldPos_tree;
  Mesh groundPlane;

  // Graphics gl;
  ShaderProgram shaderP;
  Shader shaderV, shaderF, shaderG;

  // thread stuff
  thread computeThread;
  // bool threadDone = false;

  SoundSource tap;
  gam::SamplePlayer<> samplePlayer0;
  gam::SamplePlayer<> samplePlayer1;

  int PLAYING = 0;

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
    lens().far(300);
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
    searchPaths.addSearchPath("./", true);
    cout << "Here are all the search paths:" << endl;
    searchPaths.print();

    const char *soundFileName0 = "branches_sound_3min_v1.wav";

    string soundFilePath0 = searchPaths.find(soundFileName0).filepath();
    if (soundFilePath0 == "") {
      cerr << "FAIL: your sound file " << soundFileName0 << " was not found in the file path." << endl;
      exit(1);
    }

    cout << "Full path to your sound file is " << soundFilePath0 << endl;
    if (!samplePlayer0.load(soundFilePath0.c_str())) {
      cerr << "FAIL: your sound file did not load." << endl;
      exit(1);
    }

    cout << "Loaded sound file; it has " << samplePlayer0.size()  << " samples." << endl;
    if (samplePlayer0.channels() != 1) {
      cerr << "FAIL: your sound file has " << samplePlayer0.channels() 
           << " channels but I can only handle mono; sorry!"  << endl;
      exit(1);
    }

    // don't play in the beginning
    samplePlayer0.reset();
    samplePlayer0.phase(1.0);

    const char *soundFileName1 = "branches_sound_3min_v2.wav";

    string soundFilePath1 = searchPaths.find(soundFileName1).filepath();
    if (soundFilePath1 == "") {
      cerr << "FAIL: your sound file " << soundFileName1 << " was not found in the file path." << endl;
      exit(1);
    }

    cout << "Full path to your sound file is " << soundFilePath1 << endl;
    if (!samplePlayer1.load(soundFilePath1.c_str())) {
      cerr << "FAIL: your sound file did not load." << endl;
      exit(1);
    }

    cout << "Loaded sound file; it has " << samplePlayer1.size()  << " samples." << endl;
    if (samplePlayer1.channels() != 1) {
      cerr << "FAIL: your sound file has " << samplePlayer1.channels() 
           << " channels but I can only handle mono; sorry!"  << endl;
      exit(1);
    }

    // don't play in the beginning
    samplePlayer1.reset();
    samplePlayer1.phase(1.0);
  }

  ///////////////////////////////////////////////////////////////////////
  // O N C R E A T E
  ///////////////////////////////////////////////////////////////////////
  void onCreate(const ViewpointWindow& w) {
    cout<<"Loading shaders..."<<endl;

    // load shaders from files
    SearchPaths searchPaths;
    searchPaths.addSearchPath("./", true);
    
    File vPointSprite(searchPaths.find("tubes.vert"), "r", true);
    File fPointSprite(searchPaths.find("tubes.frag"), "r", true);
    File gPointSprite(searchPaths.find("tubes.geom"), "r", true);

    shaderV.source(vPointSprite.readAll(), Shader::VERTEX).compile().printLog();
    shaderF.source(fPointSprite.readAll(), Shader::FRAGMENT).compile().printLog();
    shaderG.source(gPointSprite.readAll(), Shader::GEOMETRY).compile().printLog();

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
    // oldPos_tree = m_tree.vertices(); // let's call it once after Trunk()
    computeThread = thread([&]() {
      while (threadDone == false ) {
        //growth iteration running away from anim step
        if (animStep >= growthIteration - growthBufferSteps) {
            Grow(state);
            // oldPos_tree = m_tree.vertices(); // trxxee from last grow? ???
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
    float dist;
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
      // m_tree.vertices()[i+1] = oldPos_tree[i+1] * (1-time) + newPos_tree[i/2] * time;
      m_tree.vertices()[i+1] = m_tree.vertices()[i+1] * (1-time) + newPos_tree[i/2] * time;
      dist = abs(Vec3f(m_tree.vertices()[i+1] - newPos_tree[i/2]));
    }

    // if (time > 1) {
    //   waitingToAnimateNextStep = true;
    //   time = 0;
    // }

    if (dist < .001) {
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
      shaderP.uniform("toggle_fog", state->toggleFog);
      g.draw(m_tree);
    shaderP.end();

    frame_num++;
  }
  
  virtual void onSound(AudioIOData& io) {
    static cuttlebone::Stats fps("onSound()");
    fps(io.secondsPerBuffer());

    tap.pose(state->pose);

    while (io()) {

      float s = 0;
      if (PLAYING == 1) {
        s = samplePlayer0() / 50.0;
      } else if (PLAYING == 2) {
        s = samplePlayer1() / 50.0;
      }
 
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
    if (k.key() == 'y') {
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

    if (k.key() == 'g' || k.key() == 'G') {
      // present mode

      state->refreshLeaves = 1;

      if (animStep == 0) animStep++;
      if (animToggle == false) {animToggle = true; state->audioGain = 0.097; }
      else if (animToggle == true) {animToggle = false; state->audioGain = 0.0; }

      cout << "Anim Step: " << animStep << "?" << endl;

      PLAYING = 1;
      samplePlayer1.phase(1.0);
      if (samplePlayer0.pos() >= samplePlayer0.frames() - 1) {
        // samplePlayer is currently stopped at the end point, so restart
        samplePlayer0.reset();
        cout << "Playing entire sample..." << endl;
      } else {
        // samplePlayer is currently playing, so shut it off
        samplePlayer0.phase(1.0);
        cout << "Stop playing entire sample." << endl;
      }
    }

    if (k.key() == 'h' || k.key() == 'H') {
      // present mode

      state->refreshLeaves = 1;

      if (animStep == 0) animStep++;
      if (animToggle == false) {animToggle = true; state->audioGain = 0.097; }
      else if (animToggle == true) {animToggle = false; state->audioGain = 0.0; }

      cout << "Anim Step: " << animStep << "?" << endl;

      PLAYING = 2;
      samplePlayer0.phase(1.0);
      if (samplePlayer1.pos() >= samplePlayer1.frames() - 1) {
        // samplePlayer is currently stopped at the end point, so restart
        samplePlayer1.reset();
        cout << "Playing entire sample..." << endl;
      } else {
        // samplePlayer is currently playing, so shut it off
        samplePlayer1.phase(1.0);
        cout << "Stop playing entire sample." << endl;
      }
    }

    if (k.key() == '1') {
      PLAYING = 1;
      samplePlayer1.phase(1.0);
      if (samplePlayer0.pos() >= samplePlayer0.frames() - 1) {
        // samplePlayer is currently stopped at the end point, so restart
        samplePlayer0.reset();
        cout << "Playing entire sample..." << endl;
      } else {
        // samplePlayer is currently playing, so shut it off
        samplePlayer0.phase(1.0);
        cout << "Stop playing entire sample." << endl;
      }
    }

    if (k.key() == '2') {
      PLAYING = 2;
      samplePlayer0.phase(1.0);
      if (samplePlayer1.pos() >= samplePlayer1.frames() - 1) {
        // samplePlayer is currently stopped at the end point, so restart
        samplePlayer1.reset();
        cout << "Playing entire sample..." << endl;
      } else {
        // samplePlayer is currently playing, so shut it off
        samplePlayer1.phase(1.0);
        cout << "Stop playing entire sample." << endl;
      }
    }

    if (k.key() == '=' ) {
      float fov = lens().fovy() + 1;
      lens().fovy(fov);
      cout << "fov: " << fov << endl;
    }
    if (k.key() == '-' ) {
      float fov = lens().fovy() - 1;
      if (fov < 0) fov = 0;
      lens().fovy(fov);
      cout << "fov: " << fov << endl;
    }

    if (k.key() == 'f' ) {
      state->toggleFog = !state->toggleFog;
      cout << "Fog toggle " << state->toggleFog << endl;
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
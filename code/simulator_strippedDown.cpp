/* 
  Stripped down version of simulator.cpp
  
  todo:
  - loop through branches to add vertices so you can properly update width
  - construct in such a way as to modulate timing for each branch growth

*/

#include <iostream>
#include <math.h>
#include <thread>
#include <map>

#include "allocore/io/al_App.hpp"
#include "Cuttlebone/Cuttlebone.hpp"
#include "allocore/graphics/al_Mesh.hpp"

#include "common.hpp"
#include "Grow.hpp"
#include "audio.hpp"

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
  Buffer<Mesh::Vertex> oldPos_tree;
  Mesh groundPlane;

  float time = 0.0;
  float timeMod = 1.4;

  // thread stuff
  thread computeThread;
  bool threadDone = false;

  SpaceCol() :  maker(Simulator::defaultBroadcastIP()),
                InterfaceServerClient(Simulator::defaultInterfaceServerIP())  {
    cout << "-------------------" << endl;
    cout << "Space Colonization" << endl;
    cout << "-------------------" << endl;
    cout << "" << endl;

    state = new State;
    memset(state, 0, sizeof(State));
    // Initialize State parameters
    InitState(state);
    state->pose = nav();
    
    // initial camera position and far clipping plane
    nav().pos(0, 0, 0);
    lens().far(1000);
    initWindow(Window::Dim(0, 0, 600, 400), "Pineal Portal", 60);

    ///////////////////////////////////////////////////////////////////////
    // set up ground plane
    ///////////////////////////////////////////////////////////////////////
    addSurface(groundPlane, 100, 100, 100, 100);
    groundPlane.generateNormals();
    groundPlane.primitive(Graphics::TRIANGLES);
    
    ///////////////////////////////////////////////////////////////////////
    // INITIALIZE LEAVES
    ///////////////////////////////////////////////////////////////////////
    // set up leaf geo
    // m_leaf.primitive(Graphics::POINTS);
    m_leaf.primitive(Graphics::TRIANGLE_STRIP);
    m_root.primitive(Graphics::POINTS);
    
    // for (int i=0; i<MAX_LEAVES; i++) {
    //   state->leafSkip[i] = 0;
    // }

    for (int i=0; i<LEAF_COUNT; i++){
      leaves.push_back(Vec3f(0,0,0));        
      leaves[i].RandomizeTorus();

      state->currentLeafSize = LEAF_COUNT;
      state->leafPos[i] = leaves[i].Position;
      state->leafColor[i] = leafColorUnhit;

      m_leaf.vertex(state->leafPos[i]);
      m_leaf.color(state->leafColor[i]);      
    }

    state->refreshLeaves = 1;
    state->refreshTree = 1;

    ///////////////////////////////////////////////////////////////////////
    // A U D I O   S H I T
    ///////////////////////////////////////////////////////////////////////
    AlloSphereAudioSpatializer::initAudio();
    AlloSphereAudioSpatializer::initSpatialization();
    gam::Sync::master().spu(44100);
    audioSetup();
    for (int i = 0; i<numSoundSources; i++) {
      scene()->addSource(tap[i]);
    }
  }
  
  
  void startThread() {
    computeThread = thread([&]() {
      while (threadDone == false ) {
        //growth iteration running away from anim step
        if (animStep >= growthIteration - growthBufferSteps) {
            Grow(state);
            oldPos_tree = m_tree.vertices(); // tree from last grow? ???
        }
      }
    });
  }

  ///////////////////////////////////////////////////////////////////////
  // A N I M A T E
  ///////////////////////////////////////////////////////////////////////
  virtual void onAnimate(double dt) {
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
    dt *= timeMod;
    dt *= safeToAnimate;

    if (time < 0) time = 0;
    time += dt;

    // animate based on two points starting in the same position. we will move one of them
    // to its new position to form a branch.
    //
    for (int i = 0; i < m_tree.vertices().size(); i+=2) {
      // if growth iteration of branch group matches animation step
      if (branchVec[i/2].group == animStep) {
        // because group was push four times per branch to newPos_tree

        // do the animation
        if (m_tree.vertices()[i+1] != newPos_tree[i+1]) {          

          // animate the quads. we are skipping to every 4th vertex
          m_tree.vertices()[i+1] = oldPos_tree[i+1] * (1-time) + newPos_tree[i+1] * time;
          // m_tree.vertices()[i+3] = oldPos_tree[(i)+3] * (1-time) + newPos_tree[i+3] * time;
          
          // snap to final position when in range and change leaf color if appropriate
          if (abs((m_tree.vertices()[i+1] - newPos_tree[i+1]).mag()) <= .01 ) {

            // check if close enough to a leaf to change its color
            for (int j=0; j<leaves.size(); j++){
              Vec3f direction = leaves[j].Position - branchVec[i/2].Position; //should i/2 on branchVec?
              // because pos was push twice per branch to newPos
              float distance = direction.mag();

              if (distance <= minDistance) {
                state->leafColor[j] = leafColorHit;
                state->refreshLeaves = 1;

                // set freqency of this leaf's sine wave
                setLeafFreq(j);
              }
            }

            m_tree.vertices()[i+1] = newPos_tree[i+1];
            // m_tree.vertices()[i+3] = newPos_tree[i+3];

            // figuring out angles n stuff to set branch frequencies 
            float randRange = animStep * .2;
            sine[i%NUM_SINE].freq    ( 100 + animStep * rnd::uniform(1.0f, branchVec[i/2].siblings / 4.0f) );
            // sine[(i+1)%NUM_SINE].freq( 100 + animStep * rnd::uniform(1.0f, branchVec[i/2].siblings / 4.0f) );
            // because numNewBranches was pushed four times per branch
            
            float p = (float)rnd::uniform( (60/animStep) + .2, 80/animStep + .5);
            tmr.period(p);
            // cout << "freq " << i%S << ": " << sine[i%S].freq() << endl;
            // cout<<"<<"<<endl;

            animFinishedCheck++;
            cout << "anim finished check = " << animFinishedCheck << endl;
            cout << "num new branches = " << branchVec[i/2].siblings << endl;
            cout << "" << endl;
          }
        }

        state->treePos[i] = m_tree.vertices()[i];
      }

      if (branchVec[i/2].siblings == animFinishedCheck) {
        if (animStep < growthIteration - 5) {
          time = 0;
          animFinishedCheck = 0;
          animStep++;
          cout << "Anim step: " << animStep << endl;
        }
      }
    }

    // load results to state
    for (int i=0; i<m_tree.vertices().size(); i++){
      state->treePos[i] = m_tree.vertices()[i];
      state->treeColor[i] = m_tree.colors()[i];
    }

    state->t += dt;
    state->n++;
    state->pose = nav();
    state->pSize = m_tree.vertices().size();
    state->cSize = m_tree.colors().size();
    maker.set(*state);
  }

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
      g.pointSize(3);
      g.polygonMode(Graphics::POINT);
      g.draw(m_leaf);

      g.polygonMode(Graphics::POINT);
      g.draw(m_root);

      // g.pointSize(15);
      // g.draw(m_tap);
    }

    // draw "tree"
    // g.polygonMode(Graphics::POINT);
    m_tree.primitive(Graphics::LINES);
    g.draw(m_tree);
  }
  
  virtual void onSound(AudioIOData& io) {
    static cuttlebone::Stats fps("onSound()");
    fps(io.secondsPerBuffer());

    while (io()) {
      updateAudio(state->audioGain);
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

      cout << "Anim Step: " << animStep << endl;
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
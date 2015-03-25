/* 
  Stripped down version of simulator.cpp
  
  todo:
  - loop through branches to add vertices so you can properly update width
  - construct in such a way as to modulate timing for each branch growth

*/

#include "allocore/io/al_App.hpp"
#include "allocore/graphics/al_Mesh.hpp"
#include "allocore/sound/al_reverb.hpp"
#include "Cuttlebone/Cuttlebone.hpp"
#include "alloutil/al_Simulator.hpp"
#include "alloutil/al_AlloSphereAudioSpatializer.hpp"
#include <Gamma/Noise.h>
#include <Gamma/Oscillator.h>
#include <Gamma/Envelope.h>
#include <Gamma/Filter.h>
#include <Gamma/Delay.h>
#include <vector>    
#include <iostream>  
#include <math.h>
#include <map>
#include <iterator>
#include <thread>

using namespace al;
using namespace std;

#include "common.hpp"
#include "constants.cpp"
#include "Grow.cpp"

// Main app
struct SpaceCol : App, AlloSphereAudioSpatializer, InterfaceServerClient {
  
  State* state;
  cuttlebone::Maker<State, 9000> maker; 
  Buffer<Mesh::Vertex> oldPos_tree;

  Mesh groundPlane;

  float time = 0.0;
  float timeMod = 1.4;
  float soundTime = 0;

  bool mouseDown;

  // thread stuff
  thread computeThread;
  bool threadDone = false;

  // Sound stuff
  gam::Sine<> sine[S];
  gam::Sine<> leafWave[S];
  gam::LFO<> dtLFO;
  gam::Delay<> echo[SOUND_SOURCES];
  gam::OnePole<> highPassFilter;
  gam::AD<> leafAD;
  gam::AD<> branchAD;
  gam::Accum<> tmr;     // Timer for resetting envelope

  Reverb<> reverb[SOUND_SOURCES];
  float carrier[SOUND_SOURCES];

  // sound source to represent a sound in space
  SoundSource tap[SOUND_SOURCES];
  SoundSource tapOut;
  int numSoundSources;
  float f[S];

  map<int,int> majorScale;



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
    // init audio and ambisonic spatialization
    ///////////////////////////////////////////////////////////////////////
    AlloSphereAudioSpatializer::initAudio();
    AlloSphereAudioSpatializer::initSpatialization();
    gam::Sync::master().spu(44100);
    numSoundSources = SOUND_SOURCES;


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
    
    for (int i=0; i<MAX_LEAVES; i++) {
      state->leafSkip[i] = 0;
    }
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
    // cout << "test: " << AlloSphereAudioSpatializer::audioIO().fps() << endl;
    cout<< "check" <<endl;

    // set up major scale map
      majorScale[0] = 0;
      majorScale[2] = 4;
      majorScale[5] = 7;
      majorScale[9] = 11;
      // modFreq.freq(1.f/20.f);

    highPassFilter.freq(1100);
    tmr.period(1.3);

    branchAD.attack(0.02);
    branchAD.decay(1.2);

    // add our sound source to the audio scene
    for (int i = 0; i<numSoundSources; i++) {
      tap[i].pose(Pose(Vec3f( 4 * cos(M_PI*(i+1)/(numSoundSources)), 0, 
                              4 * sin(M_PI*(i+1)/(numSoundSources)) ), Quatf()));
      scene()->addSource(tap[i]);
      carrier[i] = 0.f;
      m_tap.vertex(Vec3f( 4 * cos( 2.f * 3.1415 * ((float)i/numSoundSources) ), 0, 
                          4 * sin( 2.f * 3.1415 * ((float)i/numSoundSources) ) ));
      m_tap.color(RGB(1,0,0));
      echo[i].maxDelay(.7);
      echo[i].delay(.7);
    }

    for (int i=0; i<S; i++) {
      sine[i].freq(440);
      leafWave[i].freq(0);
    }

  }
  

  void startThread(){
    // start compute thread
    static int frame = 0;
    if (threadDone == true) computeThread.join(); threadDone = false;
    LOG("compute cycle %d", frame);
    frame++;

    computeThread = thread( [&]() {
      while (threadDone == false ) {
        //growth iteration running away from anim step
        if (animStep >= growthIteration - growthBufferSteps) {
            Grow(state);
            growthIteration++;
            oldPos_tree = m_tree.vertices();
        }
      }
    } );
  }


  virtual void onAnimate(double dt) {

    float safeToAnimate;
    safeToAnimate = 1.f;

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
    int majorsc;
    float mag;


    // animate based on two points starting in the same position. we will move one of them
    // to its new position to form a branch.
    //
    for (int i = 0; i < m_tree.vertices().size(); i+=4) {
      if (newPosGroup[i] == animStep) {     // if growth iteration of branch group matches animation step

        // do the animation
        if (m_tree.vertices()[i+2] != newPos_tree[i+2]) {          

          // animate the quads. we are skipping to every 4th vertex
          m_tree.vertices()[i+2] = oldPos_tree[i+2] * (1-time) + newPos_tree[i+2] * time;
          m_tree.vertices()[i+3] = oldPos_tree[(i)+3] * (1-time) + newPos_tree[i+3] * time;
          
          // snap to final position when in range and change leaf color if appropriate
          if (abs((m_tree.vertices()[i+2] - newPos_tree[i+2]).mag()) <= .01 ) {

            // check if close enough to a leaf to change its color
            for (int j=0; j<leaves.size(); j++){
              Vec3f direction = leaves[j].Position - newPos[i];
              float distance = direction.mag();

              if (distance <= minDistance) {
                state->leafColor[j] = leafColorHit;
                state->refreshLeaves = 1;

                // set freqency of this leaf's sine wave
                leafWave[j%S].freq(220 * (j%S + 1.f * 0.5));

                // reset leaf frequencies to 0 if too many are pinging
                int leafWaveCheck = 0;
                for (int i=0; i<S; i++){
                  if (leafWave[i].freq() > 0) leafWaveCheck++;
                }
                if (leafWaveCheck >= S / 2) {
                  for (int i=0; i<S; i++) leafWave[i].freq(0.0);
                }

              }
            }

            m_tree.vertices()[i+2] = newPos_tree[i+2];
            m_tree.vertices()[i+3] = newPos_tree[i+3];

            // figuring out angles n stuff to set branch frequencies 
            float randRange = animStep * .2;

            sine[i%S].freq    ( 100 + animStep * (float)rnd::uniform(1.0f,(float)numNewBranches[i] / 4) );
            sine[(i+1)%S].freq( 100 + animStep * (float)rnd::uniform(1.0f,(float)numNewBranches[i] / 4) );
            
            float p = (float)rnd::uniform( (60/animStep) + .2, 80/animStep + .5);
            tmr.period(p);
            // cout << "freq " << i%S << ": " << sine[i%S].freq() << endl;
            // cout<<"<<"<<endl;

            animFinishedCheck++;
            cout << "anim finished check = " << animFinishedCheck << endl;
            cout << "num new branches = " << numNewBranches[i] << endl;
            cout << "" << endl;

          }
        }

        state->treePos[i] = m_tree.vertices()[i];
      }

      if (animFinishedCheck == numNewBranches[i]) {
        if (animStep < growthIteration - 5) {
          time = 0;

          animFinishedCheck = 0;

          animStep++;
          cout << "Anim step: " << animStep << endl;

        }
      }

    }

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

      // draw test tree
      g.pointSize(5);
      g.polygonMode(Graphics::FILL);
      m_test.primitive(Graphics::QUADS);
      // g.draw(m_test);
    }

    // draw "tree"
    // g.polygonMode(Graphics::POINT);
    g.polygonMode(Graphics::LINE);
    m_tree.primitive(Graphics::QUADS);
    // g.polygonMode(Graphics::FILL);
    g.draw(m_tree);

  }

  
  virtual void onSound(AudioIOData& io) {
    static cuttlebone::Stats fps("onSound()");
    fps(io.secondsPerBuffer());

    while (io()) {

      if (tmr()){
        soundTime += 0.01; if(soundTime>1) soundTime = 0;
        branchAD.lengths(soundTime, 1-soundTime);
        leafAD.lengths(soundTime*2.f, 1-soundTime*2.f);

        branchAD.reset();
        leafAD.reset();
      }
        for (int i=0; i<S; i++) {
          // carrier[i%numSoundSources] += ((sine[i]() * branchAD()) + (leafWave[i]() * leafAD()) * 0.4) / S;
          carrier[i%numSoundSources] += ((sine[i]() * branchAD())) / numSoundSources;
        }

        for (int i=0; i<numSoundSources; i++) {
          float s = carrier[i] ;
          float s_reverb;
          reverb[i].damping(.2);
          reverb[i].mix(s, s_reverb, .8);
          // tap[i].writeSample(highPassFilter( s + 0.5 * echo[i](s + echo[i]() * .05 ) / SOUND_SOURCES ));
          tap[i].writeSample(highPassFilter( s / SOUND_SOURCES ) * state->audioGain );
          carrier[i] = 0.0;
        }
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
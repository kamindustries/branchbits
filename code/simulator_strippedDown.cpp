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

// Main app
struct SpaceCol : App, AlloSphereAudioSpatializer, InterfaceServerClient {
  cuttlebone::Maker<State, 9000> maker; 
  State* state;

  Material material;
  Light light;
  Buffer<Mesh::Vertex> oldPos_tree;
  Mesh m_leaf;
  Mesh m_root;
  Mesh m_tap; 
  Mesh m_tree;
  Mesh m_test;
  Color branchColor;
  Color rootColor;
  Color leafColorUnhit;
  Color leafColorHit;

  // float time[N];
  float time;
  float timeMod;
  float soundTime;

  GroundPlane ground;
  Mesh groundPlane;

  bool mouseDown;

  bool doneGrowing;

  int leafCount;
  int leafCountBox;
  int numNewLeaves;
  float newLeafRadius;
  float treeWidth;    
  float treeHeight;   
  float trunkHeight;
  float minDistance;
  float maxDistance;
  float branchLength;
  float branchWidth;
  float maxWidthIncrement;
  float widthIncrement;
  int growthIteration;
  int growthBufferSteps;
  int animStep;
  int animFinishedCheck;
  bool animPrepareToStop;
  int animStopOnStep;
  bool animToggle;
  bool doneAnimating;
  bool dynamicLeaves;

  Color treeInitialColor;
  Color treeIncrementColor;

  // thread stuff
  //
  thread computeThread;
  bool threadDone;
  bool growthComputing;

  Vec3f rootPosition;

  vector<Leaf> leaves;
  Branch trunk;
  map <Vec3f, Branch, compare> branches;
  map<Vec3f, Branch, compare> newBranches;

  vector<Vec3f> newPos;
  vector<Vec3f> parentPos;
  vector<int> newPosGroup;
  vector<int> numNewBranches;
  vector<float> widthGroup;
  vector<Color> colorGroup;
  vector<Vec3f> newPos_tree;

  // torus stuff
  //
  float r0;
  float r1;
  float s0;
  float y0;
  Vec3f circle;
  Vec3f scramble;
  float theta;
  float phi;

  // Sound stuff
  //
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


    doneGrowing = false;

    dynamicLeaves = false;

    leafCount = N;
    leafCountBox = N2;
    numNewLeaves = 3;
    newLeafRadius = .2;
    treeWidth = 1.5;    
    treeHeight = 2.5;   
    trunkHeight = 1.2;
    minDistance = .13;
    maxDistance = 2.f;
    branchLength = .1;
    branchWidth = .2;
    maxWidthIncrement = 100.f;
    widthIncrement = .15;
    state->currentLeafSize = N;
    growthIteration = 0;
    animStep = 0;
    animFinishedCheck = 0;
    animToggle = 0;
    doneAnimating = false;
    growthBufferSteps = 15;

    animPrepareToStop = false;
    animStopOnStep = 0;

    treeInitialColor = RGB(0,0,.6);
    treeIncrementColor = RGB(0.01,.02,0);

    time = 0;
    timeMod = 1.4;
    soundTime = 0;

    threadDone = false;
    growthComputing = false;

    rootPosition = Vec3f(0,0,-4);
    branchColor = RGB(1,1,1);
    rootColor = HSV(.2,.5,.35);
    leafColorUnhit = HSV(.35,.6,.35);
    leafColorHit = HSV(.35,.9,.8);

    state->n = 0;
    state->pose = nav();
    state->nearClip = 0.1;
    state->farClip = 1000;
    state->focalLength = 6.f;
    state->eyeSeparation = 0.06;
    state->backgroundColor = Color(0.1, 1);
    state->audioGain = 0.0;
    state->drawLeaves = true;
    state->drawBranches = false;
    state->drawGround = true;
    numSoundSources = SOUND_SOURCES;

    // initial camera position and far clipping plane
    nav().pos(0, 0, 0);
    lens().far(1000);
    light.pos(0, 0, 0);

    // set up ground plane
    addSurface(groundPlane, 100, 100, 100, 100);
    groundPlane.generateNormals();
    groundPlane.primitive(Graphics::TRIANGLES);

    // set up leaf geo
    // m_leaf.primitive(Graphics::POINTS);
    m_leaf.primitive(Graphics::TRIANGLE_STRIP);
    m_root.primitive(Graphics::POINTS);
    
    r0 = 4.f;
    r1 = 1.f;
    s0 = .5;
    y0 = 2.f;

    // LEAVES
    //
    for (int i=0; i<MAX_LEAVES; i++) {
      state->leafSkip[i] = 0;
    }
    for (int i=0; i<leafCount; i++){
      leaves.push_back(Vec3f(0,0,0));        
      leaves[i].RandomizeTorus();

      state->currentLeafSize = leafCount;

      state->leafPos[i] = leaves[i].Position;
      state->leafColor[i] = leafColorUnhit;

      m_leaf.vertex(state->leafPos[i]);
      m_leaf.color(state->leafColor[i]);      
    }
    state->refreshLeaves = 1;
    state->refreshTree = 1;


    initWindow(Window::Dim(0, 0, 600, 400), "Pineal Portal", 60);

    // init audio and ambisonic spatialization
    AlloSphereAudioSpatializer::initAudio();
    AlloSphereAudioSpatializer::initSpatialization();
    gam::Sync::master().spu(44100);

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
  

  // TRUNK
  //
  void Trunk(){

    // root branch
    Branch Root(NULL, rootPosition, Vec3f(0,1,0), 0, false, branchWidth);
    // branches[Root.Position] = Root;
    
    // growing branch
    Branch current  ( &Root, rootPosition + Root.GrowDir * branchLength, Root.GrowDir, 
                      Root.GrowCount, Root.Skip, Root.Width);
    // branches[current.Position] = current;

    // stack branches vertically until trunkHeight is reached
    while ((Root.Position - current.Position).mag() < trunkHeight) {
      Branch trunk (current.Parent, current.Position + Root.GrowDir * branchLength, 
                    current.GrowDir, current.GrowCount, current.Skip, current.Width);
      branches[trunk.Position] = trunk;

      current = trunk;      
    }


    // put vertex at each trunk pos
    cout << "trunk size: " << branches.size() << endl;
    typedef map<Vec3f, Branch, compare>::iterator branchIt;
    for (branchIt iterator = branches.begin(); iterator != branches.end(); iterator++) {

      if (iterator->second.Parent != NULL){
        newPos.push_back(iterator->second.Position);
        newPos.push_back(iterator->second.Position);
        
        newPosGroup.push_back(growthIteration);
        newPosGroup.push_back(growthIteration);
        newPosGroup.push_back(growthIteration);
        newPosGroup.push_back(growthIteration);

        parentPos.push_back(iterator->second.Parent->Position);   
        parentPos.push_back(iterator->second.Parent->Position);
        parentPos.push_back(iterator->second.Parent->Position);
        parentPos.push_back(iterator->second.Parent->Position);
        
        numNewBranches.push_back(1);
        numNewBranches.push_back(1);
        numNewBranches.push_back(1);
        numNewBranches.push_back(1);

        m_root.vertex(iterator->second.Position);
        m_root.color(rootColor);
      }
    }

    state->pSize = m_root.vertices().size();

  }


  // GROW
  //
  void Grow(){

    cout << "" << endl;
    cout << "" << endl;
    cout << "Iteration # " << growthIteration << endl;
    cout << "------------" << endl;

    growthComputing = true;

    // check to see if we should add more leaves
    //
    int leavesSkipped = 0;
      for (int i=0; i<state->currentLeafSize; i++) {
        if (state->leafSkip[i] == 1) leavesSkipped++;        
      }
      cout << "num leaves skipped: " << leavesSkipped << endl;
      cout << "leaves size: " << state->currentLeafSize << endl;
      cout << "" << endl;

    if (leavesSkipped >= state->currentLeafSize * .9 && dynamicLeaves == false) {
      
      animPrepareToStop = true;
      animStopOnStep = growthIteration;

      cout << "<<" << endl;
      cout << "~~~~~~~~~Done growing!!!" << endl;
      cout << "~~~~~~~~~Preparing to stop animating!" << endl;
      cout << "~~~~~~~~~Iteration #: " << growthIteration << endl;
      cout << "<<" << endl;

      return;
    }

    // process the leaves
    //
    for (int i=0; i<leaves.size(); i++) {
      if (state->leafSkip[i] == 0) {

        vector<Leaf>::iterator leafIt= leaves.begin();
        advance(leafIt, i);

        // bool leafRemoved = false;
        leaves[i].ClosestBranch = NULL;
        Vec3f direction;

        // find nearest branch for this leaf
        //
        typedef map<Vec3f, Branch, compare>::iterator branchIt;
        for (branchIt iterator = branches.begin(); iterator != branches.end(); iterator++) {

          direction = leaves[i].Position - iterator->second.Position;
          float distance = direction.mag();
          direction.normalize();

          // skip this leaf next time if branches are too close. no more leaf growing!!
          //
          if (distance <= minDistance) {
            state->leafSkip[i] = 1;
            break;
          }
      
          // branch is in range, determine if it's the closest
          //
          else if (distance <= maxDistance) {
            if (leaves[i].ClosestBranch == NULL) {
              leaves[i].ClosestBranch = &iterator->second;
            }
            else if ((leaves[i].Position - leaves[i].ClosestBranch->Position).mag() > distance){
              leaves[i].ClosestBranch = &iterator->second;
            }
          }
          
        }
     
        // tell the branch that this leaf would like it to grow, and in what direction
        //
        if (state->leafSkip[i] == 0) {
          if (leaves[i].ClosestBranch != NULL) {
            Vec3f dir = leaves[i].Position - leaves[i].ClosestBranch->Position;
            Vec3f dirNorm = dir.normalize();
           if (dir.mag() <= maxDistance) {
              leaves[i].ClosestBranch->GrowDir += dirNorm;
              leaves[i].ClosestBranch->GrowCount += 1;
            }
          }
        }

      }
    }
    // Done processing leaves.

    // generate new branches
    //
    newBranches.clear();

    typedef map<Vec3f, Branch, compare>::iterator branchIt;
    for (branchIt iterator = branches.begin(); iterator != branches.end(); iterator++) {
      if (iterator->second.Skip == false) {

        // if at least one leaf is affecting the branch
        if (iterator->second.GrowCount > 0) {

          Vec3f avgDirection = iterator->second.GrowDir / iterator->second.GrowCount;
          avgDirection.normalize();
          
          // set grow count to 0 so the new branches don't inherit a grow count >0
          iterator->second.Reset();        

          // create a branch with the new position info
          Branch newBranch(&iterator->second, iterator->second.Position + avgDirection *
                            branchLength, avgDirection, iterator->second.GrowCount, iterator->second.Skip, 
                            iterator->second.Width);

          newBranches[newBranch.Position] = newBranch;

        }
        
        // attempting to throw out branches if most leaves are too far away, but not sure if this is
        // just making it slower. i can't do it on the first leaf loop because throwing out a branch
        // there would be too premature
        //
        // float keep = 0.f;
        // if (branches.size() / leaves.size() >= 1.f) {
        //   for (int i=0; i<leaves.size(); i++) {
        //     Vec3f direction = iterator->second.Position - leaves[i].Position ;
        //     if (direction.mag() <= maxDistance) {
        //       keep += 1.f;
        //     }
        //   }
        //   if (keep <= (float)leaves.size() * .05) {
        //     // cout << "************************** BRANCH SKIPPED~ " << endl;
        //     // cout << "************************** keep #: " << keep << endl;
        //     iterator->second.Skip = 1;
        //   }
        // }

      }

    }
    cout << "Number of new branches: " << newBranches.size() << endl;
    
    // an alternative method of writing vertex information...
    //      
    m_test.reset();
    for (branchIt iterator = branches.begin(); iterator != branches.end(); iterator++) {
      if (iterator->second.Parent != NULL){
        Vec3f crossP;
        
        Vec3f vertAhead;
        Vec3f vertBehind;
        Vec3f vertOrig;
        Vec3f vertNew;

        Vec3f vert1;
        Vec3f vert2;
        Vec3f vert3;
        Vec3f vert4;

        Vec3f move1;
        Vec3f move2;
        Vec3f move3;
        Vec3f move4;

        crossP = cross(iterator->second.Parent->Position, iterator->second.Position) / 2;

        vert1 = iterator->second.Parent->Position + crossP * branchLength * branchWidth;
        vert2 = iterator->second.Parent->Position - crossP * branchLength * branchWidth;
        vert3 = iterator->second.Position - crossP * branchLength * branchWidth;
        vert4 = iterator->second.Position + crossP * branchLength * branchWidth;

        move1 = vert1 - vert2;
        move2 = vert2 - vert1;
        move3 = vert3 - vert4;
        move4 = vert4 - vert3;

        vert1 = vert1 + move1 * branchLength * iterator->second.Width;
        vert2 = vert2 + move2 * branchLength * iterator->second.Width;
        vert3 = vert3 + move3 * branchLength * iterator->second.Width;
        vert4 = vert4 + move4 * branchLength * iterator->second.Width;

        
        m_test.vertex(vert1);
        m_test.vertex(vert2);
        m_test.vertex(vert3);
        m_test.vertex(vert4);

        m_test.color(RGB(1,1,1));
        m_test.color(RGB(1,1,1));
        m_test.color(RGB(1,1,1));
        m_test.color(RGB(1,1,1));

      }
      // cout << "m_test size: " << m_test.vertices().size() << endl;
    }
  
    cout << "branch info: " << endl;
    cout << "branches size: " << branches.size() << endl;
  
    // add new branches to tree
    bool branchAdded = false;
    for (branchIt iterator = newBranches.begin(); iterator != newBranches.end(); iterator++) {
      // if (iterator->second.Parent != NULL){
        // ^^^^^ having issues with this
      
      branches[iterator->first] = iterator->second;
      
      // having issues with hitting NULL in trunk...
      // the following is the element of the C# implementation that I want to emulate...
      //
      // while (p.Parent != NULL) {
      //   if (p.Parent->Parent != NULL){
      //     Branch next(p.Parent, p.Position, p.GrowDir, p.GrowCount, p.Skip, p.Width);
      //     p.Width += .001;
      //     p = next;
      //   }
      // }

      branches[iterator->first].Width += .001;
    
      // draw two vertices at the parent position (two makes a line), but store the new position 
      // in a separate array at the same index to be used later as an animation target
      //  
      Vec3f crossP;
      crossP = cross(iterator->second.Parent->Position, iterator->second.Position) / 2;

      m_tree.vertex(iterator->second.Parent->Position + crossP * branchLength * branchWidth);
      m_tree.vertex(iterator->second.Parent->Position - crossP * branchLength * branchWidth);
      m_tree.vertex(iterator->second.Parent->Position - crossP * branchLength * branchWidth);
      m_tree.vertex(iterator->second.Parent->Position + crossP * branchLength * branchWidth);

      newPos_tree.push_back(iterator->second.Position + crossP * branchLength * branchWidth);
      newPos_tree.push_back(iterator->second.Position - crossP * branchLength * branchWidth);
      newPos_tree.push_back(iterator->second.Position - crossP * branchLength * branchWidth);
      newPos_tree.push_back(iterator->second.Position + crossP * branchLength * branchWidth);

      m_tree.color(treeInitialColor);
      m_tree.color(treeInitialColor);
      m_tree.color(treeInitialColor);
      m_tree.color(treeInitialColor);
      widthGroup.push_back(0.001);
      widthGroup.push_back(0.001);
      widthGroup.push_back(0.001);
      widthGroup.push_back(0.001);
      colorGroup.push_back(RGB(0,0,0));
      colorGroup.push_back(RGB(0,0,0));
      colorGroup.push_back(RGB(0,0,0));
      colorGroup.push_back(RGB(0,0,0));
      
      parentPos.push_back(iterator->second.Parent->Parent->Position);   
      parentPos.push_back(iterator->second.Parent->Parent->Position);   
      parentPos.push_back(iterator->second.Parent->Parent->Position);   
      parentPos.push_back(iterator->second.Parent->Parent->Position);   

      // the array with new positions
      newPos.push_back(iterator->second.Position);
      newPos.push_back(iterator->second.Position);
      newPosGroup.push_back(growthIteration);
      newPosGroup.push_back(growthIteration);
      newPosGroup.push_back(growthIteration);
      newPosGroup.push_back(growthIteration);

      numNewBranches.push_back(newBranches.size());
      numNewBranches.push_back(newBranches.size());
      numNewBranches.push_back(newBranches.size());
      numNewBranches.push_back(newBranches.size());

      // cout << "branch at " << iterator->first << " width = " << branches[iterator->first].Width << endl;

      branchAdded = true;
      
      // }
      
      }

      // cout << "**************************************" << endl;
      // cout << "BRANCHES VERTEX INFO:" << endl;
      // for (int i = 0; i < m_tree.vertices().size(); i+=1) {
      //   cout << "vertex " << i << ": " << m_tree.vertices()[i] << endl;
      // }

    if (branchAdded == false) {
      doneGrowing = true;
      cout << "Done growing!" << endl;
    }
    cout << "Number of leaves: " << leaves.size() << endl;
    cout << "Number of branches: " << branches.size() << endl;
    cout << "Number of vertices: " << m_tree.vertices().size() << endl;

    // update tree width and color
    //
    for (int i = 0; i < m_tree.vertices().size(); i++) {
      if (widthGroup[i] < maxWidthIncrement && animToggle == true) {

        // widthGroup acts as a timer for when the branch will stop getting thicker
        widthGroup[i] += widthIncrement;

        Vec3f move;
        Vec3f move2;
        Vec3f vertAhead;
        Vec3f vertBehind;
        Vec3f vertOrig;
        Vec3f vertNew;

        vertOrig = m_tree.vertices()[i];
        vertNew = newPos_tree[i];
        vertAhead = m_tree.vertices()[i+1];
        vertBehind = m_tree.vertices()[i-1];
        move = m_tree.vertices()[i] - vertAhead;
        move2 = m_tree.vertices()[i] - vertBehind;

        // scale width by widthIncrement, also update newPos vector accordingly to be used by anim
        if (i%2==0) {
          m_tree.vertices()[i] = vertOrig + move * branchLength * widthIncrement;
          newPos_tree[i] = vertNew + move * branchLength * widthIncrement;
        }
        if (i%2==1) {
          m_tree.vertices()[i] = vertOrig + move2 * branchLength * widthIncrement;
          newPos_tree[i] = vertNew + move2 * branchLength * widthIncrement;
        }
      }

      // set min color and max/final color
      //
      m_tree.colors()[i] += RGB(treeIncrementColor);

      if (m_tree.colors()[i][0] >= 1) m_tree.colors()[i][0] = 1;
      if (m_tree.colors()[i][1] >= 1) m_tree.colors()[i][1] = 1;
      if (m_tree.colors()[i][2] >= 1) m_tree.colors()[i][2] = 1;
      if (m_tree.colors()[i][0] <= .02) m_tree.colors()[i][0] = .02;
      if (m_tree.colors()[i][1] <= .02) m_tree.colors()[i][1] = .02;
      if (m_tree.colors()[i][2] <= .02) m_tree.colors()[i][2] = .02;

    }

    growthIteration += 1;
    growthComputing = false;
 
  }

  void startThread(){
    // start compute thread
    static int frame = 0;
    if (threadDone == true) computeThread.join(); threadDone = false;
    LOG("compute cycle %d", frame);
    frame++;

    computeThread = thread( [&]() {
      while (threadDone == false ) {
        if (animStep >= growthIteration - growthBufferSteps && growthComputing == false) {
            Grow();
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
      for (int i=0; i<=leafCount; i++) {
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
    material();

    // draw groundplane
    if (state->drawGround == true){
      ground.onDraw(g,v,groundPlane);
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
  app.Trunk();
  app.startThread();
  app.start();
}
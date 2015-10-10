#pragma once

#include "allocore/io/al_App.hpp"

using namespace al;
using namespace std;

// #define LEAF_COUNT 2200  // number of leaves
#define LEAF_COUNT 6000  // number of leaves
#define STEPS 30         // number of iterations
#define NUM_VTX 200000         // max number of verts in tree mesh
#define MAX_LEAVES 20000

///////////////////////////////////////////////////////////////////////
// S T A T E   C L A S S
///////////////////////////////////////////////////////////////////////
struct State {
  double eyeSeparation, nearClip, farClip;
  double t;    // simulation time
  int frame_num; 
  unsigned n;  // "frame" number
  float focalLength;
  Pose pose;   // for navigation
  Color backgroundColor;
  
  // bool timeToggle;
  bool drawLeaves;
  bool drawBranches;
  bool drawGround;  

  Vec3f leafPos[MAX_LEAVES];    //240000
  Color leafColor[MAX_LEAVES];  //240000
  int refreshLeaves;
  int refreshTree;

  // treePos is main thing being drawn in graphics.cpp
  Vec3f treePos[NUM_VTX];             //600000
  Color treeColor[NUM_VTX];           //600000
  int pSize;                    
  int cSize;

  int currentLeafSize;

  double audioGain;             //8
  float f[LEAF_COUNT];                   //8800

  void print() {
    cout << "printin stuff from state!" << endl;
  }
};

void InitState(State* state){
  state->currentLeafSize = LEAF_COUNT;
  state->n = 0;
  state->frame_num = 0;
  state->nearClip = 0.1;
  state->farClip = 1000;
  state->focalLength = 6.f;
  state->eyeSeparation = 0.06;
  state->backgroundColor = Color(0.1, 1);
  state->audioGain = 0.0;
  state->drawLeaves = true;
  state->drawBranches = false;
  state->drawGround = true;
}
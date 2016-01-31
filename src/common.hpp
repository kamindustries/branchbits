#pragma once

#include "allocore/io/al_App.hpp"

using namespace al;
using namespace std;

#define LEAF_COUNT 6000  // number of leaves
#define STEPS 30         // number of iterations
#define NUM_VTX 200000         // max number of verts in tree mesh
#define MAX_LEAVES 20000

///////////////////////////////////////////////////////////////////////
// S T A T E   C L A S S
///////////////////////////////////////////////////////////////////////
struct State {
  double eyeSeparation, nearClip, farClip;
  double t;
  int frame_num; 
  unsigned n;
  float focalLength;
  Pose pose;
  Color backgroundColor;
  
  bool drawLeaves;
  bool drawBranches;
  bool drawGround;
  bool toggleFog; 

  Vec3f leafPos[MAX_LEAVES];
  Color leafColor[MAX_LEAVES];
  int refreshLeaves;
  int refreshTree;

  Vec3f treePos[NUM_VTX];
  Color treeColor[NUM_VTX];
  int pSize;                    
  int cSize;

  int currentLeafSize;

  double audioGain;
  float f[LEAF_COUNT];

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
  state->toggleFog = true;
}
#pragma once

#include "allocore/io/al_App.hpp"

using namespace al;
using namespace std;

const int numNewLeaves = 3;
const float anim_speed = 1.4;
const float newLeafRadius = .2;
const float treeWidth = 1.5;    
const float treeHeight = 2.5;   
const float trunkHeight = 1.;
// const float minDistance = .13;
const float minDistance = .09;
const float maxDistance = 2.5;
// const float branchLength = .1;
const float branchLength = .05;
const float branchWidth = .2;
const float maxWidthIncrement = 100.f;
const float widthIncrement = .15;
const int growthBufferSteps = 15;
const Color treeInitialColor = RGB(0,0,0);
const Color treeIncrementColor = RGB(0.005,0,0);
const Vec3f rootPosition = Vec3f(0,0,-4);
const Color branchColor = RGB(1,1,1);
const Color rootColor = HSV(.2,.5,.35);
const Color leafColorUnhit = HSV(.35,.6,.35);
const Color leafColorHit = HSV(.35,.9,.8);
const Color groundColor = HSV(.3,.1,.15);
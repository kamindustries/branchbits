#pragma once

#include "allocore/io/al_App.hpp"

const int numNewLeaves = 3;
const float newLeafRadius = .2;
const float treeWidth = 1.5;    
const float treeHeight = 2.5;   
const float trunkHeight = 1.2;
const float minDistance = .13;
const float maxDistance = 2.f;
const float branchLength = .1;
const float branchWidth = .2;
const float maxWidthIncrement = 100.f;
const float widthIncrement = .15;
const int growthBufferSteps = 15;
const Color treeInitialColor = RGB(0,0,.6);
const Color treeIncrementColor = RGB(0.01,.02,0);
const Vec3f rootPosition = Vec3f(0,0,-4);
const Color branchColor = RGB(1,1,1);
const Color rootColor = HSV(.2,.5,.35);
const Color leafColorUnhit = HSV(.35,.6,.35);
const Color leafColorHit = HSV(.35,.9,.8);
const Color groundColor = HSV(.3,.1,.15);

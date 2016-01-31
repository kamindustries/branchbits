#pragma once

#include <vector>

#include "allocore/io/al_App.hpp"
// #include "allocore/graphics/al_Mesh.hpp"

#include "common.hpp"
#include "constants.hpp"
#include "Classes.hpp"

using namespace al;
using namespace std;

vector<Branch*> branchVec;
vector<Branch*> newBranchesVec;

vector<Leaf> leaves;
vector<Vec3f> newPos_tree;

// Mesh m_root;
Mesh m_tree;

// bool dynamicLeaves = false;
bool doneGrowing = false;
bool doneAnimating = false;
bool animPrepareToStop = false;
int growthIteration = 0;
int animStep = 0;
int animToggle = 0;
// int animFinishedCheck = 0;
int animStopOnStep = 0;

///////////////////////////////////////////////////////////////////////
// T R U N K
///////////////////////////////////////////////////////////////////////

// simple solution for not losing Root. make it global.
Branch* Root;
void Trunk(State* state){
  Root = new Branch(NULL, rootPosition, Vec3f(0,1,0), -5, false, branchWidth);
  Root->group = -1;

  // just put down one branch/two verts to start
  // Branch* trunk = new Branch(Root, Root->Position + Root->GrowDir * branchLength);
  Branch* trunk = new Branch(Root, Root->Position);
  trunk->group = -1;
  trunk->siblings = 1;
  trunk->Width = branchWidth;

  // cout << (trunk->Parent)->GrowCount << endl;

  m_tree.vertex(trunk->Parent->Position);
  m_tree.vertex(trunk->Position);
  m_tree.color(treeInitialColor);
  m_tree.color(treeInitialColor);

  newPos_tree.push_back(trunk->Position); // mesh for animation
  branchVec.push_back(trunk);

  // state->pSize = m_root.vertices().size();
}

///////////////////////////////////////////////////////////////////////
// G R O W
///////////////////////////////////////////////////////////////////////

void Grow(State* state){
  cout << "\n---------- Iteration #" << growthIteration << " ----------" << endl;

  // check to see if we should add more leaves
  int leavesSkipped = 0;
  for (int i = 0; i < leaves.size(); i++) {
    if (leaves[i].skip) leavesSkipped++;
  }

  // to stop grow
  if (leavesSkipped >= state->currentLeafSize * 0.94 || doneGrowing == true) {// && dynamicLeaves == false) {
    // animPrepareToStop = true;
    // animStopOnStep = growthIteration;
    // animToggle = false;
    threadDone = true;

    cout << "<<" << endl;
    cout << "~~~~~~~~~Done growing!!!" << endl;
    cout << "~~~~~~~~~Preparing to stop animating!" << endl;
    cout << "~~~~~~~~~Iteration #" << growthIteration << endl;
    cout << "<<" << endl;
    return;
  }

  ///////////////////////////////////////////////////////////////////////
  // process the leaves : find closest branch, add dir to leaf
  ///////////////////////////////////////////////////////////////////////
  for (int i = 0; i < leaves.size(); i++) {
    Leaf& li = leaves[i];
    if (li.skip) continue;

    li.ClosestBranch = NULL;
    float curr_min_dist = maxDistance; // start with max, save anything closer
    Vec3f min_dir = Vec3f(0, 0, 0);

    // find nearest branch for this leaf
    for (int j = 0; j < branchVec.size(); j++) {
      Branch* b = branchVec[j];
      Vec3f direction = li.Position - b->Position;
      float distance = direction.mag();

      // skip next time if branches too close. no more growing for this leaf!!
      if (distance <= minDistance) {
        li.skip = true;
        break;
      }
  
      // branch is in range, determine if it's the closest
      if (distance <= curr_min_dist) {
        li.ClosestBranch = b;
        curr_min_dist = distance;
        min_dir = direction;
      }
    }
 
    // tell the closest branch gorw to this leaf
    if (!li.skip && li.ClosestBranch != NULL) {
      Vec3f dirNorm = min_dir.normalize();
      li.ClosestBranch->GrowDir += dirNorm;
      li.ClosestBranch->GrowCount += 1;
    }
  }

  ///////////////////////////////////////////////////////////////////////
  // generate new branches
  ///////////////////////////////////////////////////////////////////////
  newBranchesVec.clear();

  for (int i = 0; i < branchVec.size(); i++) {
    Branch* b = branchVec[i];

    // since we are looping through all the branches now, it's a good time to increment color
    Color plus_width = RGB(b->Width * 0.5, 0, 0);
    m_tree.colors()[i*2] += plus_width;
    m_tree.colors()[(i*2)+1] += plus_width;

    if (b->Skip) continue;

    // if at least one leaf is affecting the branch
    if (b->GrowCount > 0) {
      Vec3f avgDirection = b->GrowDir / b->GrowCount;
      avgDirection.normalize();
      
      // set grow count to 0 so the new branches don't inherit a grow count > 0
      b->Reset();

      // create a branch with the new position info
      Branch* newBranch = new Branch(b, b->Position + avgDirection * branchLength, avgDirection);
      newBranchesVec.push_back(newBranch);
    }
  }

  ///////////////////////////////////////////////////////////////////////
  // add new branches to tree
  ///////////////////////////////////////////////////////////////////////
  bool branchAdded = false;

  for (int i = 0; i < newBranchesVec.size(); i++) {
    Branch* b = newBranchesVec[i];

    m_tree.vertex(b->Parent->Position);
    m_tree.vertex(b->Parent->Position);

    m_tree.color(treeInitialColor);
    m_tree.color(treeInitialColor);
  
    newPos_tree.push_back(b->Position);

    b->Width = branchWidth;
    b->group = growthIteration;
    b->siblings = newBranchesVec.size();

    branchVec.push_back(b);
    branchAdded = true;

    Branch* p = b->Parent;

    // increase width
    // float depth = 1;
    while (p->group >= 0) {
      p->children += 1;

      float fat_value = float(p->children);
      if (fat_value >= 60) fat_value = 60;
      fat_value = 1/fat_value;
      fat_value *= 0.001;

      p->Width += fat_value;
      if (p->Width >= .0105) p->Width = .0105;

      p = p->Parent;
    }

  }

  if (branchAdded == false) {
    doneGrowing = true;
    cout << "Done growing!" << endl;
  }

  
  cout << "leaves skipped: " << leavesSkipped;
  cout << "  |  leaves: " << state->currentLeafSize << endl;
  cout << "new branches: " << newBranchesVec.size();
  cout << "  |  branches: " << branchVec.size();
  cout << "  |  vertices: " << m_tree.vertices().size();
  cout << "  |  colors: " << m_tree.colors().size() << endl;

  growthIteration++;
} // end of Grow()
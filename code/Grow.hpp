#pragma once

#include <vector>

#include "allocore/io/al_App.hpp"
#include "allocore/graphics/al_Mesh.hpp"

#include "Classes.hpp"
#include "constants.hpp"

using namespace al;
using namespace std;

vector<Branch> branchVec;
vector<Branch> newBranchesVec;

vector<Leaf> leaves;
vector<Vec3f> newPos_tree;

Mesh m_root;
Mesh m_tree;

bool dynamicLeaves = false;
bool doneGrowing = false;
bool doneAnimating = false;
bool animPrepareToStop = false;
int growthIteration = 0;
int animStep = 0;
int animToggle = 0;
int animFinishedCheck = 0;
int animStopOnStep = 0;

///////////////////////////////////////////////////////////////////////
// T R U N K
///////////////////////////////////////////////////////////////////////

// simple solution for not losing Root. make it global.
Branch Root(NULL, rootPosition, Vec3f(0,1,0), 0, false, branchWidth); 

void Trunk(State* state){

  // Set mesh to be treated as lines, only need to call this once. Turn off to render points
  m_tree.primitive(Graphics::LINES);

  // stack branches vertically until trunkHeight is reached
  Branch current(&Root, Root.Position + Root.GrowDir * branchLength);
  current.group = -1;
  current.siblings = 1;

  // skipping drawing a vertical trunk for now...
  // while ((Root.Position - current.Position).mag() < trunkHeight) {
  //   Branch trunk(current.Parent, current.Position + Root.GrowDir * branchLength);
  //   m_tree.vertex(trunk.Parent->Position);
  //   m_tree.vertex(trunk.Position);
  //   branchVec.push_back(trunk);
  //   current = trunk;
  // }

  // just put down one branch/two verts to start
  Branch trunk(current.Parent, current.Position + Root.GrowDir * branchLength);
  trunk.group = -1;
  trunk.siblings = 1;
  m_tree.vertex(trunk.Parent->Position);
  m_tree.vertex(trunk.Position);
  m_tree.color(treeInitialColor);
  m_tree.color(treeInitialColor);
  newPos_tree.push_back(trunk.Position);
  branchVec.push_back(trunk);

  // put vertex at each trunk pos
  cout << "trunk size: " << branchVec.size() << endl;
  for (int i = 0; i < branchVec.size(); i++) {
    Branch& b = branchVec[i];
    if (b.Parent != NULL){
      b.group =  growthIteration;
      b.siblings = 1;
      m_root.vertex(b.Position);
      m_root.color(rootColor);
    }
  }

  state->pSize = m_root.vertices().size();
}

///////////////////////////////////////////////////////////////////////
// G R O W
///////////////////////////////////////////////////////////////////////

void Grow(State* state){
  cout << endl << endl;
  cout << "Iteration #" << growthIteration << endl;
  cout << "------------" << endl;

  // check to see if we should add more leaves
  int leavesSkipped = 0;
  for (int i = 0; i < leaves.size(); i++) {
    if (leaves[i].skip) leavesSkipped++;
  }
  cout << "num leaves skipped: " << leavesSkipped << endl;
  cout << "leaves size: " << state->currentLeafSize << endl << endl;

  // to stop grow
  if (leavesSkipped >= state->currentLeafSize * 0.9 && dynamicLeaves == false) {
    animPrepareToStop = true;
    animStopOnStep = growthIteration;

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
    float curr_min_dist = maxDistance; // start with max dist, save anything closer
    Vec3f min_dir = Vec3f(0, 0, 0);

    // find nearest branch for this leaf
    for (int j = 0; j < branchVec.size(); j++) {
      Branch* b = &branchVec[j];
      Vec3f direction = li.Position - b->Position;
      float distance = direction.mag();

      // skip this leaf next time if branches are too close. no more leaf growing!!
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
 
    // tell the branch that this leaf would like it to grow, and in what direction
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
    Branch* b = &branchVec[i];
    if (b->Skip) continue;

    // if at least one leaf is affecting the branch
    if (b->GrowCount > 0) {
      Vec3f avgDirection = b->GrowDir / b->GrowCount;
      avgDirection.normalize();
      
      // set grow count to 0 so the new branches don't inherit a grow count > 0
      b->Reset();

      // create a branch with the new position info
      Branch newBranch(b, b->Position + avgDirection * branchLength, avgDirection);
      newBranchesVec.push_back(newBranch);
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

  cout << "Number of new branches: " << newBranchesVec.size() << endl;

  ///////////////////////////////////////////////////////////////////////
  // add new branches to tree
  ///////////////////////////////////////////////////////////////////////
  bool branchAdded = false;
  for (int i = 0; i < newBranchesVec.size(); i++) {
    
    Branch b = newBranchesVec[i];


    m_tree.vertex(b.Parent->Position);
    m_tree.vertex(b.Parent->Position);

    m_tree.color(treeInitialColor);
    m_tree.color(treeInitialColor);
    
    newPos_tree.push_back(b.Position);


    b.Width = 0.001;
    b.group = growthIteration;
    b.siblings = newBranchesVec.size();


    branchVec.push_back(b);

    branchAdded = true;
  } 

  cout << "**************************************" << endl;
  cout << "BRANCHES VERTEX INFO:" << endl;
  for (int i = 0; i < m_tree.vertices().size(); i+=1) {
    cout << "vertex " << i << ": " << m_tree.vertices()[i] << endl;
  }

  if (branchAdded == false) {
    doneGrowing = true;
    cout << "Done growing!" << endl;
  }

  cout << "Number of leaves: " << leaves.size() << endl;
  cout << "Number of branches: " << branchVec.size() << endl;
  cout << "Number of vertices: " << m_tree.vertices().size() << endl;

  ///////////////////////////////////////////////////////////////////////
  // W I D T H   &   C O L O R
  ///////////////////////////////////////////////////////////////////////

  for (int i = 0; i < m_tree.vertices().size(); i++) {
    // if (branchVec[i/4].Width < maxWidthIncrement && animToggle == true) {
    //   // Width itself acts as a timer for when the branch will stop getting thicker
    //   branchVec[i/4].Width += widthIncrement;

    //   Vec3f vertAhead = m_tree.vertices()[i+1];
    //   Vec3f vertBehind = m_tree.vertices()[i-1];
    //   Vec3f vertOrig = m_tree.vertices()[i];
    //   Vec3f vertNew = newPos_tree[i];
    //   Vec3f move2 = m_tree.vertices()[i] - vertBehind;
    //   Vec3f move = m_tree.vertices()[i] - vertAhead;

    //   // scale width by widthIncrement, also update newPos vector accordingly to be used by anim
    //   if (i % 2 == 0) {
    //     m_tree.vertices()[i] = vertOrig + move * branchLength * widthIncrement;
    //     newPos_tree[i] = vertNew + move * branchLength * widthIncrement;
    //   }
    //   if (i % 2 == 1) {
    //     m_tree.vertices()[i] = vertOrig + move2 * branchLength * widthIncrement;
    //     newPos_tree[i] = vertNew + move2 * branchLength * widthIncrement;
    //   }
    // }

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

  growthIteration++;

} // end of Grow()
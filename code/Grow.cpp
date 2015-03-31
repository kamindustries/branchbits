#include "Classes.hpp"
#include "constants.cpp"
#include "common.hpp"

vector<Branch> branchVec;
vector<Branch> newBranchesVec;

vector<Leaf> leaves;
vector<Vec3f> newPos;
vector<int> newPosGroup;
vector<int> numNewBranches;
vector<float> widthGroup;
vector<Color> colorGroup;
vector<Vec3f> newPos_tree;

Mesh m_root;
Mesh m_leaf;
Mesh m_tap; 
Mesh m_tree;
Mesh m_test;

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
void Trunk(State* state){
  Branch Root(NULL, rootPosition, Vec3f(0,1,0), 0, false, branchWidth);
  // [?] Root not pushed to our list? trunks will lost their parent! **********************
  
  // growing branch
  Branch current(&Root, Root.Position + Root.GrowDir * branchLength);

  // stack branches vertically until trunkHeight is reached
  while ((Root.Position - current.Position).mag() < trunkHeight) {
    Branch trunk(current.Parent, current.Position + Root.GrowDir * branchLength);
    branchVec.push_back(trunk);
    current = trunk;      
  }

  // put vertex at each trunk pos
  cout << "trunk size: " << branchVec.size() << endl;
  for (int i = 0; i < branchVec.size(); i++) {
    Branch b = branchVec[i];

    if (b.Parent != NULL){
      newPos.push_back(b.Position);
      newPos.push_back(b.Position);
      
      newPosGroup.push_back(growthIteration);
      newPosGroup.push_back(growthIteration);
      newPosGroup.push_back(growthIteration);
      newPosGroup.push_back(growthIteration);
      
      numNewBranches.push_back(1);
      numNewBranches.push_back(1);
      numNewBranches.push_back(1);
      numNewBranches.push_back(1);

      m_root.vertex(b.Position);
      m_root.color(rootColor);
    }
  }

  state->pSize = m_root.vertices().size();
}

///////////////////////////////////////////////////////////////////////
// GROW
///////////////////////////////////////////////////////////////////////
void Grow(State* state){
  cout << "" << endl;
  cout << "" << endl;
  cout << "Iteration #" << growthIteration << endl;
  cout << "------------" << endl;

  // check to see if we should add more leaves
  ///////////////////////////////////////////////////////////////////////
  int leavesSkipped = 0;
  for (int i=0; i<state->currentLeafSize; i++) {
    if (state->leafSkip[i] == 1) leavesSkipped++;
  }
  cout << "num leaves skipped: " << leavesSkipped << endl;
  cout << "leaves size: " << state->currentLeafSize << endl;
  cout << "" << endl;

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

  // process the leaves
  ///////////////////////////////////////////////////////////////////////
  for (int i = 0; i < leaves.size(); i++) {
    if (state->leafSkip[i] != 0) continue;

    leaves[i].ClosestBranch = NULL;
    float min_dist = maxDistance; // start with max dist, save anything closer
    Vec3f min_dir = Vec3f(0, 0, 0);

    // find nearest branch for this leaf
    ///////////////////////////////////////////////////////////////////////
    for (int j = 0; j < branchVec.size(); j++) {
      Branch* b = &branchVec[j];
      Vec3f direction = leaves[i].Position - b->Position;
      float distance = direction.mag();

      // skip this leaf next time if branches are too close. no more leaf growing!!
      ///////////////////////////////////////////////////////////////////////
      if (distance <= minDistance) {
        state->leafSkip[i] = 1;
        break;
      }
  
      // branch is in range, determine if it's the closest
      ///////////////////////////////////////////////////////////////////////
      if (distance <= min_dist) {
        leaves[i].ClosestBranch = b;
        min_dist = distance;
        min_dir = direction;
      }
    }
 
    // tell the branch that this leaf would like it to grow, and in what direction
    ///////////////////////////////////////////////////////////////////////
    if (state->leafSkip[i] == 0 && leaves[i].ClosestBranch != NULL) {
      Vec3f dirNorm = min_dir.normalize();
      leaves[i].ClosestBranch->GrowDir += dirNorm;
      leaves[i].ClosestBranch->GrowCount += 1;
    }
  }
  ///////////////////////////////////////////////////////////////////////
  // Done processing leaves.
  ///////////////////////////////////////////////////////////////////////


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
      b->Reset(); // setting also the parent's growcount to 0? ***************************

      // create a branch with the new position info
      Branch newBranch( b, b->Position + avgDirection * branchLength, avgDirection);
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
  cout << "branch info: " << endl;
  cout << "branches size: " << branchVec.size() << endl;

  // add new branches to tree
  bool branchAdded = false;
  for (int i = 0; i < newBranchesVec.size(); i++) {
    // if (iterator->second.Parent != NULL){
      // ^^^^^ having issues with this
    
    Branch b = newBranchesVec[i];
    branchVec.push_back(b);
    
    // Kee: maybe cuz we lost the root? **************************************************
    // exactly what kind of issues??

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

    branchVec[i].Width += .001;
  
    // draw two vertices at the parent position (two makes a line), but store the new position 
    // in a separate array at the same index to be used later as an animation target
    //  
    Vec3f crossP;
    crossP = cross(b.Parent->Position, b.Position) / 2.0;

    m_tree.vertex(b.Parent->Position + crossP * branchLength * branchWidth);
    m_tree.vertex(b.Parent->Position - crossP * branchLength * branchWidth);
    m_tree.vertex(b.Parent->Position - crossP * branchLength * branchWidth);
    m_tree.vertex(b.Parent->Position + crossP * branchLength * branchWidth);

    newPos_tree.push_back(b.Position + crossP * branchLength * branchWidth);
    newPos_tree.push_back(b.Position - crossP * branchLength * branchWidth);
    newPos_tree.push_back(b.Position - crossP * branchLength * branchWidth);
    newPos_tree.push_back(b.Position + crossP * branchLength * branchWidth);

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
    
    // the array with new positions
    newPos.push_back(b.Position);
    newPos.push_back(b.Position);
    newPosGroup.push_back(growthIteration);
    newPosGroup.push_back(growthIteration);
    newPosGroup.push_back(growthIteration);
    newPosGroup.push_back(growthIteration);

    numNewBranches.push_back(newBranchesVec.size());
    numNewBranches.push_back(newBranchesVec.size());
    numNewBranches.push_back(newBranchesVec.size());
    numNewBranches.push_back(newBranchesVec.size());

    // cout << "branch at " << iterator->first << " width = " << branches[iterator->first].Width << endl;

    branchAdded = true;
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
  cout << "Number of branches: " << branchVec.size() << endl;
  cout << "Number of vertices: " << m_tree.vertices().size() << endl;

  ///////////////////////////////////////////////////////////////////////
  // W I D T H   &   C O L O R
  ///////////////////////////////////////////////////////////////////////

  for (int i = 0; i < m_tree.vertices().size(); i++) {
    if (widthGroup[i] < maxWidthIncrement && animToggle == true) {

      // widthGroup acts as a timer for when the branch will stop getting thicker
      widthGroup[i] += widthIncrement;

      Vec3f vertAhead = m_tree.vertices()[i+1];
      Vec3f vertBehind = m_tree.vertices()[i-1];
      Vec3f vertOrig = m_tree.vertices()[i];
      Vec3f vertNew = newPos_tree[i];
      Vec3f move2 = m_tree.vertices()[i] - vertBehind;
      Vec3f move = m_tree.vertices()[i] - vertAhead;

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


}
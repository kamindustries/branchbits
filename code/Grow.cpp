#include "Classes.hpp"
#include "constants.cpp"
#include "common.hpp"

map <Vec3f, Branch, compare> branches;
map<Vec3f, Branch, compare> newBranches;

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

void Trunk(State* state){

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

///////////////////////////////////////////////////////////////////////
// GROW
///////////////////////////////////////////////////////////////////////

void Grow(State* state){

  cout << "" << endl;
  cout << "" << endl;
  cout << "Iteration # " << growthIteration << endl;
  cout << "------------" << endl;


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
  ///////////////////////////////////////////////////////////////////////
  // Done processing leaves.
  ///////////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////////
  // generate new branches
  ///////////////////////////////////////////////////////////////////////
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

  ///////////////////////////////////////////////////////////////////////
  // W I D T H   &   C O L O R
  ///////////////////////////////////////////////////////////////////////

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

}
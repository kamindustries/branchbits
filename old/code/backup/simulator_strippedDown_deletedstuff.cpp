 // removed "box" stuff which was the line i started out with:
 
  Mesh box;
  Buffer<Mesh::Vertex> oldPos;

    for (branchIt iterator = branches.begin(); iterator != branches.end(); iterator++) {

      if (iterator->second.Parent != NULL){
        box.vertex(iterator->second.Parent->Position);
        box.vertex(iterator->second.Parent->Position);



 state->pSize = box.vertices().size();

    for (int i=0; i<box.vertices().size(); i++) {
      state->treePos[i] = box.vertices()[i];
      // state->treeColor[i] = box.colors()[i];
      // state->treePos[i] = m_tree.vertices()[i];
    }


    for (branchIt iterator = newBranches.begin(); iterator != newBranches.end(); iterator++) {
      box.vertex(iterator->second.Parent->Position);
      box.vertex(iterator->second.Parent->Position);


startthread(){
  oldPos = box.vertices();
}


    draw branches
    if (state->drawBranches == true){
      box.primitive(Graphics::LINES);
      g.polygonMode(Graphics::LINE);
      g.color(branchColor);
      g.lineWidth(1.f);
      g.draw(box);
    }



      
      if (leavesSkipped >= state->currentLeafSize * .9 && dynamicLeaves == true) {
        cout << "more leaves!" << endl;
        moreLeaves();
      }



  void moreLeaves() {
    cout << "<<" << endl;
    cout << "Adding more leaves..." << endl;

    float radius = newLeafRadius;
    int sumNewLeaves = 0;
    int oldNumLeaves = leaves.size();

    typedef map<Vec3f, Branch, compare>::iterator branchIt;
    for (branchIt iterator = newBranches.begin(); iterator != newBranches.end(); iterator++) {
    
    Vec3f avgDirection = iterator->second.GrowDir;
    avgDirection.normalize();

    Vec3f pos = iterator->second.Position;
    Vec3f newPos;
    newPos = iterator->second.Position + avgDirection * branchLength;

      for (int i=0; i<=numNewLeaves; i++) {
        Vec3f location;
        location = Vec3f( (float)rnd::uniform(-radius,+radius),
                          (float)rnd::uniform(-radius,+radius),
                          (float)rnd::uniform(-radius,+radius));
        location = Vec3f(location[0]+newPos[0],location[1]+newPos[1],location[2]+newPos[2]);
        leaves.push_back(Leaf(location));

        sumNewLeaves++;
      }
    }

    for (int i=oldNumLeaves; i<leaves.size(); i++) {
      // cout << "leaf " << i << " position: " << leaves[i].Position << endl;
      state->leafColor[i] = leafColorHit;
      state->leafPos[i] = leaves[i].Position;
      state->leafSkip[i] = 0;

      m_leaf.vertex(state->leafPos[i]);
      m_leaf.color(state->leafColor[i]);
    }

    state->currentLeafSize = leaves.size();

    cout << "Done adding leaves!" << endl;
    cout << "<<" << endl;
  }
  

  void leafTorus(){
    for (int i=0; i<MAX_LEAVES; i++) {
      state->leafSkip[i] = 0;
      state->leafPos[i] = Vec3f(0,0,0);
    }

    for (int i=0; i<leafCount; i++){
      leaves.push_back(Vec3f(0,0,0));        
      leaves[i].RandomizeTorus();

      state->leafPos[i] = leaves[i].Position;
      state->leafColor[i] = leafColorUnhit;

      m_leaf.vertex(state->leafPos[i]);
      m_leaf.color(state->leafColor[i]);      
    }
    state->currentLeafSize = leafCount;
    state->refreshLeaves = 1;
  }

  void leafTrefoil(){
    for (int i=0; i<MAX_LEAVES; i++) {
      state->leafSkip[i] = 0;
      state->leafPos[i] = Vec3f(0,0,0);
    }

    for (int i=0; i<leafCount; i++){
      leaves.push_back(Vec3f(0,0,0));        
      leaves[i].RandomizeTrefoil();

      state->leafPos[i] = leaves[i].Position;
      state->leafColor[i] = leafColorUnhit;

      m_leaf.vertex(state->leafPos[i]);
      m_leaf.color(state->leafColor[i]);      
    }
    state->currentLeafSize = leafCount;
    state->refreshLeaves = 1;
  }

  void leafBox(){
    for (int i=0; i<MAX_LEAVES; i++) {
      state->leafSkip[i] = 0;
      state->leafPos[i] = Vec3f(0,0,0);
    }

    for (int i=0; i<=leafCountBox; i++) {
      Vec3f location;
      location = Vec3f( (float)rnd::uniform(-treeWidth,treeWidth),
                        (float)rnd::uniform(trunkHeight,treeHeight),
                        (float)rnd::uniform(-treeWidth - 4.f,treeWidth - 4.f));
      leaves.push_back(Leaf(location));
      state->leafColor[i] = leafColorUnhit;
      state->leafPos[i] = location;
      m_leaf.vertex(state->leafPos[i]);
      m_leaf.color(state->leafColor[i]);
    }
    state->currentLeafSize = leafCountBox;
    state->refreshLeaves = 1;
  }

  void resetAll(){  
    cout << "<<" << endl;
    cout << "~~~~~~~~~Resetting..." << endl;
      threadDone = true;
    cout << "Killed thread." << endl;   
      animToggle = false;
      timeMod = 0;
      time = 0;
      growthIteration = 0;
      animStep = 0;
      animFinishedCheck = 0;
    cout << "Reset clocks." << endl;
      box.reset();
      m_tree.reset();
      m_leaf.reset();
    cout << "Reset meshes." << endl;

      newPos.clear();
      newPos_tree.clear();
      parentPos.clear();
      newPosGroup.clear();
      newPosGroup_tree.clear();
      numNewBranches.clear();
      widthGroup.clear();
      colorGroup.clear();
      oldPos.reset();
      oldPos_tree.reset();
    cout << "Cleared vectors and other variables." << endl;

      leaves.clear();
      cout << "leaves cleared" << endl;

    // typedef map<Vec3f, Branch, compare>::iterator branchIt;
    // for (branchIt iterator = branches.begin(); iterator != branches.end(); iterator++) {
    //   branches.erase(iterator);
    // }
      branches.clear();
      cout << "branches cleared" << endl;
    cout << "Cleared maps." << endl;

      for (int i=0; i<MAX_LEAVES; i++) {
        state->leafSkip[i] = 0;
      }
    cout << "Set leaf skip to 0" << endl;
 

    cout << "~~~~~~~~~Reset done!" << endl;
    cout << "<<" << endl;
  }

// KEYBOARD STUFF
//
virtual void onKeyDown(const ViewpointWindow&, const Keyboard& k){

  if (k.key() == '.') {
      cout << "Reset branches." << endl;
      branches.clear();
      newBranches.clear();
      newPos.clear();
      newPos_tree.clear();
      parentPos.clear();
      newPosGroup.clear();
      newPosGroup_tree.clear();
      numNewBranches.clear();
      widthGroup.clear();
      colorGroup.clear();
      oldPos.reset();
      oldPos_tree.reset();
      box.reset();
      m_tree.reset();
      growthIteration = 0;
      animStep = 0;
      animFinishedCheck = 0;
      animToggle = false;
      time = 0;
      Trunk();
    }


    if (k.key() == 'n') {
      leaves.clear();
      leafBox();
    }

    // new torus
    //
    if (k.key() == 't') {
      leaves.clear();
      leafTorus();
    }
     
    // toggle dynamic leaves                          
    if (k.key() == 'o') {
      if (dynamicLeaves == true) {
        dynamicLeaves = false;
        animToggle = false;
        cout << "Dynamic leaves are off. " << endl;
        cout << "Animation turned off. " << endl;
      }
      else if (dynamicLeaves == false) {
        dynamicLeaves = true;
        cout << "Dynamic leaves are on! " << endl;
        cout << "Animation turned on! " << endl;
        moreLeaves();
        animPrepareToStop = false;
        animToggle = true;
      }
    }

    if (k.key() == 'i' ) {
      state->eyeSeparation+=.002;
      cout << "eye separation: " << state->eyeSeparation << endl;
    }

    if (k.key() == 'u' ) {
      state->eyeSeparation-=.002;
      cout << "eye separation: " << state->eyeSeparation << endl;
    }
    if (k.key() == 'k' ) {
      state->focalLength+=.1;
      cout << "focal length: " << state->focalLength << endl;
    }    
    if (k.key() == 'j' ) {
      state->focalLength-=.1;
      cout << "focal length: " << state->focalLength << endl;
    }

    // PRESETS
    //
    if (k.key() == '1' ) {
      cout<<""<<endl;
      cout << "Preset #1: THE TORUS" << endl;
      cout <<""<<endl;
      resetAll();
      minDistance = .13;
      maxDistance = 2.f;
      branchLength = .1;
      branchWidth = .2;
      maxWidthIncrement = 100.f;
      widthIncrement = 0.15;

      growthIteration = 0;
      animStep = 0;
      animFinishedCheck = 0;
      animToggle = 0;
      doneAnimating = false;
      growthBufferSteps = 15;

      dynamicLeaves = false;

      rootPosition = Vec3f(0,0,-4);

      treeInitialColor = RGB(0,0,.6);
      treeIncrementColor = RGB(0.01,.02,0.0);

      timeMod = .7;
      leafTorus();
      Trunk();
      startThread();
      sleep(2);
    }
    if (k.key() == '2' ) {
      cout<<""<<endl;
      cout << "Preset #2: THE KNOT" << endl;
      cout <<""<<endl;
      resetAll();
      minDistance = .16;
      maxDistance = 1.7;
      branchLength = .2;
      branchWidth = .1;
      maxWidthIncrement = 5.f;
      widthIncrement = 0.1;

      growthIteration = 0;
      animStep = 0;
      animFinishedCheck = 0;
      animToggle = 0;
      doneAnimating = false;
      growthBufferSteps = 15;

      leafCount = 1000;
      dynamicLeaves = false;
      
      rootPosition = Vec3f(1,1,0);

      treeInitialColor = RGB(.97,.45,0);
      treeIncrementColor = RGB(-.01,-.005,0.01);

      timeMod = 2.f;

      nav().pos(0, 0, 6);
      state->pose = nav();
      cout << "New parameters set: preset 2" << endl;

      leafTrefoil();
      cout << "Trefoil leaves scattered." << endl;

      Trunk();
      cout << "New trunk created." << endl;

      startThread();
      cout << "Thread restarted." << endl;

      sleep(2);
    }
    if (k.key() == '4' ) {
      cout<<""<<endl;
      cout << "Preset #2: THE SMALL BOX" << endl;
      cout <<""<<endl;
      resetAll();
      treeWidth = 1.f;    
      treeHeight = 2.f;   
      trunkHeight = 1.2;
      minDistance = .1;
      maxDistance = 2.f;
      branchLength = .05;
      branchWidth = .3;
      maxWidthIncrement = 30.f;
      widthIncrement = .75;
      leafCountBox = 700;

      growthIteration = 0;
      animStep = 0;
      animFinishedCheck = 0;
      animToggle = 0;
      doneAnimating = false;
      growthBufferSteps = 25;

      treeInitialColor = RGB(0.0,1.f,0.1);
      treeIncrementColor = RGB(0.008,-.01,0.006);

      dynamicLeaves = true;

      rootPosition = Vec3f(0,0,-4);

      timeMod = 2.f;

      leafBox();
      Trunk();

      startThread();

      sleep(2);
    }
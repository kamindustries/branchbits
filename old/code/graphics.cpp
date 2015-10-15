#include <vector>    // vector
#include <string>    // memcpy

#include "Cuttlebone/Cuttlebone.hpp"
#include "osgr.hpp"

using namespace al;
using namespace std;

#include "common.hpp"

struct Testing : OmniStereoGraphicsRenderer1 {
  cuttlebone::Taker<State, 9000> taker;
  State* state;

  Mesh m;
  Mesh tree;

  Testing() {
    state = new State();
  }

  virtual ~Testing() {}

  void setup() {
    omni().stereo(true);

    for (int i = 0; i < NUM_VTX; i++) {
      tree.vertex(0,0,0);
      tree.color(0,0,0);
    }

    m.reset();
    m.primitive(graphics().LINES);
    tree.primitive(graphics().LINES);
    // m.primitive(graphics().TRIANGLE_STRIP);
    // int N = addSphere(m, 1, 32, 32);
    // for(int i=0; i<N; ++i){
    //   m.color(HSV(i / (float)N, 0.5, 1.0));
    // }
  
    m.vertex(40, -6, -6);
    m.vertex(15, 0, 3);

    m.vertex(40, 12, 0);
    m.vertex(15, 6, 6);
    
    m.vertex(15, 6, 6);
    m.vertex(40, -9, -3);

    m.color(1, 0, 0);
    m.color(0, 1, 0);

    m.color(0, 0, 1);
    m.color(1, 1, 0);

    m.color(0, 1, 1);
    m.color(1, 0, 1);
  }

  virtual void onAnimate(double dt) {
    static bool first_frame(true);
    if (first_frame) {
      setup();
      first_frame = false;
    }

    int popCount = taker.get(*state);

    memcpy(&tree.vertices()[0], &state->treePos[0], sizeof(Vec3f) * state->pSize);
    memcpy(&tree.colors()[0], &state->treeColor[0], sizeof(Color) * state->cSize);

    pose = state->pose;
  }

  virtual void onDraw(Graphics& g) {
    static float theta(0);
    
    omni().clearColor() = state->backgroundColor;
    omni().sphereRadius(state->focalLength);
    lens().near(state->nearClip);
    lens().far(state->farClip);
    lens().eyeSep(state->eyeSeparation);
    

    // omni().clearColor() = RGB(0, 0, 0);
    // omni().sphereRadius(30);
    // lens().near(10);
    // lens().far(100);
    // lens().eyeSep(0);

    g.polygonMode(Graphics::FILL);
    g.depthTesting(true);

    g.pushMatrix();
    // g.rotate(theta, 0, 1, 0);
    // g.translate(2, 0, 0);
    shader().uniform("frame_num", float(state->frame_num)); //gives us our custom shader
    g.draw(tree);
    // g.draw(m); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    g.popMatrix();


    theta += 0.5;
    if (theta > 360) {
      theta = 0;
    }
  }

  virtual void start() {
    taker.start();
    OmniStereoGraphicsRenderer1::start();
  }
};

int main() { Testing().start(); }



// #include "Cuttlebone/Cuttlebone.hpp"
// #include "alloutil/al_OmniStereoGraphicsRenderer.hpp"
// #include "allocore/graphics/al_Mesh.hpp"
// #include "allocore/math/al_Random.hpp"
// #include "allocore/io/al_App.hpp"
// #include <vector>    // vector
// #include <string>    // memcpy
// #include <iostream>  // cerr

// using namespace al;
// using namespace std;

// #include "common.hpp"

// struct Render : OmniStereoGraphicsRenderer {
//   cuttlebone::Taker<State, 9000> taker;
//   State* state;
//   Material material;
//   Light light;
//   Mesh tree;
//   Mesh m_leaf;
//   Mesh groundPlane;
//   Color groundColor;
//   Color branchColor;
//   Color leafColor;

//   int clickNum;
//   int leafCount;

//   bool onAnimateCheck;
//   bool onDrawCheck;

//   Render() {
//     state = new State;

//     onAnimateCheck = false;
//     onDrawCheck = false;

//     // set up leaf geo
//     // addSphere(m_leaf, 0.5, 16, 16);
//     m_leaf.primitive(Graphics::POINTS);
//     state->currentLeafSize = LEAF_COUNT;
//     state->refreshLeaves = 1;
//     state->refreshTree = 1;

//     // set up ground plane
//     addSurface(groundPlane, 100, 100, 100, 100);
//     groundPlane.generateNormals();
//     groundPlane.primitive(Graphics::TRIANGLES);

//     branchColor = RGB(1,1,1);

//     tree.primitive(Graphics::LINES);

//     for (int i=0; i<NUM_VTX; i++){
//       tree.vertex(0,0,0);
//       tree.color(0,0,0);
//     }

//     state->pSize = 0;
//     state->cSize = 0;
//     state->drawLeaves = true;
//     state->drawGround = false;
//     state->currentLeafSize = LEAF_COUNT;

//     for (int i=0; i<=state->currentLeafSize; i++) {
//       m_leaf.vertex(state->leafPos[i]);
//       m_leaf.color(state->leafColor[i]);
//     }

//     cout << "Graphics constructed." << endl;

//   }


//   virtual ~Render() {}

//   virtual void onAnimate(double dt) {
//     if (onAnimateCheck == false) {
//       cout << "Animation started." << endl;
//       onAnimateCheck = true;
//     }

//     int popCount = taker.get(*state);
//     if (state->refreshTree == 1) {
//       state->refreshTree = 0;
//       for (int i=0; i<NUM_VTX; i++){
//         tree.vertex(0,0,0);
//         tree.color(0,0,0);
//       }
//     }
//     state->refreshTree = 0;

//     // have to do this every time leaves change
//     if (state->refreshLeaves == 1) {
//       m_leaf.reset();
//       for (int i=0; i<=state->currentLeafSize; i++) {
//         m_leaf.vertex(state->leafPos[i]);
//         m_leaf.color(state->leafColor[i]);
//       }
//       state->refreshLeaves = 0;
//     }
    
//     memcpy(&tree.vertices()[0], &state->treePos[0], sizeof(Vec3f) * state->pSize); 
//     memcpy(&tree.colors()[0], &state->treeColor[0], sizeof(Color) * state->cSize); 

//     pose = state->pose;

//   }

//   virtual void onDraw(Graphics& g) {

//     if (onDrawCheck == false) {
//       cout << "Draw started." << endl;
//       onDrawCheck = true;
//     }

//     shader().uniform("lighting", 0.0f);

//     omni().clearColor() = state->backgroundColor;
//     omni().sphereRadius(state->focalLength);
//     lens().near(state->nearClip);
//     lens().far(state->farClip);
//     lens().eyeSep(state->eyeSeparation);


//     if (state->drawLeaves == true){
//         g.polygonMode(Graphics::POINT);
//         g.pointSize(6);
//         g.draw(m_leaf);
//     }

//     if (state->drawGround == true){
//       // draw groundplane
//       groundColor = HSV(.3,.1,.15);
//       g.polygonMode(Graphics::LINE);
//       g.pushMatrix();
//         g.color(groundColor);
//         g.translate(0,-4,0);
//         g.rotate(90,1,0,0);
//         g.draw(groundPlane);
//       g.popMatrix();

//       g.pushMatrix();
//         g.color(groundColor);
//         g.translate(0,4,0);
//         g.rotate(90,1,0,0);
//         g.draw(groundPlane);
//       g.popMatrix();
//     }


//     g.polygonMode(Graphics::FILL);
//     g.draw(tree);
    

//   }

//   virtual void start() {
//     cout << "Running graphics..." << endl;
//     taker.start();
//     cout << "starting omni..." << endl;
//     OmniStereoGraphicsRenderer::start();
//     cout << "omni started." << endl;

//   }
// };

// int main() { Render().start(); }

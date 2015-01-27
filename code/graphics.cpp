#include "Cuttlebone/Cuttlebone.hpp"
#include "alloutil/al_OmniStereoGraphicsRenderer.hpp"
#include "allocore/graphics/al_Mesh.hpp"
#include "allocore/math/al_Random.hpp"
#include "allocore/io/al_App.hpp"
#include <vector>    // vector
#include <string>    // memcpy
#include <iostream>  // cerr

using namespace al;
using namespace std;

#include "common.hpp"


struct Render : OmniStereoGraphicsRenderer {
  cuttlebone::Taker<State, 9000> taker;
  State* state;
  Material material;
  Light light;
  Mesh tree;
  Mesh m_leaf;
  Mesh groundPlane;
  Color groundColor;
  Color branchColor;
  Color leafColor;

  int clickNum;
  int leafCount;

  bool onAnimateCheck;
  bool onDrawCheck;


  Render() {
    state = new State;

    onAnimateCheck = false;
    onDrawCheck = false;

    // set up leaf geo
    // addSphere(m_leaf, 0.5, 16, 16);
    m_leaf.primitive(Graphics::POINTS);
    state->currentLeafSize = N;
    state->refreshLeaves = 1;
    state->refreshTree = 1;

    // set up ground plane
    addSurface(groundPlane, 100, 100, 100, 100);
    groundPlane.generateNormals();
    groundPlane.primitive(Graphics::TRIANGLES);

    branchColor = RGB(1,1,1);

    tree.primitive(Graphics::QUADS);

    for (int i=0; i<V; i++){
      tree.vertex(0,0,0);
      tree.color(0,0,0);
    }

    state->pSize = 0;
    state->cSize = 0;
    state->drawLeaves = true;
    state->drawGround = false;
    state->currentLeafSize = N;

    for (int i=0; i<=state->currentLeafSize; i++) {
      m_leaf.vertex(state->leafPos[i]);
      m_leaf.color(state->leafColor[i]);
    }

    cout << "Graphics constructed." << endl;

  }


  virtual ~Render() {}

  virtual void onAnimate(double dt) {
    if (onAnimateCheck == false) {
      cout << "Animation started." << endl;
      onAnimateCheck = true;
    }

    int popCount = taker.get(*state);
    if (state->refreshTree == 1) {
      state->refreshTree = 0;
      for (int i=0; i<V; i++){
        tree.vertex(0,0,0);
        tree.color(0,0,0);
      }
    }
    state->refreshTree = 0;

    // have to do this every time leaves change
    if (state->refreshLeaves == 1) {
      m_leaf.reset();
      for (int i=0; i<=state->currentLeafSize; i++) {
        m_leaf.vertex(state->leafPos[i]);
        m_leaf.color(state->leafColor[i]);
      }
      state->refreshLeaves = 0;
    }
    
    memcpy(&tree.vertices()[0], &state->treePos[0], sizeof(Vec3f) * state->pSize); 
    memcpy(&tree.colors()[0], &state->treeColor[0], sizeof(Color) * state->cSize); 

    pose = state->pose;

  }

  virtual void onDraw(Graphics& g) {

    if (onDrawCheck == false) {
      cout << "Draw started." << endl;
      onDrawCheck = true;
    }

    shader().uniform("lighting", 0.0f);

    omni().clearColor() = state->backgroundColor;
    omni().sphereRadius(state->focalLength);
    lens().near(state->nearClip);
    lens().far(state->farClip);
    lens().eyeSep(state->eyeSeparation);


    // light.pos(0, 1, 0);

    if (state->drawLeaves == true){
        g.polygonMode(Graphics::POINT);
        g.pointSize(6);
        g.draw(m_leaf);
    }

    if (state->drawGround == true){
      // draw groundplane
      groundColor = HSV(.3,.1,.15);
      g.polygonMode(Graphics::LINE);
      g.pushMatrix();
        g.color(groundColor);
        g.translate(0,-4,0);
        g.rotate(90,1,0,0);
        g.draw(groundPlane);
      g.popMatrix();

      g.pushMatrix();
        g.color(groundColor);
        g.translate(0,4,0);
        g.rotate(90,1,0,0);
        g.draw(groundPlane);
      g.popMatrix();
    }

      tree.primitive(Graphics::QUADS);
      g.polygonMode(Graphics::FILL);
      g.draw(tree);
    

  }

  virtual void start() {
    cout << "Running graphics..." << endl;
    taker.start();
    cout << "starting omni..." << endl;
    OmniStereoGraphicsRenderer::start();
    cout << "omni started." << endl;

  }
};

int main() { Render().start(); }

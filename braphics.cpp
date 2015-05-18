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

    pose = state-pose;
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
    g.draw(tree);
    g.draw(m); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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

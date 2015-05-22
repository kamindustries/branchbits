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

    tree.primitive(graphics().LINES);
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
    shader().uniform("frame_num", float(state->frame_num)); //gives us our custom shader
    g.draw(tree);
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
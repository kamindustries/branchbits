#include <vector>    // vector
#include <string>    // memcpy

#include "BranchRenderer.hpp"
#include "Cuttlebone/Cuttlebone.hpp"

using namespace al;
using namespace std;

#include "common.hpp"

struct BranchGraphics : BranchRenderer {
  cuttlebone::Taker<State, 9000> taker;
  State* state;
  Mesh tree;

  virtual bool onCreate() {
    BranchRenderer::onCreate();
    state = new State();

    for (int i = 0; i < NUM_VTX; i++) {
      tree.vertex(0,0,0);
      tree.color(0,0,0);
    }
    tree.primitive(graphics().LINES);

    shader().uniform("lighting", 0.0);
    shader().uniform("texture", 0.0);

    return true;
  }

  virtual void onAnimate(double dt) {
    int popCount = taker.get(*state);

    memcpy(&tree.vertices()[0], &state->treePos[0], sizeof(Vec3f) * state->pSize);
    memcpy(&tree.colors()[0], &state->treeColor[0], sizeof(Color) * state->cSize);

    pose = state->pose;
  }

  virtual void onDraw(Graphics& g) {
    g.polygonMode(Graphics::FILL);
    g.depthTesting(true);

    g.pushMatrix();
    shader().uniform("frame_num", float(state->frame_num));
    g.draw(tree);
    g.popMatrix();
  }

  virtual void start() {
    taker.start();
    BranchRenderer::start();
  }
};

int main() { BranchGraphics().start(); }
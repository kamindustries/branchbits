/*  
  ~Common stuff~
*/

#include "allocore/io/al_App.hpp"
#include "allocore/io/al_Window.hpp"
#include <Gamma/Oscillator.h>

#define N 2200         // number of leaves
#define N2 400         // number of leaves for small box
#define STEPS 30      // number of iterations
#define S 20         // number of sine waves
#define SOUND_SOURCES 4    // number of sound sources
#define V 100000       // max number of verts in tree mesh
#define MAX_LEAVES 20000

// template <typename M> void FreeClear( M & amap );
//     for ( typename M::iterator it = amap.begin(); it != amap.end(); ++it ) {
//         delete it->second;
//     }
//     amap.clear();
// }

// comparator for Vec3f
struct compare {
    bool operator() (Vec3f a, Vec3f b) const {
        return a.mag() < b.mag();
    }
};

struct Branch {
  Branch* Parent;
  int GrowCount;
  Vec3f Position;
  Vec3f GrowDir;
  Vec3f OrigGrowDir;
  bool Skip;
  float Width;

  Branch(){};
  Branch( Branch* _parent, const Vec3f& _position, const Vec3f& _growDirection, 
          int _growCount, bool _skip, float _width ) {
    Parent = _parent;
    Position = _position;
    GrowDir = _growDirection;
    OrigGrowDir = _growDirection;
    GrowCount = _growCount;
    Skip = _skip;
    Width = _width;
  }

  void Reset () {
    GrowCount = 0;
    GrowDir = OrigGrowDir;
  }

};


struct Leaf {
  Vec3f Position;
  Branch* ClosestBranch;
  Material material;
  Light light;
  Color color;
  Vec3f location;
  Vec3f circle;
  Vec3f scramble;
  float r0;
  float r1;
  float theta;
  float phi;
  float s;


  Leaf(const Vec3f& _position) {
    Position = _position;
  }

  void onDraw(Graphics& g, const Viewpoint&, Mesh &m_leaf) {
    color = HSV(.35,.9,.8);
    material();
    g.polygonMode(Graphics::FILL);
    g.pushMatrix();
    g.color(color);
    g.translate(Position);
    g.scale(.05);
    g.draw(m_leaf);
    g.popMatrix();
  }
  
  void Randomize() {
    location = Vec3f( (float)rnd::uniform(-3.f,3.f),
                      (float)rnd::uniform(-3.f,3.f),
                      (float)rnd::uniform(-3.f,3.f));
    Position = location;  
  }

  void RandomizeTorus() {
    r0 = 3.8;
    r1 = 1.6;
    theta = (float)rnd::uniform(0.f, 2.f);
    theta *=  M_PI;
    phi = (float)rnd::uniform(0.f, 2.f);
    phi *= M_PI;
    s = .8;
    scramble = Vec3f( (float)rnd::uniform(-s,s),
                      (float)rnd::uniform(-s,s),
                      (float)rnd::uniform(-s,s));


    circle = Vec3f( cos(theta) * (r0 + r1 * cos(phi)),
                    r1 * sin(phi) * 2.f,
                    sin(theta) * (r0 + r1 * cos(phi)));

    // circle offset
    circle += scramble; 
    circle = Vec3f(circle[0], circle[1], circle[2]);
    Position = circle;
    }

  void RandomizeTrefoil() {
    float a, d, p, q;
    a = 2.f;
    d = 4.f;
    p = 2.f;
    q = 3.f;
    theta = (float)rnd::uniform(0.f, 2.f);
    theta *=  M_PI;
    phi = (float)rnd::uniform(0.f, 2.f);
    phi *= M_PI;
    s = .8;
    scramble = Vec3f( (float)rnd::uniform(-s,s),
                      (float)rnd::uniform(-s,s),
                      (float)rnd::uniform(-s,s));




    circle = Vec3f( (a * sin(q*theta) + d) * sin(p*theta),
                    (a * sin(q*theta) + d) * cos(p*theta),
                    a * cos(q*theta));

    // circle offset
    circle += scramble; 
    circle = Vec3f(circle[0], circle[1], circle[2]);
    Position = circle;
    }

};

// build ground plane
struct GroundPlane {
  Material material;
  Light light;
  Mesh groundPlane;
  Color color;

  void onDraw(Graphics& g, const Viewpoint& v, Mesh &groundPlane) {
    color = HSV(.3,.1,.13);
    material();
    // light();
    g.polygonMode(Graphics::LINE);
    g.lineWidth(1);
    g.pushMatrix();
    g.color(color);
    g.translate(0,-4,0);
    g.rotate(90,1,0,0);
    g.draw(groundPlane);
    g.popMatrix();
  }
};

// some more shared variables
struct State {
  double eyeSeparation, nearClip, farClip;
  double t;    // simulation time
  unsigned n;  // "frame" number
  float focalLength;
  Pose pose;   // for navigation
  Color backgroundColor;
  
  // bool timeToggle;
  bool drawLeaves;
  bool drawBranches;
  bool drawGround;  

  Vec3f leafPos[MAX_LEAVES];    //240000
  Color leafColor[MAX_LEAVES];  //240000
  int leafSkip[MAX_LEAVES];     //80000
  int refreshLeaves;
  int refreshTree;

  Vec3f treePos[V];             //600000
  Color treeColor[V];           //600000
  int pSize;                    
  int cSize;

  int currentLeafSize;

  double audioGain;             //8
  float f[N];                   //8800



  void print() {
    // for (int i=0; i<10000; i++) {
    //   cout << "p " << i << ": " << p[i] << endl;    
    // }
  }


};


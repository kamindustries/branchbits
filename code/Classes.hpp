#pragma once

///////////////////////////////////////////////////////////////////////
// B R A N C H   C L A S S
///////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////
// L E A F   C L A S S
///////////////////////////////////////////////////////////////////////
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
//WITHIN GROW()
// an alternative method of writing vertex information...
    //      
    m_test.reset();
    for (branchIt iterator = branches.begin(); iterator != branches.end(); iterator++) {
      if (iterator->second.Parent != NULL){
        Vec3f crossP;
        
        Vec3f vertAhead;
        Vec3f vertBehind;
        Vec3f vertOrig;
        Vec3f vertNew;

        Vec3f vert1;
        Vec3f vert2;
        Vec3f vert3;
        Vec3f vert4;

        Vec3f move1;
        Vec3f move2;
        Vec3f move3;
        Vec3f move4;

        crossP = cross(iterator->second.Parent->Position, iterator->second.Position) / 2;

        vert1 = iterator->second.Parent->Position + crossP * branchLength * branchWidth;
        vert2 = iterator->second.Parent->Position - crossP * branchLength * branchWidth;
        vert3 = iterator->second.Position - crossP * branchLength * branchWidth;
        vert4 = iterator->second.Position + crossP * branchLength * branchWidth;

        move1 = vert1 - vert2;
        move2 = vert2 - vert1;
        move3 = vert3 - vert4;
        move4 = vert4 - vert3;

        vert1 = vert1 + move1 * branchLength * iterator->second.Width;
        vert2 = vert2 + move2 * branchLength * iterator->second.Width;
        vert3 = vert3 + move3 * branchLength * iterator->second.Width;
        vert4 = vert4 + move4 * branchLength * iterator->second.Width;

        
        m_test.vertex(vert1);
        m_test.vertex(vert2);
        m_test.vertex(vert3);
        m_test.vertex(vert4);

        m_test.color(RGB(1,1,1));
        m_test.color(RGB(1,1,1));
        m_test.color(RGB(1,1,1));
        m_test.color(RGB(1,1,1));

      }
    }


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


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
#pragma once

#include <Tempest/PaintDevice>
#include <Tempest/Transform>
#include <Tempest/Brush>
#include <Tempest/Pen>

namespace Tempest {

class PaintEvent;

class Painter {
  public:
    enum Mode {
      Clear,
      Preserve
      };
    Painter(PaintEvent& ev, Mode m=Preserve);
    ~Painter();

    void setScissot(int x,int y,int w,int h);
    void setScissot(int x,int y,unsigned w,unsigned h);

    void setBrush(const Brush& b);
    void setPen  (const Pen&   p);

    void drawRect(int x,int y,int width,int height,
                  float u1,float v1,float u2,float v2);
    void drawRect(int x,int y,int width,int height);
    void drawRect(int x,int y,unsigned width,unsigned height);

    void drawLine(int x1,int y1,int x2,int y2);

  private:
    PaintDevice&       dev;
    PaintDevice::Point pt;
    Tempest::Transform tr=Transform();

    float invW    =1.f;
    float invH    =1.f;
    float penWidth=1.f;

    struct ScissorRect {
      int x=0,y=0,x1=0,y1=0;
      } scissor;

    struct FPoint{
      float x,y;
      float u,v;
      };

    void implAddPoint   (int x, int y, float u, float v);
    void implAddPointRaw(float x, float y, float u, float v);
    void implSetColor   (float r,float g,float b,float a);

    void drawTriangle( int x0, int y0, int u0, int v0,
                       int x1, int y1, int u1, int v1,
                       int x2, int y2, int u2, int v2 );
    void drawTrigImpl( float x0, float y0, float u0, float v0,
                       float x1, float y1, float u1, float v1,
                       float x2, float y2, float u2, float v2,
                       FPoint *out, int stage);
    void implDrawRect(int x1, int y1, int x2, int y2,
                      float u1, float v1, float u2, float v2);
  };

}
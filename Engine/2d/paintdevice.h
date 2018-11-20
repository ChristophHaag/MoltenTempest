#pragma once

#include <Tempest/AbstractGraphicsApi>
#include <Tempest/RenderPipeline>
#include <Tempest/VertexBuffer>
#include <initializer_list>

namespace Tempest {

class Texture2d;
class Color;

class PaintDevice {
  public:
    virtual ~PaintDevice()=default;

    struct Point {
      float x=0,y=0,z=0;
      float u=0,v=0;
      float r=0,g=0,b=0,a=0;
      };

  protected:
    using TexPtr =Detail::ResourcePtr<Tempest::Texture2d>;
    using PipePtr=Detail::ResourcePtr<Tempest::RenderPipeline>;

    virtual void clear()=0;
    virtual void addPoint(const Point& p)=0;
    virtual void commitPoints()=0;

    virtual void beginPaint(bool clear,uint32_t w,uint32_t h)=0;
    virtual void endPaint()=0;

    virtual void setBrush(const TexPtr& t,const Color& c)=0;
    virtual void setTopology(Topology t)=0;

  friend class Painter;
  };

template<>
inline std::initializer_list<Decl::ComponentType> vertexBufferDecl<PaintDevice::Point>() {
  return {Decl::float3,Decl::float2,Decl::float4};
  }
}
#include "vectorimage.h"

#include <Tempest/Device>
#include <Tempest/Builtin>
#include <Tempest/Painter>
#include <Tempest/Event>

#define  NANOSVG_IMPLEMENTATION
#include "thirdparty/nanosvg.h"

using namespace Tempest;

void VectorImage::beginPaint(bool clr, uint32_t w, uint32_t h) {
  if(clr || blocks.size()==0)
    clear();
  info.w=w;
  info.h=h;
  }

void VectorImage::endPaint() {
  for(size_t i=0;i<frameCount;++i)
    frame[i].outdated=true;
  outdatedCount=frameCount;
  }

template<class T,T VectorImage::Block::*param>
void VectorImage::setState(const T &t) {
  // blocks.size()>0, see VectorImage::clear()
  if(blocks.back().*param==t)
    return;

  if(blocks.back().size==0){
    blocks.back().*param=t;
    return;
    }

  blocks.emplace_back();
  blocks.back().begin =buf.size();
  blocks.back().*param=t;
  }

void VectorImage::setBrush(const TexPtr &t, const Color &c) {
  setState<TexPtr,&Block::brush>(t);
  }

void VectorImage::setTopology(Topology t) {
  setState<Topology,&Block::tp>(t);
  }

void VectorImage::clear() {
  buf.clear();
  blocks.resize(1);
  blocks.back()=Block();
  }

void VectorImage::addPoint(const PaintDevice::Point &p) {
  buf.push_back(p);
  blocks.back().size++;
  }

void VectorImage::commitPoints() {
  blocks.resize(blocks.size());

  while(blocks.size()>1){
    if(blocks.back().size!=0)
      return;
    blocks.pop_back();
    }
  }

void VectorImage::makeActual(Device &dev,RenderPass& pass) {
  if(!frame || frameCount!=dev.maxFramesInFlight()) {
    uint8_t count=dev.maxFramesInFlight();
    frame.reset(new PerFrame[count]);
    frameCount=count;
    }

  PerFrame& f=frame[dev.frameId()];
  if(f.outdated) {
    f.vbo=dev.loadVbo(buf,BufferFlags::Static);
    f.blocks.reserve(blocks.size());
    f.blocks.clear();

    for(auto& i:blocks){
      Uniforms ux;
      if(i.brush) {
        ux=dev.uniforms(dev.builtin().texture2d(pass,info.w,info.h).layout);
        ux.set(0,i.brush);
        } else {
        ux=dev.uniforms(dev.builtin().empty(pass,info.w,info.h).layout);
        }
      f.blocks.push_back(std::move(ux));
      }

    f.outdated=false;
    outdatedCount--;
    if(outdatedCount==0)
      clear();
    }
  }

void VectorImage::draw(Device & dev, CommandBuffer &cmd, RenderPass& pass) {
  makeActual(dev,pass);

  PerFrame& f=frame[dev.frameId()];

  for(size_t i=0;i<blocks.size();++i){
    auto& b=blocks[i];
    auto& u=f.blocks[i];

    if(!b.pipeline) {
      const RenderPipeline* p;
      if(b.brush) {
        if(b.tp==Triangles)
          p=&dev.builtin().texture2d(pass,info.w,info.h).brush; else
          p=&dev.builtin().texture2d(pass,info.w,info.h).pen;
        } else {
        if(b.tp==Triangles)
          p=&dev.builtin().empty(pass,info.w,info.h).brush; else
          p=&dev.builtin().empty(pass,info.w,info.h).pen;
        }
      b.pipeline=PipePtr(*p);
      }

    if(b.brush)
      cmd.setUniforms(b.pipeline,u); else
      cmd.setUniforms(b.pipeline);
    cmd.draw(f.vbo,b.begin,b.size);
    }
  }

bool VectorImage::load(const char *file) {
  NSVGimage* image = nsvgParseFromFile(file,"px",96);
  if(image==nullptr)
    return false;

  try {
    VectorImage img;
    PaintEvent  event(img,int(image->width),int(image->height));
    Painter     p(event);

    for(NSVGshape* shape=image->shapes; shape!=nullptr; shape=shape->next) {
      for(NSVGpath* path=shape->paths; path!=nullptr; path=path->next) {
        for(int i=0; i<path->npts-1; i+=3) {
          const float* p = &path->pts[i*2];
          //drawCubicBez(p[0],p[1], p[2],p[3], p[4],p[5], p[6],p[7]);
          }
        }
      }

    *this=std::move(img);
    }
  catch(...){
    nsvgDelete(image);
    return false;
    }
  nsvgDelete(image);
  return true;
  }

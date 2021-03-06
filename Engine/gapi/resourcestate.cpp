#include "resourcestate.h"

using namespace Tempest;
using namespace Tempest::Detail;

void ResourceState::setLayout(AbstractGraphicsApi::Attach& a, TextureLayout lay, bool preserve) {
  State& img   = findImg(&a,preserve);
  img.next     = lay;
  img.outdated = true;
  }

void ResourceState::setLayout(AbstractGraphicsApi::Buffer& b, BufferLayout lay) {
  BufState& buf = findBuf(&b);
  buf.next      = lay;
  buf.outdated  = true;
  }

void ResourceState::flushLayout(AbstractGraphicsApi::CommandBuffer& cmd) {
  for(auto& i:imgState) {
    if(!i.outdated)
      continue;
    cmd.changeLayout(*i.img,i.last,i.next,(i.next==i.last));
    i.last     = i.next;
    i.outdated = false;
    }
  for(auto& i:bufState) {
    if(!i.outdated)
      continue;
    cmd.changeLayout(*i.buf,i.last,i.next);
    i.last     = i.next;
    i.outdated = false;
    }
  }

void ResourceState::finalize(AbstractGraphicsApi::CommandBuffer& cmd) {
  if(imgState.size()==0)
    return; //early-out
  flushLayout(cmd);
  imgState.reserve(imgState.size());
  imgState.clear();
  }

ResourceState::State& ResourceState::findImg(AbstractGraphicsApi::Attach* img, bool preserve) {
  auto nativeImg = img->nativeHandle();
  for(auto& i:imgState) {
    if(i.img->nativeHandle()==nativeImg)
      return i;
    }
  State s={};
  s.img      = img;
  s.last     = preserve ? img->defaultLayout() : TextureLayout::Undefined;
  s.next     = TextureLayout::Undefined;
  s.outdated = false;
  imgState.push_back(s);
  return imgState.back();
  }

ResourceState::BufState& ResourceState::findBuf(AbstractGraphicsApi::Buffer* buf) {
  for(auto& i:bufState)
    if(i.buf==buf)
      return i;
  BufState s={};
  s.buf  = buf;
  s.last = BufferLayout::ComputeRead;
  s.next = BufferLayout::ComputeRead;
  s.outdated = false;
  bufState.push_back(s);
  return bufState.back();
  }

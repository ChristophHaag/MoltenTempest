#pragma once

#include <Tempest/IDevice>

namespace Tempest {

class MemReader : public IDevice {
  public:
    MemReader( const char* vec, size_t sz );
    MemReader( const uint8_t* vec, size_t sz );

    size_t  read(void* to,size_t sz) override;
    size_t  size() const override;
    uint8_t peek() override;
    size_t  seek(size_t advance) override;

  private:
    const uint8_t* vec;
    size_t         sz, pos=0;
  };

}

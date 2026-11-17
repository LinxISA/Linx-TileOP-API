#include "common/pto_memory.hpp"
#include <cassert>
#include <cstring>
using namespace pto;
int main() {
  int data[4] = {11, 22, 33, 44};
  auto v = scalar_view<MemoryScope::IntraCore>(data);
  assert(v.load(1) == 22);
  v.subview(2).store(0, -7);
  assert(data[2] == -7);
  float f = 1.25f;
  assert(scalar_load<MemoryScope::IntraCore>(&f) == 1.25f);
  scalar_store<MemoryScope::InterCore>(&f, -2.5f);
  assert(f == -2.5f);
  unsigned char src[17], dst[19];
  std::memset(src, 0x5a, sizeof(src));
  std::memset(dst, 0xa5, sizeof(dst));
  scoped_memcpy<MemoryScope::IntraCore, MemoryScope::Private>(dst+1, src, 17);
  assert(dst[0] == 0xa5 && dst[18] == 0xa5);
  assert(std::memcmp(dst+1, src, 17) == 0);
  scoped_memset<MemoryScope::IntraCore>(dst+2, 0xff, 13);
  for (int i=2; i<15; ++i) assert(dst[i] == 0xff);
  assert(dst[1] == 0x5a && dst[15] == 0x5a);
  scoped_memset<MemoryScope::Private>(nullptr, 0, 0);
  scoped_memcpy<MemoryScope::IntraCore, MemoryScope::Private>(nullptr, nullptr, 0);
}

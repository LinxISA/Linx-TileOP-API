#include <common/pto_tileop.hpp>
using namespace pto;

template<class T, unsigned Part>
void copy_element_snapshot(const T *input, T *output) {
  ElementTile<T,512> parent;
  TLOAD(parent,input,512);
  auto parts=TPARTVIEW<Part>(parent,512);
  TileArray<ElementTile<T,Part>,1,512/Part> staged;
  for(unsigned part=0; part<parts.size(); ++part) {
    auto source=parts.part(part);
    ElementTile<T,Part> values;
    TMOV(values,source);
    TMOV(staged[0][part],values);
  }
  auto snapshot=TASSEMBLY<ElementTile<T,512>>(std::move(staged));
  TSTORE(output,snapshot,512);
}
template void copy_element_snapshot<__half,64>(const __half *,__half *);
template void copy_element_snapshot<uint16_t,64>(const uint16_t *,uint16_t *);
template void copy_element_snapshot<int32_t,32>(const int32_t *,int32_t *);
template void copy_element_snapshot<uint32_t,32>(const uint32_t *,uint32_t *);

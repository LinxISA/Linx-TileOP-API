#include <common/pto_tileop.hpp>

using namespace pto;

template <class T, unsigned N>
void store_dense_part(T *output, ElementTile<T, 64> &values,
                      ElementTile<T, N> &parent, unsigned part,
                      unsigned count) {
  auto parts = TPARTVIEW<64>(parent, N);
  TSTORE(output, values, parts, part, count);
}

template void store_dense_part<__half, 64>(
    __half *, ElementTile<__half, 64> &, ElementTile<__half, 64> &, unsigned,
    unsigned);
template void store_dense_part<__half, 128>(
    __half *, ElementTile<__half, 64> &, ElementTile<__half, 128> &, unsigned,
    unsigned);
template void store_dense_part<uint16_t, 2048>(
    uint16_t *, ElementTile<uint16_t, 64> &,
    ElementTile<uint16_t, 2048> &, unsigned, unsigned);

template <class T, unsigned N>
void store_dense_full_part(T *output, ElementTile<T, 64> &values,
                           ElementTile<T, N> &parent, unsigned part) {
  auto parts = TPARTVIEW<64>(parent, N);
  TSTORE(output, values, parts, part);
}

template void store_dense_full_part<__half, 128>(
    __half *, ElementTile<__half, 64> &, ElementTile<__half, 128> &, unsigned);
template void store_dense_full_part<uint16_t, 512>(
    uint16_t *, ElementTile<uint16_t, 64> &, ElementTile<uint16_t, 512> &,
    unsigned);

// A zero-count store with a null base has no memory operation or address math.
void zero_count_store(ElementTile<uint16_t,64> &values) {
  auto parts = TPARTVIEW<64>(values, 0);
  TSTORE(static_cast<uint16_t *>(nullptr), values, parts, 0, 0);
}

template<class T>
void copy_dense_part(ElementTile<T,128> &parent, ElementTile<T,64> &values) {
  auto parts = TPARTVIEW<64>(parent,128);
  auto part = parts.part(1);
  TMOV(values,part);
}
template void copy_dense_part<__half>(ElementTile<__half,128> &, ElementTile<__half,64> &);
template void copy_dense_part<uint16_t>(ElementTile<uint16_t,128> &, ElementTile<uint16_t,64> &);
template void copy_dense_part<uint32_t>(ElementTile<uint32_t,128> &, ElementTile<uint32_t,64> &);
template void copy_dense_part<int32_t>(ElementTile<int32_t,128> &, ElementTile<int32_t,64> &);

template<class T>
void copy_dense_tile(ElementTile<T,64> &dst, const ElementTile<T,64> &src) {
  TMOV(dst,src);
}
template void copy_dense_tile<__half>(ElementTile<__half,64> &, const ElementTile<__half,64> &);
template void copy_dense_tile<uint16_t>(ElementTile<uint16_t,64> &, const ElementTile<uint16_t,64> &);
template void copy_dense_tile<uint32_t>(ElementTile<uint32_t,64> &, const ElementTile<uint32_t,64> &);
template void copy_dense_tile<int32_t>(ElementTile<int32_t,64> &, const ElementTile<int32_t,64> &);

#ifdef PTO_DENSE_REJECT_N8_TMOV
void reject_n8(CubeTileN8<float,4,8> &a, const CubeTileN8<float,4,8> &b) {
  TMOV(a,b);
}
#endif
#ifdef PTO_DENSE_REJECT_BOXED_TMOV
using Boxed = Tile<Location::Vec,float,16,16,BLayout::RowMajor,
                   16,16,SLayout::RowMajor>;
void reject_boxed(Boxed &a, const Boxed &b) { TMOV(a,b); }
#endif
#ifdef PTO_DENSE_REJECT_COL_MAJOR_TMOV
using Column = Tile<Location::Right,float,16,16,BLayout::ColMajor>;
void reject_column(Column &a, const Column &b) { TMOV(a,b); }
#endif

#ifdef PTO_DENSE_ENABLE_ELEMENT_VIEWS
#ifndef PTO_DENSE_DISABLE_F16_VIEW
void f16_element_view(ElementTile<__half, 64> &tile) {
  auto &elements = TPARTELEMENT(tile);
  elements[0] = elements[1];
}
#endif

#ifndef PTO_DENSE_DISABLE_U16_VIEW
void u16_element_view(ElementTile<uint16_t, 64> &tile) {
  auto &elements = TPARTELEMENT(tile);
  elements[0] = elements[1];
}
#endif
#endif

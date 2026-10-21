#include <common/pto_tileop.hpp>

using namespace pto;

using Leaf = ElementTile<uint32_t, 32>;
using Parent = ElementTile<uint32_t, 128>;

#ifndef PTO_TILEOP_API_HAS_TILE_ARRAY_PARTICIPANT_MASK
#error "TileArray participant-mask capability gate is required"
#endif

static_assert(TileArray<Leaf, 1, 4>::participant_mask == 15);
static_assert(TileArray<Leaf, 1, 4, 8>::participant_mask == 8);

PTO_PE_DOMAIN(8)
extern "C" void assemble_pe0(const uint32_t *input, uint32_t *output) {
  TileArray<Leaf, 1, 4, 8> leaves;
  for (unsigned part = 0; part < 4; ++part) {
    Leaf leaf;
    global_tensor<uint32_t, RowMajor<32, 4>> source(input + part);
    TLOAD(leaf, source);
    TMOV(leaves[0][part], leaf);
  }
  Parent assembled = TASSEMBLY<Parent>(std::move(leaves));
  TSTORE(output, assembled, 128);
  auto parts = TPARTVIEW<32>(assembled, 128);
  Leaf selected;
  auto first = parts.part(0);
  TMOV(selected, first);
  auto &elements = TPARTELEMENT(selected);
#pragma pto element for
  for (unsigned element = 0; element < 32; ++element)
    elements[element] ^= 1u;
  global_tensor<uint32_t, RowMajor<32, 1>> selected_output(output + 128);
  TSTORE(selected_output, selected);
}

extern "C" void assemble_default(const uint32_t *input, uint32_t *output) {
  TileArray<Leaf, 1, 4> leaves;
  for (unsigned part = 0; part < 4; ++part) {
    Leaf leaf;
    global_tensor<uint32_t, RowMajor<32, 4>> source(input + part);
    TLOAD(leaf, source);
    TMOV(leaves[0][part], leaf);
  }
  Parent assembled = TASSEMBLY<Parent>(std::move(leaves));
  TSTORE(output, assembled, 128);
}

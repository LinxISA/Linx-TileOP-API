#include <common/pto_tileop.hpp>

using namespace pto;

using LocalTile = TileRight<float, 16, 16>;
using Shared = SharedTile<LocalTile>;
using GM = global_tensor<float, RowMajor<16, 16>>;
using Out = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
using WeightOut = Tile<Location::Right, float, 16, 128, BLayout::RowMajor,
                       -1, -1>;
using WeightShared = SharedTile<WeightOut>;
using WeightGM = global_tensor<float, RowMajor<16, 16>>;

__attribute__((always_inline)) inline void load_shared(Shared &shared,
                                                        const GM &src) {
  TLOAD(shared, src);
}

__attribute__((always_inline)) inline void load_shared_ass(Shared &shared,
                                                            const GM &src) {
  TLOAD_ASS(shared, src);
}

void shared_tload(Out &out, const GM &src) {
  Shared shared;
  load_shared(shared, src);
  TMOV_S2L_BROADCAST(out, shared);
}

// TLOAD_ASS consumes an already-associated Shared handle: the producer
// TLOAD first establishes the Shared_ABS relation through its "=Sr"
// destination, then TLOAD_ASS reuses that handle as a source-only "Sr"
// operand. Feeding a default-constructed SharedTile directly to TLOAD_ASS
// is invalid: its handle is uninitialized (GR-class value) and the
// associated form has no association to consume.
void shared_tload_ass(Out &out, const GM &src) {
  Shared shared;
  load_shared(shared, src); // producer: establishes the Shared handle
  load_shared_ass(shared, src);
  TMOV_S2L_BROADCAST(out, shared);
}

void shared_insert(Out &out, const LocalTile &src) {
  Shared shared;
  TMOV_L2S_INSERT(shared, src);
  TMOV_S2L_BROADCAST(out, shared);
}

void shared_publish(Out &out, const LocalTile &src) {
  Shared shared;
  TMOV_L2S_PUBLISH(shared, src);
  TMOV_S2L_BROADCAST(out, shared);
}

void weight_tload(WeightShared &dst, const WeightGM &src) {
  // ShapeWord: Cin=16, Cout=16, KernelH=1, KernelW=1;
  // StartWord: NStart=0, KStart=0.  The destination exposes a 128-column
  // physical NK row while ValidK/ValidN come from its dynamic Shared view.
  TLOAD<OHWI2NK>(dst, src, make_weight_tload_params(16, 16, 1, 1));
}

int main() { return 0; }

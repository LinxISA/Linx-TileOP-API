#include <common/pto_tileop.hpp>

using namespace pto;

using LocalTile = TileRight<float, 16, 16>;
using Shared = SharedTile<LocalTile>;
using GM = global_tensor<float, RowMajor<16, 16>>;
using Out = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;

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

int main() { return 0; }

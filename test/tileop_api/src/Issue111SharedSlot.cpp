// Issue #111: Shared producers and consumers must name the architectural
// Shared slot directly. The carrier must not contain a C++ handle that can be
// copied, stored, or spilled through the ordinary integer ABI.
#include <common/pto_tileop.hpp>

using namespace pto;

using SharedA = SharedMatrixLeft<float, 64, 32>;
using SharedB = SharedMatrixRight<float, 16, 32>;
using SlotA = SharedTileSlot<0, SharedA>;
using SlotB = SharedTileSlot<1, SharedB>;
using Acc = CubeAccumulatorM16<float, 16, 16>;
using GM = global_tensor<float, RowMajor<64, 32>>;

__attribute__((noinline)) void issue111_shared_slots(Acc &dst, GM &src,
                                                       TIMG2COLParams image,
                                                       WeightTLOADParams weight) {
  TIMG2COL_SPART_SLOT<DN2ND, SlotA::Slot, 1, SharedA>(src, image);
  TLOAD_SLOT<OIHW2NK, SlotB::Slot, 1, SharedB>(src, weight);
  TMATMUL_SLOT<Acc, SlotA::Slot, SlotA, SlotB::Slot, SlotB>(dst);
}

int main() { return 0; }

#include <common/pto_tileop.hpp>
using namespace pto;
using W = VecTileM32<float, 32, 2>;
using P = VecTileM32<float, 32, 4>;
using In = global_tensor<float, RowMajor<32, 2>>;

void negatives(const float *input) {
#if defined(SHOULD_FAIL_301_CAPACITY)
  LocalCubeAssembly<W, W, W> bad;
#elif defined(SHOULD_FAIL_301_PARENT_LIMIT)
  LocalCubeAssembly<VecTileM32<float,32,1024>, W> bad;
#elif defined(SHOULD_FAIL_301_DTYPE)
  LocalCubeAssembly<P, VecTileM32<int32_t, 32, 2>> bad;
#elif defined(SHOULD_FAIL_301_LAYOUT)
  LocalCubeAssembly<P, VecTileM16<float, 16, 4>> bad;
#elif defined(SHOULD_FAIL_301_ROWS)
  LocalCubeAssembly<P, VecTileM32<float, 16, 2>> bad;
#elif defined(SHOULD_FAIL_301_VALID_ROWS)
  LocalCubeAssembly<P, W, VecTileM32<float, 32, 2, 16, 2>> bad;
#elif defined(SHOULD_FAIL_301_ENVELOPE)
  LocalCubeAssembly<P, VecTileM32<float, 32, 3>> bad;
#elif defined(SHOULD_FAIL_301_OVERSIZED_ENVELOPE)
  LocalCubeAssembly<P, VecTileM32<float, 32, 4, 32, 2>> bad;
#elif defined(SHOULD_FAIL_301_TAIL)
  LocalCubeAssembly<P, VecTileM32<float, 32, 2, 32, 1>, W> bad;
#elif defined(SHOULD_FAIL_301_DYNAMIC)
  LocalCubeAssembly<P, VecTileM32<float, 32, 2, 32, -1>> bad;
#elif defined(SHOULD_FAIL_301_WRITER_LIMIT)
  LocalCubeAssembly<P, W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W> bad;
#elif defined(SHOULD_FAIL_301_INDEX)
  LocalCubeAssembly<P, W, W> assembly;
  auto bad = assembly.writer<2>();
#elif defined(SHOULD_FAIL_301_INIT_ASS)
  LocalCubeAssembly<P, W, W> assembly;
  auto first = assembly.writer<0>();
  In src(input);
  TLOAD_ASS(first, src);
#elif defined(SHOULD_FAIL_301_CONT_LOAD)
  LocalCubeAssembly<P, W, W> assembly;
  auto last = assembly.writer<1>();
  In src(input);
  TLOAD(last, src);
#elif defined(SHOULD_FAIL_301_GM_DTYPE)
  LocalCubeAssembly<P, W, W> assembly;
  auto first = assembly.writer<0>();
  global_tensor<int32_t, RowMajor<32, 2>> src(nullptr);
  TLOAD(first, src);
#elif defined(SHOULD_FAIL_301_GM_SHAPE)
  LocalCubeAssembly<P, W, W> assembly;
  auto first = assembly.writer<0>();
  global_tensor<float, RowMajor<32, 1>> src(input);
  TLOAD(first, src);
#elif defined(SHOULD_FAIL_301_STORE_SHAPE)
  LocalCubeAssembly<P, W, W> assembly;
  auto parent = assembly.finish();
  global_tensor<float, RowMajor<32, 2>> dst(nullptr);
  TSTORE(dst, parent);
#elif defined(SHOULD_FAIL_301_LEGACY)
  P parent;
  auto first = range::assemble<2, 0>(parent);
  In src(input);
  TLOAD(first, src);
#elif defined(SHOULD_FAIL_301_OPEN_SOURCE)
  LocalCubeAssembly<P, W, W> assembly;
  global_tensor<float, RowMajor<32, 4>> dst(nullptr);
  TSTORE(dst, assembly);
#elif defined(SHOULD_FAIL_301_SLOT_SOURCE)
  LocalCubeAssembly<P, W, W> assembly;
  auto first = assembly.writer<0>();
  global_tensor<float, RowMajor<32, 4>> dst(nullptr);
  TSTORE(dst, first);
#else
  LocalCubeAssembly<P, W, W> assembly;
  static_assert(decltype(assembly)::offset<1>() == 2);
#endif
}

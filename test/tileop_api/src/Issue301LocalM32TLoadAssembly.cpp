#include <common/pto_tileop.hpp>
using namespace pto;
using W = VecTileM32<float, 32, 2>;
using In = global_tensor<float, RowMajor<32, 2>>;

template <int N, std::size_t... I>
__attribute__((always_inline)) inline void load_all(
    LocalCubeAssembly<VecTileM32<float, 32, 2 * N>,
                      std::conditional_t<(I < N), W, W>...> &assembly,
    const float *input, std::index_sequence<I...>) {
  auto load = [&]<std::size_t J>() {
    auto slot = assembly.template writer<J>();
    In src(input + J * 64);
    if constexpr (J == 0) TLOAD(slot, src);
    else TLOAD_ASS(slot, src);
  };
  (load.template operator()<I>(), ...);
}

template <int N, std::size_t... I>
__attribute__((always_inline)) inline void copy_impl(
    const float *input, float *output, std::index_sequence<I...> indices) {
  using P = VecTileM32<float, 32, 2 * N>;
  LocalCubeAssembly<P, std::conditional_t<(I < N), W, W>...> assembly;
  load_all<N>(assembly, input, indices);
  auto parent = assembly.finish();
  static_assert(decltype(parent)::PhysicalCols == 2 * N);
  static_assert(decltype(parent)::ValidCol == 2 * N);
  static_assert(decltype(parent)::PhysicalBytes == 256 * N);
  global_tensor<float, RowMajor<32, 2 * N>> dst(output);
  TSTORE(dst, parent);
}

extern "C" {
__attribute__((noinline)) void issue301_two(const float *in, float *out) {
  copy_impl<2>(in, out, std::make_index_sequence<2>{});
}
__attribute__((noinline)) void issue301_three(const float *in, float *out) {
  copy_impl<3>(in, out, std::make_index_sequence<3>{});
}
__attribute__((noinline)) void issue301_seven(const float *in, float *out) {
  copy_impl<7>(in, out, std::make_index_sequence<7>{});
}
__attribute__((noinline)) void issue301_mixed(const float *in, float *out) {
  using P = VecTileM32<float, 32, 8>;
  using Wide = VecTileM32<float, 32, 4>;
  using Narrow = VecTileM32<float, 32, 1>;
  LocalCubeAssembly<P, W, Wide, Narrow> assembly;
  auto first = assembly.writer<0>();
  In a(in);
  TLOAD(first, a);
  auto middle = assembly.writer<1>();
  global_tensor<float, RowMajor<32, 4>> b(in + 64);
  TLOAD_ASS(middle, b);
  auto last = assembly.writer<2>();
  global_tensor<float, RowMajor<32, 1>> c(in + 192);
  TLOAD_ASS(last, c);
  auto parent = assembly.finish();
  static_assert(decltype(parent)::PhysicalCols == 7);
  static_assert(decltype(parent)::ValidCol == 7);
  global_tensor<float, RowMajor<32, 7>> dst(out);
  TSTORE(dst, parent);
}
__attribute__((noinline)) void issue301_single(const float *in, float *out) {
  LocalCubeAssembly<VecTileM32<float, 32, 8>, W> assembly;
  auto first = assembly.writer<0>();
  In src(in);
  TLOAD(first, src);
  auto parent = assembly.finish();
  global_tensor<float, RowMajor<32, 2>> dst(out);
  TSTORE(dst, parent);
}
__attribute__((noinline)) void issue301_f16_tail(const __half *in, __half *out) {
  using Full = VecTileM32<__half, 32, 4>;
  using Tail = VecTileM32<__half, 32, 4, 32, 3>;
  LocalCubeAssembly<VecTileM32<__half, 32, 16>, Full, Tail> assembly;
  auto first = assembly.writer<0>();
  global_tensor<__half, RowMajor<32, 4>> a(in);
  TLOAD(first, a);
  auto last = assembly.writer<1>();
  global_tensor<__half, RowMajor<32, 3>> b(in + 128);
  TLOAD_ASS(last, b);
  auto parent = assembly.finish();
  static_assert(decltype(parent)::PhysicalCols == 8);
  static_assert(decltype(parent)::ValidCol == 7);
  global_tensor<__half, RowMajor<32, 7>> dst(out);
  TSTORE(dst, parent);
}
__attribute__((noinline)) void issue301_i64(const uint64_t *in, uint64_t *out) {
  using Fragment = VecTileM32<uint64_t, 32, 1>;
  LocalCubeAssembly<VecTileM32<uint64_t, 32, 2>, Fragment, Fragment> assembly;
  global_tensor<uint64_t, RowMajor<32, 1>> a(in), b(in + 32);
  auto first = assembly.writer<0>();
  TLOAD(first, a);
  auto last = assembly.writer<1>();
  TLOAD_ASS(last, b);
  auto parent = assembly.finish();
  global_tensor<uint64_t, RowMajor<32, 2>> dst(out);
  TSTORE(dst, parent);
}
// A real Local allocation between writers must not redirect continuation to
// the nearest Tile. Store it only after LAST so it remains live across LAST.
__attribute__((noinline)) void issue301_interleaved(
    const float *in, float *out, float *other) {
  LocalCubeAssembly<VecTileM32<float, 32, 4>, W, W> assembly;
  In a(in), b(in + 64), extra(in + 128), extra_out(other);
  auto first = assembly.writer<0>();
  TLOAD(first, a);
  W temporary;
  TLOAD(temporary, extra);
  auto last = assembly.writer<1>();
  TLOAD_ASS(last, b);
  auto parent = assembly.finish();
  global_tensor<float, RowMajor<32, 4>> dst(out);
  TSTORE(dst, parent);
  TSTORE(extra_out, temporary);
}
}

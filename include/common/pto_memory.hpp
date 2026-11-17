#ifndef PTO_MEMORY_HPP
#define PTO_MEMORY_HPP

#include <stddef.h>
#include <stdint.h>
#include <type_traits>

#if defined(__clang__) && __has_attribute(linx_scalar_shareability)
#define PTO_SCALAR_SHAREABILITY(Scope) __attribute__((linx_scalar_shareability(Scope)))
#else
// Expanding this on an unsupported compiler must fail at the declaration.
#define PTO_SCALAR_SHAREABILITY(Scope) static_assert(false, "compiler lacks PTO scalar shareability attribute");
#endif

namespace pto {

// These select participation of each Scalar GM request. They do not select
// cache policy or replace fences, completion waits, or SYNC_CORE.
enum class MemoryScope : unsigned { Private = 0, IntraCore = 1, InterCore = 2 };
enum class MemoryOrder : unsigned {
  Relaxed = 0, Release = 1, Acquire = 2, AcqRel = 3, SeqCst = 4
};
enum class AtomicOperation : unsigned {
  Add = 0, Exchange = 1, CompareExchange = 2, LoadReserved = 3,
  StoreConditional = 4, And = 5, Or = 6, Xor = 7,
  SignedMin = 8, SignedMax = 9, UnsignedMin = 10, UnsignedMax = 11
};

namespace memory_detail {
template<class> inline constexpr bool dependent_false = false;
template<class T> using bits_type = std::conditional_t<sizeof(T) == 1, uint8_t,
    std::conditional_t<sizeof(T) == 2, uint16_t,
    std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>>>;
template<class T> inline T from_bits(uint64_t bits) {
  bits_type<T> narrow = static_cast<bits_type<T>>(bits);
  T result;
  __builtin_memcpy(&result, &narrow, sizeof(T));
  return result;
}
template<class T> inline uint64_t to_bits(T value) {
  bits_type<T> bits;
  __builtin_memcpy(&bits, &value, sizeof(T));
  return bits;
}
template<MemoryScope S> constexpr void check_scope() {
  static_assert(static_cast<unsigned>(S) <= 2, "invalid PTO memory scope");
}
template<class T> constexpr void check_type() {
  static_assert(std::is_trivially_copyable_v<T> &&
                !std::is_volatile_v<T> &&
                (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8),
                "PTO scalar access requires a nonvolatile trivially copyable 1/2/4/8-byte type");
}
} // namespace memory_detail

// Explicit builtins retain request scope through optimization. Never fall
// back to an ordinary target dereference when compiler support is missing.
template<MemoryScope S, class T>
__attribute__((always_inline)) inline T scalar_load(const T* p) {
  memory_detail::check_scope<S>();
  memory_detail::check_type<T>();
#if defined(__linx)
#if __has_builtin(__builtin_linx_scalar_load)
  uint64_t bits = __builtin_linx_scalar_load(p, sizeof(T), static_cast<unsigned>(S));
  return memory_detail::from_bits<T>(bits);
#else
  static_assert(memory_detail::dependent_false<T>, "compiler lacks PTO scalar shareability support");
#endif
#else
  return *p;
#endif
}

template<MemoryScope S, class T>
__attribute__((always_inline)) inline void scalar_store(T* p, T value) {
  memory_detail::check_scope<S>();
  memory_detail::check_type<T>();
#if defined(__linx)
#if __has_builtin(__builtin_linx_scalar_store)
  uint64_t bits = memory_detail::to_bits(value);
  __builtin_linx_scalar_store(p, bits, sizeof(T), static_cast<unsigned>(S));
#else
  static_assert(memory_detail::dependent_false<T>, "compiler lacks PTO scalar shareability support");
#endif
#else
  *p = value;
#endif
}

// A view associates scope with accesses made through this interface, not with
// an address allocation. raw_data() is solely for Tile GM binders/interop;
// ordinary dereferences of that raw pointer retain the compiler's defaults.
template<class T, MemoryScope S> class ScalarMemoryView {
  T* pointer_;
public:
  explicit constexpr ScalarMemoryView(T* p) : pointer_(p) { memory_detail::check_scope<S>(); }
  T load(size_t i = 0) const { return scalar_load<S>(pointer_ + i); }
  void store(size_t i, T value) const { scalar_store<S>(pointer_ + i, value); }
  T* raw_data() const { return pointer_; }
  ScalarMemoryView subview(size_t offset) const { return ScalarMemoryView(pointer_ + offset); }
};
template<MemoryScope S, class T> inline ScalarMemoryView<T, S> scalar_view(T* p) {
  return ScalarMemoryView<T, S>(p);
}

// Byte-wise helpers handle arbitrary alignment and never turn into ordinary
// target memset/memcpy calls. No synchronization is implied. memcpy requires
// nonoverlapping ranges, as does the standard C operation.
template<MemoryScope S>
inline void scoped_memset(void* dst, unsigned char value, size_t bytes) {
  auto* d = static_cast<unsigned char*>(dst);
  for (size_t i = 0; i < bytes; ++i) scalar_store<S>(d + i, value);
}
template<MemoryScope D, MemoryScope S>
inline void scoped_memcpy(void* dst, const void* src, size_t bytes) {
  auto* d = static_cast<unsigned char*>(dst);
  auto* s = static_cast<const unsigned char*>(src);
  for (size_t i = 0; i < bytes; ++i) scalar_store<D>(d + i, scalar_load<S>(s + i));
}

template<MemoryScope S, AtomicOperation Op,
         MemoryOrder O = MemoryOrder::Relaxed, class T>
__attribute__((always_inline)) inline T scalar_atomic(T* p, T value = {}, T desired = {}) {
  memory_detail::check_scope<S>();
  static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool> &&
                (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8),
                "PTO atomics require an integer of 1/2/4/8 bytes");
  static_assert(static_cast<unsigned>(O) <= 4 && static_cast<unsigned>(Op) <= 11,
                "invalid PTO atomic order or operation");
  static_assert((Op != AtomicOperation::Add && static_cast<unsigned>(Op) < 5) || sizeof(T) >= 4,
                "arithmetic atomics require 4 or 8 bytes");
  static_assert(Op != AtomicOperation::LoadReserved ||
                (O != MemoryOrder::Release && O != MemoryOrder::AcqRel), "LR cannot release");
  static_assert(Op != AtomicOperation::StoreConditional ||
                (O != MemoryOrder::Acquire && O != MemoryOrder::AcqRel), "SC cannot acquire");
#if defined(__linx) && __has_builtin(__builtin_linx_scalar_atomic)
  uint64_t result = __builtin_linx_scalar_atomic(
      p, static_cast<uint64_t>(value), static_cast<uint64_t>(desired), sizeof(T),
      static_cast<unsigned>(S), static_cast<unsigned>(O), static_cast<unsigned>(Op));
  return memory_detail::from_bits<T>(result);
#else
  // Native LR/SC monitors cannot be emulated by host C++ atomics. Keep all
  // atomic support explicit rather than implying CPU-simulation equivalence.
  static_assert(memory_detail::dependent_false<T>, "compiler lacks PTO scoped atomic support");
#endif
}

} // namespace pto
#endif

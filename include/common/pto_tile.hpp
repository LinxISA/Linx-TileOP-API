#ifndef PTO_TILE_HPP
#define PTO_TILE_HPP

#include "common/layout.hpp"
#include <common/type.hpp>
#include <cstdint>

namespace pto {

/// comparison predicates
/// 
/// Example usage:
/// @code
/// CmpMode pred = CmpMode::eq;  // equality comparison
/// if (pred == CmpMode::slt) {
///     // signed less than comparison
/// }
/// @endcode
enum class CmpMode {
  EQ,  ///< Equal (==)
  NE,  ///< Not equal (!=)
  GT,  ///< Greater than (>)
  LT,  ///< Less than (<)
  GE,  ///< Greater than or equal (>=)
  LE,  ///< Less than or equal (<=)
};

/// Padding Value : keep SAME with asm encoding
enum class PadValue {
  Zero = 0,
  Max = 1,
  Min = 2,
  Null = 3,
};

enum class FixpPreQuantMode : uint8_t {
  None = 0,
  F322F16 = 1,
  VREQS8Pre = 2,
  REQS8Pre = 3,
  VDEQF16 = 4,
  DEQF16 = 5,
  VSHIFTS322S16 = 12,
  SHIFTS322S16 = 13,
  F322BF16 = 16,
  QF322S4Pre = 17,
  VQF322S4Pre = 18,
  QF322S16Pre = 19,
  VQF322S16Pre = 20,
  VQF322S8Pre = 23,
  QF322S8Pre = 24,
  QF322HIF8Pre = 25,
  QF322FP8Pre = 26,
  QF322F32Pre = 27,
  VQF322HIF8Pre = 28,
  QF322F16Pre = 32,
  VQF322F16Pre = 33,
  QF322BF16Pre = 34,
  QS322BF16Pre = 35,
  VQF322BF16Pre = 36,
  VQF322FP8Pre = 37,
  VQF322F32Pre = 38,
  VQS322BF16Pre = 39,
};

enum class FixpReluMode : uint8_t {
  None = 0,
  Relu = 1,
  LRelu = 2,
  PRelu = 3,
};

struct FixpAttr {
  FixpPreQuantMode PreQuant = FixpPreQuantMode::None;
  FixpReluMode Relu = FixpReluMode::None;
  uint8_t GroupNCode = 0;
  bool RowMaxEn = false;
  bool GroupMaxEn = false;
  bool RowMaxInit = false;
  bool MaxAbsEn = false;

  static constexpr FixpAttr keep_acc(
      FixpReluMode ReluMode = FixpReluMode::None) {
    FixpAttr Attr;
    Attr.Relu = ReluMode;
    return Attr;
  }

  static constexpr FixpAttr f16(
      FixpReluMode ReluMode = FixpReluMode::None) {
    FixpAttr Attr;
    Attr.PreQuant = FixpPreQuantMode::F322F16;
    Attr.Relu = ReluMode;
    return Attr;
  }

  static constexpr FixpAttr bf16(
      FixpReluMode ReluMode = FixpReluMode::None) {
    FixpAttr Attr;
    Attr.PreQuant = FixpPreQuantMode::F322BF16;
    Attr.Relu = ReluMode;
    return Attr;
  }

  constexpr uint32_t encoding() const {
    return (static_cast<uint32_t>(PreQuant) << 26) |
           (static_cast<uint32_t>(Relu) << 23) |
           (static_cast<uint32_t>(GroupNCode) << 19) |
           (static_cast<uint32_t>(RowMaxEn) << 18) |
           (static_cast<uint32_t>(GroupMaxEn) << 17) |
           (static_cast<uint32_t>(RowMaxInit) << 16) |
           (static_cast<uint32_t>(MaxAbsEn) << 15) | 0x2023;
  }

  constexpr bool operator==(const FixpAttr &) const = default;
};

constexpr bool is_valid_fixp_pre_quant(FixpPreQuantMode Mode) {
  switch (Mode) {
  case FixpPreQuantMode::None:
  case FixpPreQuantMode::F322F16:
  case FixpPreQuantMode::VREQS8Pre:
  case FixpPreQuantMode::REQS8Pre:
  case FixpPreQuantMode::VDEQF16:
  case FixpPreQuantMode::DEQF16:
  case FixpPreQuantMode::VSHIFTS322S16:
  case FixpPreQuantMode::SHIFTS322S16:
  case FixpPreQuantMode::F322BF16:
  case FixpPreQuantMode::QF322S4Pre:
  case FixpPreQuantMode::VQF322S4Pre:
  case FixpPreQuantMode::QF322S16Pre:
  case FixpPreQuantMode::VQF322S16Pre:
  case FixpPreQuantMode::VQF322S8Pre:
  case FixpPreQuantMode::QF322S8Pre:
  case FixpPreQuantMode::QF322HIF8Pre:
  case FixpPreQuantMode::QF322FP8Pre:
  case FixpPreQuantMode::QF322F32Pre:
  case FixpPreQuantMode::VQF322HIF8Pre:
  case FixpPreQuantMode::QF322F16Pre:
  case FixpPreQuantMode::VQF322F16Pre:
  case FixpPreQuantMode::QF322BF16Pre:
  case FixpPreQuantMode::QS322BF16Pre:
  case FixpPreQuantMode::VQF322BF16Pre:
  case FixpPreQuantMode::VQF322FP8Pre:
  case FixpPreQuantMode::VQF322F32Pre:
  case FixpPreQuantMode::VQS322BF16Pre:
    return true;
  }
  return false;
}

constexpr bool is_valid_fixp_attr(FixpAttr Attr) {
  if (!is_valid_fixp_pre_quant(Attr.PreQuant) ||
      static_cast<uint8_t>(Attr.Relu) >
          static_cast<uint8_t>(FixpReluMode::PRelu) ||
      Attr.GroupNCode > 9)
    return false;
  if (!Attr.RowMaxEn && Attr.RowMaxInit)
    return false;
  if (Attr.GroupMaxEn != (Attr.GroupNCode != 0))
    return false;
  if (!Attr.RowMaxEn && !Attr.GroupMaxEn && Attr.MaxAbsEn)
    return false;
  return true;
}

constexpr bool is_basic_fixp_attr(FixpAttr Attr) {
  const bool ParameterFreePreQuant =
      Attr.PreQuant == FixpPreQuantMode::None ||
      Attr.PreQuant == FixpPreQuantMode::F322F16 ||
      Attr.PreQuant == FixpPreQuantMode::F322BF16;
  const bool ParameterFreeRelu = Attr.Relu == FixpReluMode::None ||
                                 Attr.Relu == FixpReluMode::Relu;
  return ParameterFreePreQuant && ParameterFreeRelu && !Attr.RowMaxEn &&
         !Attr.GroupMaxEn && !Attr.RowMaxInit && !Attr.MaxAbsEn;
}

constexpr bool is_parameter_free_fixp_pre_quant(FixpPreQuantMode Mode) {
  return Mode == FixpPreQuantMode::None || Mode == FixpPreQuantMode::F322F16 ||
         Mode == FixpPreQuantMode::F322BF16;
}

constexpr bool is_scalar_fixp_pre_quant(FixpPreQuantMode Mode) {
  switch (Mode) {
  case FixpPreQuantMode::REQS8Pre:
  case FixpPreQuantMode::DEQF16:
  case FixpPreQuantMode::SHIFTS322S16:
  case FixpPreQuantMode::QF322S4Pre:
  case FixpPreQuantMode::QF322S16Pre:
  case FixpPreQuantMode::QF322S8Pre:
  case FixpPreQuantMode::QF322HIF8Pre:
  case FixpPreQuantMode::QF322FP8Pre:
  case FixpPreQuantMode::QF322F32Pre:
  case FixpPreQuantMode::QF322F16Pre:
  case FixpPreQuantMode::QF322BF16Pre:
  case FixpPreQuantMode::QS322BF16Pre:
    return true;
  default:
    return false;
  }
}

constexpr bool is_vector_fixp_pre_quant(FixpPreQuantMode Mode) {
  switch (Mode) {
  case FixpPreQuantMode::VREQS8Pre:
  case FixpPreQuantMode::VDEQF16:
  case FixpPreQuantMode::VSHIFTS322S16:
  case FixpPreQuantMode::VQF322S4Pre:
  case FixpPreQuantMode::VQF322S16Pre:
  case FixpPreQuantMode::VQF322S8Pre:
  case FixpPreQuantMode::VQF322HIF8Pre:
  case FixpPreQuantMode::VQF322F16Pre:
  case FixpPreQuantMode::VQF322BF16Pre:
  case FixpPreQuantMode::VQF322FP8Pre:
  case FixpPreQuantMode::VQF322F32Pre:
  case FixpPreQuantMode::VQS322BF16Pre:
    return true;
  default:
    return false;
  }
}

template <FixpAttr Attr, typename DType>
constexpr bool is_fixp_output_type() {
  constexpr int TypeCode = type_traits<DType>::TypeCode;
  if constexpr (Attr.PreQuant == FixpPreQuantMode::None)
    // PreQuant=None keeps the AccType result; only FP32 is accepted (no S32
    // alias) per PTO 0.58 (handoff 3327).
    return TypeCode == __type_fp32;
  if constexpr (Attr.PreQuant == FixpPreQuantMode::F322F16 ||
                Attr.PreQuant == FixpPreQuantMode::VDEQF16 ||
                Attr.PreQuant == FixpPreQuantMode::DEQF16 ||
                Attr.PreQuant == FixpPreQuantMode::QF322F16Pre ||
                Attr.PreQuant == FixpPreQuantMode::VQF322F16Pre)
    return TypeCode == __type_fp16;
  if constexpr (Attr.PreQuant == FixpPreQuantMode::F322BF16 ||
                Attr.PreQuant == FixpPreQuantMode::QF322BF16Pre ||
                Attr.PreQuant == FixpPreQuantMode::QS322BF16Pre ||
                Attr.PreQuant == FixpPreQuantMode::VQF322BF16Pre ||
                Attr.PreQuant == FixpPreQuantMode::VQS322BF16Pre)
    return TypeCode == __type_bf16;
  if constexpr (Attr.PreQuant == FixpPreQuantMode::VREQS8Pre ||
                Attr.PreQuant == FixpPreQuantMode::REQS8Pre ||
                Attr.PreQuant == FixpPreQuantMode::VQF322S8Pre ||
                Attr.PreQuant == FixpPreQuantMode::QF322S8Pre)
    return TypeCode == __type_int8;
  if constexpr (Attr.PreQuant == FixpPreQuantMode::VSHIFTS322S16 ||
                Attr.PreQuant == FixpPreQuantMode::SHIFTS322S16 ||
                Attr.PreQuant == FixpPreQuantMode::QF322S16Pre ||
                Attr.PreQuant == FixpPreQuantMode::VQF322S16Pre)
    return TypeCode == __type_int16;
  if constexpr (Attr.PreQuant == FixpPreQuantMode::QF322S4Pre ||
                Attr.PreQuant == FixpPreQuantMode::VQF322S4Pre)
    return TypeCode == __type_int4x2;
  if constexpr (Attr.PreQuant == FixpPreQuantMode::QF322HIF8Pre ||
                Attr.PreQuant == FixpPreQuantMode::VQF322HIF8Pre)
    return TypeCode == __type_hif8;
  if constexpr (Attr.PreQuant == FixpPreQuantMode::QF322FP8Pre ||
                Attr.PreQuant == FixpPreQuantMode::VQF322FP8Pre)
    // Generic FP8 output is normalized to E4M3 only (no E5M2 alias) per
    // PTO 0.58 (handoff 3327).
    return TypeCode == __type_fp8_e4m3;
  if constexpr (Attr.PreQuant == FixpPreQuantMode::QF322F32Pre ||
                Attr.PreQuant == FixpPreQuantMode::VQF322F32Pre)
    return TypeCode == __type_fp32;
  return false;
}

template <FixpAttr Attr, typename DType>
constexpr bool is_basic_fixp_output_type() {
  return is_fixp_output_type<Attr, DType>();
}

/// Layout for GlobalTensor
/// ND: lower tow dimensions are arranged in RowMajor order;
/// DN: lower tow dimensions are arranged in ColMajor order;
/// NZ: lower tow dimensions are arranged in RowMajor order;
///     third and fourth dimensions are arranged in ColMajor order.
/// SCALE: 
/// MAX: 
enum class Layout {
  ND,
  DN,
  NZ,
  SCALE,
  MAX,
};

constexpr int DYNAMIC = -1;

template <int N1 = DYNAMIC, int N2 = DYNAMIC, int N3 = DYNAMIC, int N4 = DYNAMIC, int N5 = DYNAMIC>
struct Shape {
  static constexpr int staticShape[5] = {N1, N2, N3, N4, N5};

  inline Shape() {
    static_assert(
      (N1 == DYNAMIC) + (N2 == DYNAMIC) + (N3 == DYNAMIC) + (N4 == DYNAMIC) + (N5 == DYNAMIC) == 0,
      "0-parameter constructors is only applicable to Stride with 0 dynamic dimension."
    );
  }

  inline Shape(int n) {
    static_assert(
      (N1 == DYNAMIC) + (N2 == DYNAMIC) + (N3 == DYNAMIC) + (N4 == DYNAMIC) + (N5 == DYNAMIC) == 1,
      "1-parameter constructors is only applicable to Stride with 1 dynamic dimension."
    );
    if constexpr (N1 == DYNAMIC) shape[0] = n;
    if constexpr (N2 == DYNAMIC) shape[1] = n;
    if constexpr (N3 == DYNAMIC) shape[2] = n;
    if constexpr (N4 == DYNAMIC) shape[3] = n;
    if constexpr (N5 == DYNAMIC) shape[4] = n;
  }

  inline Shape(int n1, int n2) {
    static_assert(
      (N1 == DYNAMIC) + (N2 == DYNAMIC) + (N3 == DYNAMIC) + (N4 == DYNAMIC) + (N5 == DYNAMIC) == 2,
      "2-parameter constructors is only applicable to Stride with 2 dynamic dimension."
    );

    int idx = 0;
    const int vals[] = {n1, n2};
    if constexpr (N1 == DYNAMIC) shape[0] = vals[idx++];
    if constexpr (N2 == DYNAMIC) shape[1] = vals[idx++];
    if constexpr (N3 == DYNAMIC) shape[2] = vals[idx++];
    if constexpr (N4 == DYNAMIC) shape[3] = vals[idx++];
    if constexpr (N5 == DYNAMIC) shape[4] = vals[idx++];
  }

  inline Shape(int n1, int n2, int n3) {
    static_assert(
      (N1 == DYNAMIC) + (N2 == DYNAMIC) + (N3 == DYNAMIC) + (N4 == DYNAMIC) + (N5 == DYNAMIC) == 3,
      "3-parameter constructors is only applicable to Stride with 3 dynamic dimension."
    );
    int idx = 0;
    const int vals[] = {n1, n2, n3};
    if constexpr (N1 == DYNAMIC) shape[0] = vals[idx++];
    if constexpr (N2 == DYNAMIC) shape[1] = vals[idx++];
    if constexpr (N3 == DYNAMIC) shape[2] = vals[idx++];
    if constexpr (N4 == DYNAMIC) shape[3] = vals[idx++];
    if constexpr (N5 == DYNAMIC) shape[4] = vals[idx++];
  }

  inline Shape(int n1, int n2, int n3, int n4) {
    static_assert(
      (N1 == DYNAMIC) + (N2 == DYNAMIC) + (N3 == DYNAMIC) + (N4 == DYNAMIC) + (N5 == DYNAMIC) == 4,
      "4-parameter constructors is only applicable to Stride with 4 dynamic dimension."
    );
    int idx = 0;
    const int vals[] = {n1, n2, n3, n4};
    if constexpr (N1 == DYNAMIC) shape[0] = vals[idx++];
    if constexpr (N2 == DYNAMIC) shape[1] = vals[idx++];
    if constexpr (N3 == DYNAMIC) shape[2] = vals[idx++];
    if constexpr (N4 == DYNAMIC) shape[3] = vals[idx++];
    if constexpr (N5 == DYNAMIC) shape[4] = vals[idx++];
  }

  inline Shape(int n1, int n2, int n3, int n4, int n5)
  {
    static_assert(
      (N1 == DYNAMIC) + (N2 == DYNAMIC) + (N3 == DYNAMIC) + (N4 == DYNAMIC) + (N5 == DYNAMIC) == 5,
      "5-parameter constructors is only applicable to Stride with 5 dynamic dimension."
    );
    if constexpr (N1 == DYNAMIC) shape[0] = n1;
    if constexpr (N2 == DYNAMIC) shape[1] = n2;
    if constexpr (N3 == DYNAMIC) shape[2] = n3;
    if constexpr (N4 == DYNAMIC) shape[3] = n4;
    if constexpr (N5 == DYNAMIC) shape[4] = n5;
  }

public:
  int shape[5] = {1};
};

template <int SN1 = DYNAMIC, int SN2 = DYNAMIC, int SN3 = DYNAMIC, int SN4 = DYNAMIC, int SN5 = DYNAMIC>
struct Stride {
  static constexpr int staticStride[5] = {SN1, SN2, SN3, SN4, SN5};

  inline Stride() {  }

  inline Stride(int n) {
    static_assert(
      (SN1 == DYNAMIC) + (SN2 == DYNAMIC) + (SN3 == DYNAMIC) + (SN4 == DYNAMIC) + (SN5 == DYNAMIC) == 1,
      "1-parameter constructors is only applicable to Stride with 1 dynamic dimension."
    );

    if constexpr (SN1 == DYNAMIC) stride[0] = n;
    if constexpr (SN2 == DYNAMIC) stride[1] = n;
    if constexpr (SN3 == DYNAMIC) stride[2] = n;
    if constexpr (SN4 == DYNAMIC) stride[3] = n;
    if constexpr (SN5 == DYNAMIC) stride[4] = n;
  }

  inline Stride(int n1, int n2) {
    static_assert(
      (SN1 == DYNAMIC) + (SN2 == DYNAMIC) + (SN3 == DYNAMIC) + (SN4 == DYNAMIC) + (SN5 == DYNAMIC) == 2,
      "2-parameter constructors is only applicable to Stride with 2 dynamic dimension."
    );
    int idx = 0;
    const int vals[] = {n1, n2};
    if constexpr (SN1 == DYNAMIC) stride[0] = vals[idx++];
    if constexpr (SN2 == DYNAMIC) stride[1] = vals[idx++];
    if constexpr (SN3 == DYNAMIC) stride[2] = vals[idx++];
    if constexpr (SN4 == DYNAMIC) stride[3] = vals[idx++];
    if constexpr (SN5 == DYNAMIC) stride[4] = vals[idx++];
  }

  inline Stride(int n1, int n2, int n3) {
    static_assert(
      (SN1 == DYNAMIC) + (SN2 == DYNAMIC) + (SN3 == DYNAMIC) + (SN4 == DYNAMIC) + (SN5 == DYNAMIC) == 3,
      "3-parameter constructors is only applicable to Stride with 3 dynamic dimension."
    );
    int idx = 0;
    const int vals[] = {n1, n2, n3};
    if constexpr (SN1 == DYNAMIC) stride[0] = vals[idx++];
    if constexpr (SN2 == DYNAMIC) stride[1] = vals[idx++];
    if constexpr (SN3 == DYNAMIC) stride[2] = vals[idx++];
    if constexpr (SN4 == DYNAMIC) stride[3] = vals[idx++];
    if constexpr (SN5 == DYNAMIC) stride[4] = vals[idx++];
  }

  inline Stride(int n1, int n2, int n3, int n4) {
    static_assert(
      (SN1 == DYNAMIC) + (SN2 == DYNAMIC) + (SN3 == DYNAMIC) + (SN4 == DYNAMIC) + (SN5 == DYNAMIC) == 4,
      "4-parameter constructors is only applicable to Stride with 4 dynamic dimension."
    );
    int idx = 0;
    const int vals[] = {n1, n2, n3, n4};
    if constexpr (SN1 == DYNAMIC) stride[0] = vals[idx++];
    if constexpr (SN2 == DYNAMIC) stride[1] = vals[idx++];
    if constexpr (SN3 == DYNAMIC) stride[2] = vals[idx++];
    if constexpr (SN4 == DYNAMIC) stride[3] = vals[idx++];
    if constexpr (SN5 == DYNAMIC) stride[4] = vals[idx++];
  }

  inline Stride(int n1, int n2, int n3, int n4, int n5)
  {
    static_assert(
      (SN1 == DYNAMIC) + (SN2 == DYNAMIC) + (SN3 == DYNAMIC) + (SN4 == DYNAMIC) + (SN5 == DYNAMIC) == 5,
      "5-parameter constructors is only applicable to Stride with 5 dynamic dimension."
    );
    if constexpr (SN1 == DYNAMIC) stride[0] = n1;
    if constexpr (SN2 == DYNAMIC) stride[1] = n2;
    if constexpr (SN3 == DYNAMIC) stride[2] = n3;
    if constexpr (SN4 == DYNAMIC) stride[3] = n4;
    if constexpr (SN5 == DYNAMIC) stride[4] = n5;
  }

public:
  int stride[5] = {1};
};

template <typename Element_, typename Shape_, typename Stride_, Layout Layout_ = Layout::ND>
struct GlobalTensor {
  using Shape = Shape_;
  using Stride = Stride_;
  using DType = Element_;
  static constexpr Layout layout = Layout_;

  static const Shape defaultShape;
  static const Stride defaultStride;

  static constexpr int staticShape[5] = {Shape::staticShape[0], Shape::staticShape[1],
                                         Shape::staticShape[2], Shape::staticShape[3],
                                         Shape::staticShape[4]};
  static constexpr int staticStride[5] = {Stride::staticStride[0], Stride::staticStride[1],
                                          Stride::staticStride[2], Stride::staticStride[3],
                                          Stride::staticStride[4]};
  static constexpr bool isRowMajor = Layout_ == Layout::ND;
  static constexpr int RowStride = staticStride[3];
  static constexpr int ColStride = staticStride[4];

  inline GlobalTensor() = default;

  inline GlobalTensor(DType *data, const Shape &shape = defaultShape, const Stride &stride = defaultStride)
  {
    data_ = data;

    if constexpr (staticShape[0] == DYNAMIC) shape_.shape[0] = shape.shape[0];
    if constexpr (staticShape[1] == DYNAMIC) shape_.shape[1] = shape.shape[1];
    if constexpr (staticShape[2] == DYNAMIC) shape_.shape[2] = shape.shape[2];
    if constexpr (staticShape[3] == DYNAMIC) shape_.shape[3] = shape.shape[3];
    if constexpr (staticShape[4] == DYNAMIC) shape_.shape[4] = shape.shape[4];

    if constexpr (staticStride[0] == DYNAMIC) stride_.stride[0] = stride.stride[0];
    if constexpr (staticStride[1] == DYNAMIC) stride_.stride[1] = stride.stride[1];
    if constexpr (staticStride[2] == DYNAMIC) stride_.stride[2] = stride.stride[2];
    if constexpr (staticStride[3] == DYNAMIC) stride_.stride[3] = stride.stride[3];
    if constexpr (staticStride[4] == DYNAMIC) stride_.stride[4] = stride.stride[4];
  }

  inline int GetShape(const int dim) const
  {
    switch (dim) {
      case 0: return GetShapeSize<staticShape[0]>(dim);
      case 1: return GetShapeSize<staticShape[1]>(dim);
      case 2: return GetShapeSize<staticShape[2]>(dim);
      case 3: return GetShapeSize<staticShape[3]>(dim);
      case 4: return GetShapeSize<staticShape[4]>(dim);
      default: return -1;
    }
  }

  inline int GetStride(const int dim) const
  {
    switch (dim) {
      case 0: return GetStrideSize<staticStride[0]>(dim);
      case 1: return GetStrideSize<staticStride[1]>(dim);
      case 2: return GetStrideSize<staticStride[2]>(dim);
      case 3: return GetStrideSize<staticStride[3]>(dim);
      case 4: return GetStrideSize<staticStride[4]>(dim);
      default: return -1;
    }
  }

  DType *data() { return data_; }
  const DType *data() const { return data_; }

private:
  template <int StaticShape>
  inline int GetShapeSize(const int dim) const
  {
    if constexpr (StaticShape == DYNAMIC)
      return shape_.shape[dim];
    else
      return StaticShape;
  }

  template <int StaticStride>
  inline int GetStrideSize(const int dim) const
  {
    if constexpr (StaticStride == DYNAMIC)
      return stride_.stride[dim];
    else
      return StaticStride;
  }

  DType *data_;
  Shape shape_ = defaultShape;
  Stride stride_ = defaultStride;
};

template <typename Element_, typename Shape_, typename Stride_, Layout Layout_>
const typename GlobalTensor<Element_, Shape_, Stride_, Layout_>::Shape
GlobalTensor<Element_, Shape_, Stride_, Layout_>::defaultShape{ };

template <typename Element_, typename Shape_, typename Stride_, Layout Layout_>
const typename GlobalTensor<Element_, Shape_, Stride_, Layout_>::Stride
GlobalTensor<Element_, Shape_, Stride_, Layout_>::defaultStride{ };

template <Location Loc_, typename Element_, const int Rows_, const int Cols_,
          const BLayout BFractal_ = BLayout::RowMajor,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_,
          const SLayout SFractal_ = SLayout::NoneBox,
          const int SFractalSize_ = 512,
          const PadValue PadVal_ = PadValue::Null,
          const CompactMode Compact_ = CompactMode::Null>
struct Tile {
public:
  using DType = Element_;

  static constexpr int getInnerRow() {
    if constexpr (SFractalSize_ == 1024) { // output/acc
      static_assert(type_traits<DType>::bits == 32, "Size of datatype != 4");
      return 16;
    } else {
      return isBoxedLayout
                ? (isInnerRowMajor ? 16 : byteSize * 8 / type_traits<DType>::bits)
                : 1;
    }
  }

  static constexpr int getInnerCol() {
      if constexpr (SFractalSize_ == 1024) { // output/acc
        static_assert(type_traits<DType>::bits == 32, "Size of datatype != 4");
        return 16;
      } else {
        return isBoxedLayout
                  ? (isInnerRowMajor ? byteSize * 8 / type_traits<DType>::bits : 16)
                  : 1;
      }
  }

  static constexpr Location Loc = Loc_;
  static constexpr int Rows = Rows_;
  static constexpr int Cols = Cols_;
  static constexpr int RowStride = BFractal_ == BLayout::RowMajor ? Cols : 1;
  static constexpr int ColStride = BFractal_ == BLayout::RowMajor ? 1 : Rows;

  static constexpr int kBytes = (Rows_ * Cols_ * type_traits<DType>::bits + 7) / 8;
  // static_assert(kBytes % 512 == 0, "Tile size must be 512 bytes aligned");
  // static_assert(((kBytes / 512 - 1) & (kBytes / 512)) == 0, "Tile size must by (512 * 2 ^ n) Bytes");
  // static_assert(kBytes >= 512 && kBytes <= 64 * 1024, "Tile size must be in [512B, 32kB]");

  static constexpr int ValidRow = RowValid_;
  static constexpr int ValidCol = ColValid_;
  // Location::Shared is a storage-class marker for SharedTile<LocalTile>; a
  // bare Tile<Shared,...> would wrongly allocate a Local TileDType data_ payload
  // and expose data(), which a Shared operand must never own. Route Shared
  // storage through SharedTile<> instead.
  static_assert(Loc_ != Location::Shared,
                "Use SharedTile<LocalTile> for Location::Shared; Tile<> is for "
                "local register-backed storage only.");
  static_assert(Rows > 0 && ValidRow <= Rows && Cols > 0 && ValidCol <= Cols,
                "Invalid Tile Layout.");

  static constexpr BLayout BFractal = BFractal_;
  static constexpr SLayout SFractal = SFractal_;
  static constexpr int Numel = Rows * Cols;
  static constexpr bool isRowMajor = BFractal_ == BLayout::RowMajor;

  static constexpr int SFractalSize = SFractalSize_;
  static constexpr PadValue PadVal = PadVal_;
  static constexpr CompactMode Compact = Compact_;
  static constexpr int LogicalTileBytes =
      (Rows * Cols * type_traits<DType>::bits + 7) / 8;
  static constexpr int TilesizeCode =
      LogicalTileBytes == 128  ? __tilesize_128B :
      LogicalTileBytes == 256  ? __tilesize_256B :
      LogicalTileBytes == 512  ? __tilesize_512B :
      LogicalTileBytes == 1024 ? __tilesize_1KB :
      LogicalTileBytes == 2048 ? __tilesize_2KB :
      LogicalTileBytes == 4096 ? __tilesize_4KB :
      LogicalTileBytes == 8192 ? __tilesize_8KB : __tilesize_unknown;
  static constexpr bool IsValidActiveSize =
      TilesizeCode >= __tilesize_128B && TilesizeCode <= __tilesize_8KB;

  // constructor for static shape
  Tile() { };
  template <int RowMask = ValidRow, int ColMask = ValidCol>
  Tile(std::enable_if_t<(RowMask > 0) && (ColMask > 0), DType> val);

  // constructor for both dimensions are runtime variables
  template <int RowMask = ValidRow, int ColMask = ValidCol>
  Tile(std::enable_if_t<RowMask == -1 && ColMask == -1, size_t> VR,
       std::enable_if_t<RowMask == -1 && ColMask == -1, size_t> VC);
  template <int RowMask = ValidRow, int ColMask = ValidCol>
  Tile(DType val, std::enable_if_t<RowMask == -1 && ColMask == -1, size_t> VR,
                  std::enable_if_t<RowMask == -1 && ColMask == -1, size_t> VC);

  // constructor for row dimension is runtime variables
  template <int RowMask = ValidRow, int ColMask = ValidCol>
  Tile(std::enable_if_t<(RowMask == -1) && (ColMask > 0), size_t> VR);
  template <int RowMask = ValidRow, int ColMask = ValidCol>
  Tile(DType val, std::enable_if_t<(RowMask == -1) && (ColMask > 0), size_t> VR);

  // constructor for col dimension is runtime variables
  template <int RowMask = ValidRow, int ColMask = ValidCol>
  Tile(std::enable_if_t<(RowMask > 0) && (ColMask == -1), size_t> VC);
  template <int RowMask = ValidRow, int ColMask = ValidCol>
  Tile(DType val, std::enable_if_t<(RowMask > 0) && (ColMask == -1), size_t> VC);

  static constexpr int byteSize = 32;
  static constexpr bool isBoxedLayout = (SFractal != SLayout::NoneBox);
  static constexpr bool isInnerRowMajor = (SFractal == SLayout::RowMajor);
  static constexpr bool isInnerColMajor = (SFractal == SLayout::ColMajor);

  static constexpr int InnerRows = getInnerRow();
  static constexpr int InnerCols = getInnerCol();

  static constexpr int InnerNumel = InnerRows * InnerCols;

  static_assert(Rows % InnerRows == 0,
                "Layout rows must be divisible by inner box rows");
  static_assert(Cols % InnerCols == 0,
                "Layout cols must be divisible by inner box cols");

  static_assert(
      (BFractal_ == BLayout::RowMajor && SFractal_ == SLayout::NoneBox && Cols * type_traits<DType>::bits % (32 * 8) == 0) ||
      (BFractal_ == BLayout::ColMajor && SFractal_ == SLayout::NoneBox && Rows * type_traits<DType>::bits % (32 * 8) == 0) ||
      (SFractal_ != SLayout::NoneBox) && (Rows % InnerRows == 0 && Cols % InnerCols == 0),
      "BFractal_ is RowMajor and SFractal_ is NoneBox: Rows must be 32 bytes align, \
        BFractal_ is ColMajor and SFractal_ is NoneBox: Cols must be 32 bytes align, \
        SFractal_ in not NoneBox: Rows/Cols must be integer multiple of InnerRows/InnerCols."
        );

  static_assert(SFractalSize_ == 512 || SFractalSize_ == 1024,
                "SFractalSize_ illegal");

#ifdef __linx
  using TileDType = int32_t __attribute__((ext_vector_type(1024)));
#else
  using TileDType = DType[Rows * Cols];
#endif

  TileDType &data() { return data_; }
  const TileDType &data() const { return data_; }

  // record dynamic shape info
  int RowMaskInternal;
  int ColMaskInternal;

  template <int RowMask = ValidRow>
  static constexpr std::enable_if_t<(RowMask > 0), int> GetValidRow() {
    return RowMask;
  }

  template <int RowMask = ValidRow>
  std::enable_if_t<RowMask == -1, int> GetValidRow() const {
    return RowMaskInternal;
  }

  template <int ColMask = ValidCol>
  static constexpr std::enable_if_t<(ColMask > 0), int> GetValidCol() {
    return ColMask;
  }

  template <int ColMask = ValidCol>
  std::enable_if_t<ColMask == -1, int> GetValidCol() const {
    return ColMaskInternal;
  }

  void assignData(TileDType data) { data_ = data; }

  TileDType data_;
};

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileLeft =
  Tile<Location::Left, Element_, Rows_, Cols_, BLayout::ColMajor,
       RowValid_, ColValid_, SLayout::RowMajor, 512>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileRight =
  Tile<Location::Right, Element_, Rows_, Cols_, BLayout::RowMajor,
       RowValid_, ColValid_, SLayout::ColMajor, 512>;

template <int Rows, int Cols, bool RowMajor>
struct stride_selector;

template <>
struct stride_selector<DYNAMIC, DYNAMIC, true> {
    using type = Stride<1, 1, DYNAMIC, DYNAMIC, DYNAMIC>;
};

template <int Cols>
struct stride_selector<DYNAMIC, Cols, true> {
    using type = Stride<1, 1, -1, Cols, 1>;
};

template <int Cols>
struct stride_selector<DYNAMIC, Cols, false> {
    using type = Stride<1, 1, -1, 1, -1>;
};

template <int Rows>
struct stride_selector<Rows, DYNAMIC, true> {
    using type = Stride<1, 1, -1, -1, 1>;
};
template <int Rows>
struct stride_selector<Rows, DYNAMIC, false> {
    using type = Stride<1, 1, -1, 1, Rows>;
};

template <int Rows, int Cols, bool RowMajor>
struct stride_selector {
    using type = std::conditional_t<RowMajor,
                      Stride<1, 1, Rows * Cols, Cols, 1>,
                      Stride<1, 1, Rows * Cols, 1, Rows>>;
};

template <typename Element_, typename MLayout_>
struct global_tensor {
public:
  using DType = Element_;
  using MLayout = MLayout_;

  static constexpr int Numel = get_numel<MLayout>;
  static constexpr int Rows = num_rows<MLayout>;
  static constexpr int Cols = num_cols<MLayout>;
  static constexpr int RowStride = row_stride<MLayout>;
  static constexpr int ColStride = col_stride<MLayout>;
  static constexpr LayoutEnum kType = layout_type<MLayout>;

  static constexpr bool isRowMajor = kType == LayoutEnum::kRowMajor;

  using shape_t = Shape<1, 1, 1, 1, 1>;
  using stride_t = typename stride_selector<Rows, Cols, isRowMajor>::type;
  static constexpr Layout layout_t = isRowMajor ? Layout::ND : Layout::DN;

  using Impl = GlobalTensor<DType, shape_t, stride_t, layout_t>;

  static constexpr int staticShape[5] = {
      Impl::staticShape[0], Impl::staticShape[1], Impl::staticShape[2],
      Impl::staticShape[3], Impl::staticShape[4]
  };

  static constexpr int staticStride[5] = {
      Impl::staticStride[0], Impl::staticStride[1], Impl::staticStride[2],
      Impl::staticStride[3], Impl::staticStride[4]
  };

  template <typename T = void,
            typename = std::enable_if_t<(Rows != DYNAMIC && Cols != DYNAMIC), T>>
  global_tensor(DType* data)
            : impl_(data), layout_(MLayout{}) {}

  template <typename T = void,
            typename = std::enable_if_t<(Rows == DYNAMIC && Cols != DYNAMIC) || (Rows != DYNAMIC && Cols == DYNAMIC), T>>
  global_tensor(DType* data, int stride) {
    if constexpr ((Rows == DYNAMIC && Cols != DYNAMIC && isRowMajor) || (Rows != DYNAMIC && Cols == DYNAMIC && !isRowMajor)) {
      impl_ = Impl(data, shape_t{}, stride_t(stride)); layout_ = MLayout{};
    } else {
      impl_ = Impl(data, shape_t{}, stride_t(stride, stride)); layout_ = MLayout{};
    }
  }

  template <typename T = void,
            typename = std::enable_if_t<(Rows == DYNAMIC && Cols == DYNAMIC), T>>
  global_tensor(DType* data, int dynamicRow, int dynamicCol)
            : impl_(data, shape_t{}, stride_t(dynamicRow*dynamicCol, dynamicCol, dynamicRow)), layout_(MLayout{}) {}

  const DType *data() const { return impl_.data(); }
  DType *data() { return impl_.data(); }

  int GetShape(int dim) const { return impl_.GetShape(dim); }
  int GetStride(int dim) const { return impl_.GetStride(dim); }

private:
  Impl impl_;
  MLayout layout_;
};

template <typename T> struct is_global : std::false_type {};
template <typename T> struct is_tile : std::false_type {
  static constexpr SLayout layout_enum = SLayout::NoneBox;
};

template <typename Element_, typename Shape_, typename Stride_>
struct is_global<GlobalTensor<Element_, Shape_, Stride_>> : std::true_type {};

template <typename Element_, typename Shape_, typename Stride_, Layout Layout_>
struct is_global<GlobalTensor<Element_, Shape_, Stride_, Layout_>> : std::true_type {};

template <typename Element_, typename Layout_>
struct is_global<global_tensor<Element_, Layout_>> : std::true_type {};

template <Location Loc_, typename Element_, const int Rows_, const int Cols_,
          const BLayout BFractal_, const int RowValid_, const int ColValid_,
          const SLayout SFractal_, const int SFractalSize_, const PadValue PadVal_>
struct is_tile<Tile<Loc_, Element_, Rows_, Cols_, BFractal_, RowValid_,
                    ColValid_, SFractal_, SFractalSize_, PadVal_>> : std::true_type {
  static constexpr SLayout layout_enum = SFractal_;
};

template <typename T>
constexpr bool is_boxed_tile =
  is_tile<T>::value && (is_tile<T>::layout_enum != SLayout::NoneBox);

template <typename tile_shape> struct is_Nz_layout {
  static constexpr bool value = !tile_shape::isRowMajor &&
                                tile_shape::isBoxedLayout &&
                                tile_shape::isInnerRowMajor;
};

template <typename tile_shape> struct is_Zn_layout {
  static constexpr bool value = tile_shape::isRowMajor &&
                                tile_shape::isBoxedLayout &&
                                tile_shape::isInnerColMajor;
};

template <typename T> concept is_global_data_v = is_global<T>::value;

template <typename T> concept is_tile_data_v = is_tile<T>::value;

template <typename T> concept is_boxed_data_v = is_boxed_tile<T>;

// v5 Shared storage-class wrapper. SharedTile<LocalTile> is public C++ sugar
// that changes a matrix operand's storage class (Local -> Shared) so the
// compiler lowers it via a B.IOS binder instead of a B.IOT source stream.
// It preserves the wrapped Local Tile's role, shape, dtype and layout exactly
// (per the LinxISA v0.58 Shared semantics) and owns no Local TileDType payload.
//
// The compiler allocates the opaque handle to S0..S255. It must not be
// materialized as an ordinary integer or passed through a normal GPR ABI.
template <typename LocalTile>
class SharedTile {
  static_assert(is_tile<LocalTile>::value,
                "SharedTile<LocalTile>: LocalTile must be an ordinary Tile");
  static_assert(LocalTile::Loc != Location::Shared,
                "SharedTile cannot wrap a SharedTile (nesting not allowed)");
public:
  using LocalTileType = LocalTile;
  using DType = typename LocalTile::DType;

  // Storage-class marker; role, shape, dtype and layout are forwarded from the
  // wrapped Local Tile so a Shared Right is indistinguishable from its Local
  // counterpart except for storage/lowering.
  static constexpr Location Loc = Location::Shared;
  static constexpr Location Role = LocalTile::Loc;
  static constexpr int Rows = LocalTile::Rows;
  static constexpr int Cols = LocalTile::Cols;
  static constexpr int ValidRow = LocalTile::ValidRow;
  static constexpr int ValidCol = LocalTile::ValidCol;
  static constexpr BLayout BFractal = LocalTile::BFractal;
  static constexpr SLayout SFractal = LocalTile::SFractal;
  static constexpr int SFractalSize = LocalTile::SFractalSize;
  static constexpr PadValue PadVal = LocalTile::PadVal;

  SharedTile()
      : RowMaskInternal(ValidRow == DYNAMIC ? 0 : ValidRow),
        ColMaskInternal(ValidCol == DYNAMIC ? 0 : ValidCol) {}

  explicit SharedTile(const LocalTile &local)
      : RowMaskInternal(local.GetValidRow()),
        ColMaskInternal(local.GetValidCol()) {}

  int GetValidRow() const { return RowMaskInternal; }
  int GetValidCol() const { return ColMaskInternal; }

  void SetValidShape(const LocalTile &local) {
    RowMaskInternal = local.GetValidRow();
    ColMaskInternal = local.GetValidCol();
  }

  unsigned long &handle_ref() { return Handle; }
  unsigned long handle() const { return Handle; }

  // No data() accessor: a Shared operand must never be passed to an ordinary
  // tile-register inline-asm operand. Shared-aware operations use handle() with
  // the compiler's dedicated `S` register constraint.

private:
  unsigned long Handle;
  int RowMaskInternal;
  int ColMaskInternal;
};

// v5 SharedTile traits. is_tile_data_v stays scoped to ordinary Local Tiles
// so TADD/TCVT/TSTORE-Local etc. cannot accidentally accept a Shared operand;
// APIs that explicitly support Shared use is_shared_tile_v / tile_role_v.
template <typename T> struct is_shared_tile : std::false_type {};
template <typename LocalTile>
struct is_shared_tile<SharedTile<LocalTile>> : std::true_type {};

template <typename T>
concept is_shared_tile_v = is_shared_tile<T>::value;

// An ordinary (non-Shared) Tile that can own a Local register payload.
template <typename T>
concept is_local_tile_v =
    is_tile<T>::value && T::Loc != Location::Shared;

// TMATMUL matrix operands may live in either Local or Shared storage. Their
// matrix role remains Left/Right; only the instruction operand transport
// changes (Local B.IOT versus Shared B.IOS binder).
template <typename T>
concept is_local_or_shared_left =
    (is_tile<T>::value && T::Loc == Location::Left) ||
    (is_shared_tile<T>::value && T::Role == Location::Left);

template <typename T>
concept is_local_or_shared_right =
    (is_tile<T>::value && T::Loc == Location::Right) ||
    (is_shared_tile<T>::value && T::Role == Location::Right);

// Matrix role of a tile operand, transparently unwrapping SharedTile. A plain
// Tile's role IS its Loc; a SharedTile preserves the wrapped Local Tile's
// role in ::Role. Dispatch via a helper so a plain Tile is never required to
// define ::Role (the ternary would ill-form otherwise).
namespace detail {
template <typename T, bool IsShared> struct tile_role_impl { static constexpr Location value = T::Loc; };
template <typename T> struct tile_role_impl<T, true> { static constexpr Location value = T::Role; };
} // namespace detail
template <typename T>
inline constexpr Location tile_role_v =
    detail::tile_role_impl<T, is_shared_tile<T>::value>::value;

// detail: Shared handle access for the compiler builtin path. The handle is an
// opaque integer carrying the architectural Shared ID; it must never be forgeable
// by ordinary integers, and must never enter a Tile-vector OverloadType list.
namespace detail {
template <typename LocalTile>
constexpr unsigned long shared_handle(const SharedTile<LocalTile> &tile) {
  return tile.handle();
}
} // namespace detail

namespace fixp {

struct NoOperand {};

constexpr FixpAttr with_relu(FixpAttr Attr, FixpReluMode Mode) {
  Attr.Relu = Mode;
  return Attr;
}

constexpr FixpAttr with_row_max(FixpAttr Attr, bool Init) {
  Attr.RowMaxEn = true;
  Attr.RowMaxInit = Init;
  return Attr;
}

constexpr FixpAttr with_group_max(FixpAttr Attr, uint8_t GroupNCode) {
  Attr.GroupMaxEn = true;
  Attr.GroupNCode = GroupNCode;
  return Attr;
}

constexpr FixpAttr with_max_abs(FixpAttr Attr) {
  Attr.MaxAbsEn = true;
  return Attr;
}

constexpr int group_n_from_code(uint8_t Code) {
  constexpr int Values[] = {0, 8, 16, 32, 48, 64, 80, 96, 112, 128};
  return Code <= 9 ? Values[Code] : 0;
}

template <int GroupN> constexpr uint8_t group_n_code() {
  static_assert(GroupN == 8 || GroupN == 16 || GroupN == 32 || GroupN == 48 ||
                    GroupN == 64 || GroupN == 80 || GroupN == 96 ||
                    GroupN == 112 || GroupN == 128,
                "TMATMUL_FIXP GroupN must be 8, 16, 32, 48, 64, 80, 96, "
                "112 or 128");
  if constexpr (GroupN == 8)
    return 1;
  if constexpr (GroupN == 16)
    return 2;
  if constexpr (GroupN == 32)
    return 3;
  if constexpr (GroupN == 48)
    return 4;
  if constexpr (GroupN == 64)
    return 5;
  if constexpr (GroupN == 80)
    return 6;
  if constexpr (GroupN == 96)
    return 7;
  if constexpr (GroupN == 112)
    return 8;
  return 9;
}

template <FixpAttr Attr_, typename QuantTile_ = NoOperand,
          typename ReluTile_ = NoOperand, typename RowMaxIn_ = NoOperand,
          typename RowMaxOut_ = NoOperand,
          typename GroupMaxOut_ = NoOperand>
struct Options {
  static constexpr FixpAttr Attr = Attr_;
  using QuantTile = QuantTile_;
  using ReluTile = ReluTile_;
  using RowMaxIn = RowMaxIn_;
  using RowMaxOut = RowMaxOut_;
  using GroupMaxOut = GroupMaxOut_;

  uint64_t QuantDescriptor = 0;
  uint64_t LReluDescriptor = 0;
  QuantTile *Quant = nullptr;
  ReluTile *Relu = nullptr;
  RowMaxIn *RowIn = nullptr;
  RowMaxOut *RowOut = nullptr;
  GroupMaxOut *GroupOut = nullptr;

  constexpr Options() = default;

  constexpr Options(uint64_t QuantDescriptor, uint64_t LReluDescriptor,
                    QuantTile *Quant, ReluTile *Relu, RowMaxIn *RowIn,
                    RowMaxOut *RowOut, GroupMaxOut *GroupOut)
      : QuantDescriptor(QuantDescriptor), LReluDescriptor(LReluDescriptor),
        Quant(Quant), Relu(Relu), RowIn(RowIn), RowOut(RowOut),
        GroupOut(GroupOut) {}

  constexpr auto relu() const {
    static_assert(Attr.Relu == FixpReluMode::None,
                  "TMATMUL_FIXP ReLU mode was already selected");
    constexpr FixpAttr NewAttr = with_relu(Attr, FixpReluMode::Relu);
    return Options<NewAttr, QuantTile, ReluTile, RowMaxIn, RowMaxOut,
                   GroupMaxOut>(QuantDescriptor, LReluDescriptor, Quant, Relu,
                                RowIn, RowOut, GroupOut);
  }

  constexpr auto lrelu(uint64_t Descriptor) const {
    static_assert(Attr.Relu == FixpReluMode::None,
                  "TMATMUL_FIXP ReLU mode was already selected");
    constexpr FixpAttr NewAttr = with_relu(Attr, FixpReluMode::LRelu);
    return Options<NewAttr, QuantTile, ReluTile, RowMaxIn, RowMaxOut,
                   GroupMaxOut>(QuantDescriptor, Descriptor, Quant, Relu,
                                RowIn, RowOut, GroupOut);
  }

  template <is_local_tile_v Tile>
  constexpr auto prelu(Tile &Parameter) const {
    static_assert(Attr.Relu == FixpReluMode::None,
                  "TMATMUL_FIXP ReLU mode was already selected");
    static_assert(std::is_same_v<ReluTile, NoOperand>,
                  "TMATMUL_FIXP PReLU Tile was already supplied");
    constexpr FixpAttr NewAttr = with_relu(Attr, FixpReluMode::PRelu);
    return Options<NewAttr, QuantTile, Tile, RowMaxIn, RowMaxOut,
                   GroupMaxOut>(QuantDescriptor, LReluDescriptor, Quant,
                                &Parameter, RowIn, RowOut, GroupOut);
  }

  template <is_local_tile_v Tile>
  constexpr auto row_max(Tile &Output) const {
    static_assert(!Attr.RowMaxEn,
                  "TMATMUL_FIXP RowMax was already enabled");
    static_assert(std::is_same_v<RowMaxIn, NoOperand> &&
                      std::is_same_v<RowMaxOut, NoOperand>,
                  "TMATMUL_FIXP RowMax operands were already supplied");
    constexpr FixpAttr NewAttr = with_row_max(Attr, false);
    return Options<NewAttr, QuantTile, ReluTile, NoOperand, Tile,
                   GroupMaxOut>(QuantDescriptor, LReluDescriptor, Quant, Relu,
                                nullptr, &Output, GroupOut);
  }

  template <is_local_tile_v InputTile, is_local_tile_v OutputTile>
  constexpr auto row_max(InputTile &Input, OutputTile &Output) const {
    static_assert(!Attr.RowMaxEn,
                  "TMATMUL_FIXP RowMax was already enabled");
    static_assert(std::is_same_v<RowMaxIn, NoOperand> &&
                      std::is_same_v<RowMaxOut, NoOperand>,
                  "TMATMUL_FIXP RowMax operands were already supplied");
    constexpr FixpAttr NewAttr = with_row_max(Attr, true);
    return Options<NewAttr, QuantTile, ReluTile, InputTile, OutputTile,
                   GroupMaxOut>(QuantDescriptor, LReluDescriptor, Quant, Relu,
                                &Input, &Output, GroupOut);
  }

  template <int GroupN, is_local_tile_v Tile>
  constexpr auto group_max(Tile &Output) const {
    static_assert(!Attr.GroupMaxEn,
                  "TMATMUL_FIXP GroupMax was already enabled");
    static_assert(std::is_same_v<GroupMaxOut, NoOperand>,
                  "TMATMUL_FIXP GroupMax output was already supplied");
    constexpr FixpAttr NewAttr =
        with_group_max(Attr, group_n_code<GroupN>());
    return Options<NewAttr, QuantTile, ReluTile, RowMaxIn, RowMaxOut, Tile>(
        QuantDescriptor, LReluDescriptor, Quant, Relu, RowIn, RowOut, &Output);
  }

  constexpr auto max_abs() const {
    static_assert(Attr.RowMaxEn || Attr.GroupMaxEn,
                  "TMATMUL_FIXP max_abs requires RowMax or GroupMax");
    constexpr FixpAttr NewAttr = with_max_abs(Attr);
    return Options<NewAttr, QuantTile, ReluTile, RowMaxIn, RowMaxOut,
                   GroupMaxOut>(QuantDescriptor, LReluDescriptor, Quant, Relu,
                                RowIn, RowOut, GroupOut);
  }
};

template <FixpPreQuantMode Mode = FixpPreQuantMode::None>
constexpr auto convert() {
  static_assert(is_parameter_free_fixp_pre_quant(Mode),
                "fixp::convert accepts only parameter-free PreQuant modes");
  constexpr FixpAttr Attr = [] {
    FixpAttr Value;
    Value.PreQuant = Mode;
    return Value;
  }();
  return Options<Attr>{};
}

template <FixpPreQuantMode Mode>
constexpr auto scalar(uint64_t QuantDescriptor) {
  static_assert(is_scalar_fixp_pre_quant(Mode),
                "fixp::scalar requires a scalar-parameter PreQuant mode");
  constexpr FixpAttr Attr = [] {
    FixpAttr Value;
    Value.PreQuant = Mode;
    return Value;
  }();
  return Options<Attr>(QuantDescriptor, 0, nullptr, nullptr, nullptr, nullptr,
                       nullptr);
}

template <FixpPreQuantMode Mode, is_local_tile_v Tile>
constexpr auto vector(Tile &QuantParameter) {
  static_assert(is_vector_fixp_pre_quant(Mode),
                "fixp::vector requires a vector-parameter PreQuant mode");
  constexpr FixpAttr Attr = [] {
    FixpAttr Value;
    Value.PreQuant = Mode;
    return Value;
  }();
  return Options<Attr, Tile>(0, 0, &QuantParameter, nullptr, nullptr, nullptr,
                             nullptr);
}

constexpr auto keep_acc() { return convert<FixpPreQuantMode::None>(); }
constexpr auto f16() { return convert<FixpPreQuantMode::F322F16>(); }
constexpr auto bf16() { return convert<FixpPreQuantMode::F322BF16>(); }
constexpr auto s8(uint64_t QuantDescriptor) {
  return scalar<FixpPreQuantMode::QF322S8Pre>(QuantDescriptor);
}

template <is_local_tile_v Tile> constexpr auto s8(Tile &QuantParameter) {
  return vector<FixpPreQuantMode::VQF322S8Pre>(QuantParameter);
}

template <typename T> struct is_options : std::false_type {};
template <FixpAttr Attr, typename QuantTile, typename ReluTile,
          typename RowMaxIn, typename RowMaxOut, typename GroupMaxOut>
struct is_options<Options<Attr, QuantTile, ReluTile, RowMaxIn, RowMaxOut,
                          GroupMaxOut>> : std::true_type {};

template <typename T>
concept is_options_v = is_options<std::remove_cv_t<T>>::value;

} // namespace fixp


template <typename shape> int index(int i, int j) {
  if constexpr (is_global_data_v<shape>) {
    return i * shape::RowStride + j * shape::ColStride;
  } else if constexpr (is_tile_data_v<shape>) {
    if constexpr (is_boxed_data_v<shape>) {
      int sub_tile_i = i / shape::InnerRows;
      int sub_tile_j = j / shape::InnerCols;
      int idx_i = i % shape::InnerRows;
      int idx_j = j % shape::InnerCols;
      if constexpr (is_Nz_layout<shape>::value) {
        return sub_tile_j * shape::Rows * shape::InnerCols +
               sub_tile_i * shape::InnerNumel + idx_i * shape::InnerCols +
               idx_j;
      } else if constexpr (is_Zn_layout<shape>::value) {
        return sub_tile_i * shape::Cols * shape::InnerRows +
               sub_tile_j * shape::InnerNumel + idx_i +
               idx_j * shape::InnerRows;
      } else {
        static_assert((is_Nz_layout<shape>::value) ||
                          (is_Zn_layout<shape>::value),
                      "illegal layout");
      }
    } else {
      return i * shape::RowStride + j * shape::ColStride;
    }
  }
}

template <typename tile_shape>
const char* get_layout_str() {
  if constexpr (!tile_shape::isBoxedLayout) {
    if constexpr (tile_shape::isRowMajor)
      return "RowMajor";
    return "ColMajor";
  }
  if constexpr (is_Nz_layout<tile_shape>::value)
    return "NzLayout";
  if constexpr (is_Zn_layout<tile_shape>::value)
    return "ZnLayout";
  return "Other";
}

template <typename tile_shape>
void print_tile_info() {
#ifndef __linx
  std::cout << "Tile Rows Number: " << tile_shape::Rows << std::endl;
  std::cout << "Tile Columns Number: " << tile_shape::Cols << std::endl;
  std::cout << "Tile Active Rows Number: " << tile_shape::ValidRow << std::endl;
  std::cout << "Tile Active Columns Number: " << tile_shape::ValidCol << std::endl;
  if constexpr (tile_shape::isBoxedLayout) {
    std::cout << "Tile Fractal Inner Rows Number: " << tile_shape::InnerRows << std::endl;
    std::cout << "Tile Fractal Inner Columns Number: " << tile_shape::InnerCols << std::endl;
  }
  std::cout << "Tile Size: " << tile_shape::Numel << std::endl;
  std::cout << "Tile Layout: " << get_layout_str<tile_shape>() << std::endl;
  std::cout << "Tile Data Dump: " << std::endl;
#else
  (void)sizeof(tile_shape);
#endif
}

} // namespace pto

#endif

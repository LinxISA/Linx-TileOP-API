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
// PTO 0.58 B.DATR CMode[31:29] encoding. Values are explicit and MUST match
// the ISA: EQ=0 NE=1 LT=2 GT=3 LE=4 GE=5 (do not rely on declaration order).
enum class CmpMode : uint8_t {
  EQ = 0,  ///< Equal (==)
  NE = 1,  ///< Not equal (!=)
  LT = 2,  ///< Less than (<)
  GT = 3,  ///< Greater than (>)
  LE = 4,  ///< Less than or equal (<=)
  GE = 5,  ///< Greater than or equal (>=)
};

/// Compile-time validity check for the six ISA comparison modes. Rejects any
/// out-of-range value that a bogus static_cast would otherwise smuggle into
/// the B.DATR CMode field.
constexpr bool is_valid_cmp_mode(CmpMode Mode) {
  switch (Mode) {
  case CmpMode::EQ:
  case CmpMode::NE:
  case CmpMode::LT:
  case CmpMode::GT:
  case CmpMode::LE:
  case CmpMode::GE:
    return true;
  }
  return false;
}

/// CmpMode -> B.DATR CMode[31:29] immediate (the enum value itself).
constexpr unsigned cmp_mode_code(CmpMode Mode) {
  return static_cast<unsigned>(Mode);
}

// Rounding modes for TQUANT/TDEQUANT. The values are PTO bundle RMode
// encodings; encoding zero selects the operation default, which is RNE here.
enum class RoundMode : uint8_t {
  RNE = 0,    ///< round to nearest even / operation default
  RTZ = 2,    ///< round toward zero
  RTM = 3,    ///< round toward minus infinity
  RTP = 4,    ///< round toward plus infinity
  RNA = 5,    ///< round to nearest, ties away
  RTO = 6,    ///< round to odd
  RHB = 7,    ///< reciprocal-half bias

  // Compatibility aliases for the former direction-based names.
  RNONE = RNE,
  RDN = RTM,
  RUP = RTP,
};

constexpr bool is_valid_round_mode(RoundMode Mode) {
  switch (Mode) {
  case RoundMode::RNE:
  case RoundMode::RTZ:
  case RoundMode::RTM:
  case RoundMode::RTP:
  case RoundMode::RNA:
  case RoundMode::RTO:
  case RoundMode::RHB:
    return true;
  }
  return false;
}

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
  bool TransA = false;
  bool TransB = false;
  bool CScaleEn = false;

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

  constexpr FixpAttr transpose_a(bool Enable = true) const {
    FixpAttr Attr = *this;
    Attr.TransA = Enable;
    return Attr;
  }

  constexpr FixpAttr transpose_b(bool Enable = true) const {
    FixpAttr Attr = *this;
    Attr.TransB = Enable;
    return Attr;
  }

  constexpr FixpAttr cscale_enable(bool Enable = true) const {
    FixpAttr Attr = *this;
    Attr.CScaleEn = Enable;
    return Attr;
  }

  constexpr uint32_t encoding() const {
    return (static_cast<uint32_t>(PreQuant) << 26) |
           (static_cast<uint32_t>(Relu) << 23) |
           (static_cast<uint32_t>(GroupNCode) << 19) |
           (static_cast<uint32_t>(RowMaxEn) << 18) |
           (static_cast<uint32_t>(GroupMaxEn) << 17) |
           (static_cast<uint32_t>(RowMaxInit) << 16) |
           (static_cast<uint32_t>(MaxAbsEn) << 15) |
           (static_cast<uint32_t>(TransB) << 8) |
           (static_cast<uint32_t>(TransA) << 7) |
           (static_cast<uint32_t>(CScaleEn) << 9) | 0x2023;
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

enum class MatrixNumericClass : uint8_t {
  Unsupported,
  Floating,
  Signed,
  Unsigned,
};

constexpr MatrixNumericClass matrix_numeric_class(int TypeCode) {
  switch (TypeCode) {
  case __type_fp32:
  case __type_tf32:
  case __type_hf32:
  case __type_fp16:
  case __type_bf16:
  case __type_hif8:
  case __type_fp8_e4m3:
  case __type_fp8_e5m2:
  case __type_fp6_e3m2:
  case __type_fp5_e2m3:
  case __type_fp4_e2m1x2:
  case __type_fp4_e1m2x2:
    return MatrixNumericClass::Floating;
  case __type_int16:
  case __type_int8:
  case __type_int4x2:
    return MatrixNumericClass::Signed;
  case __type_uint16:
  case __type_uint8:
  case __type_uint4x2:
    return MatrixNumericClass::Unsigned;
  default:
    return MatrixNumericClass::Unsupported;
  }
}

constexpr bool matrix_mx_input_supported(int TypeCode) {
  return TypeCode == __type_fp16 || TypeCode == __type_bf16 ||
         TypeCode == __type_fp8_e4m3 || TypeCode == __type_fp8_e5m2 ||
         TypeCode == __type_fp4_e2m1x2 || TypeCode == __type_fp4_e1m2x2;
}

constexpr bool matrix_mx_input_needs_scale(int TypeCode) {
  return matrix_mx_input_supported(TypeCode) &&
         TypeCode != __type_fp16 && TypeCode != __type_bf16;
}

constexpr int matrix_accumulator_type_code(int InputTypeCode) {
  switch (matrix_numeric_class(InputTypeCode)) {
  case MatrixNumericClass::Floating:
    return __type_fp32;
  case MatrixNumericClass::Signed:
    return __type_int32;
  case MatrixNumericClass::Unsigned:
    return __type_uint32;
  default:
    return -1;
  }
}

constexpr bool matrix_mode_uses_s32_accumulator(FixpPreQuantMode Mode) {
  switch (Mode) {
  case FixpPreQuantMode::VREQS8Pre:
  case FixpPreQuantMode::REQS8Pre:
  case FixpPreQuantMode::VDEQF16:
  case FixpPreQuantMode::DEQF16:
  case FixpPreQuantMode::VSHIFTS322S16:
  case FixpPreQuantMode::SHIFTS322S16:
  case FixpPreQuantMode::QF322S4Pre:
  case FixpPreQuantMode::VQF322S4Pre:
  case FixpPreQuantMode::QF322S16Pre:
  case FixpPreQuantMode::VQF322S16Pre:
  case FixpPreQuantMode::QS322BF16Pre:
  case FixpPreQuantMode::VQS322BF16Pre:
    return true;
  default:
    return false;
  }
}

template <typename Left, typename Right, bool MX = false>
constexpr bool matrix_input_pair_legal() {
  constexpr int LeftCode = type_traits<typename Left::DType>::TypeCode;
  constexpr int RightCode = type_traits<typename Right::DType>::TypeCode;
  if constexpr (MX)
    return matrix_mx_input_supported(LeftCode) &&
           matrix_mx_input_supported(RightCode);
  constexpr MatrixNumericClass LeftClass = matrix_numeric_class(LeftCode);
  return LeftClass != MatrixNumericClass::Unsupported &&
         LeftClass == matrix_numeric_class(RightCode);
}

template <FixpAttr Attr, typename Left, typename Right, bool MX = false>
constexpr bool matrix_accumulator_mode_legal() {
  if constexpr (!matrix_input_pair_legal<Left, Right, MX>())
    return false;
  constexpr int AccCode = MX
      ? __type_fp32
      : matrix_accumulator_type_code(
            type_traits<typename Left::DType>::TypeCode);
  if constexpr (Attr.PreQuant == FixpPreQuantMode::None)
    return AccCode == __type_fp32 || AccCode == __type_int32 ||
           AccCode == __type_uint32;
  if constexpr (matrix_mode_uses_s32_accumulator(Attr.PreQuant))
    return AccCode == __type_int32;
  return AccCode == __type_fp32;
}

template <FixpAttr Attr, typename DType>
constexpr bool is_fixp_output_type();

template <FixpAttr Attr, typename Left, typename Right, typename Output,
          bool MX = false>
constexpr bool matrix_output_type_legal() {
  if constexpr (!matrix_accumulator_mode_legal<Attr, Left, Right, MX>())
    return false;
  constexpr int OutputCode = type_traits<typename Output::DType>::TypeCode;
  if constexpr (Attr.PreQuant == FixpPreQuantMode::None) {
    constexpr int AccCode = MX
        ? __type_fp32
        : matrix_accumulator_type_code(
              type_traits<typename Left::DType>::TypeCode);
    return OutputCode == AccCode;
  }
  return is_fixp_output_type<Attr, typename Output::DType>();
}

template <typename Left, typename Right, typename Accumulator, bool MX = false>
constexpr bool matrix_accumulator_type_legal() {
  if constexpr (!matrix_input_pair_legal<Left, Right, MX>())
    return false;
  constexpr int Expected = MX
      ? __type_fp32
      : matrix_accumulator_type_code(
            type_traits<typename Left::DType>::TypeCode);
  return type_traits<typename Accumulator::DType>::TypeCode == Expected;
}

template <FixpAttr Attr, typename DType>
constexpr bool is_fixp_output_type() {
  constexpr int TypeCode = type_traits<DType>::TypeCode;
  if constexpr (Attr.PreQuant == FixpPreQuantMode::None)
    // PreQuant=None preserves the derived FP32/S32/U32 AccType.
    return TypeCode == __type_fp32 || TypeCode == __type_int32 ||
           TypeCode == __type_uint32;
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

  inline size_t GetStrideBytes(const int dim) const {
    const size_t elements = static_cast<size_t>(GetStride(dim));
    return (elements * type_traits<DType>::bits + 7) / 8;
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
  static constexpr bool IsCubeLayout =
      BFractal_ == BLayout::CubeM16 || BFractal_ == BLayout::CubeM32 ||
      BFractal_ == BLayout::CubeN8;
  static constexpr int RowStride = BFractal_ == BLayout::RowMajor ? Cols :
                                   BFractal_ == BLayout::ColMajor ? 1 : 0;
  static constexpr int ColStride = BFractal_ == BLayout::RowMajor ? 1 :
                                   BFractal_ == BLayout::ColMajor ? Rows : 0;

  static constexpr int CubeCellBytes = 128;
  static constexpr int CubeElementBits =
      type_traits<DType>::TypeCode == __type_fp4_e2m1x2 ||
              type_traits<DType>::TypeCode == __type_fp4_e1m2x2 ||
              type_traits<DType>::TypeCode == __type_fp4_hif4x2 ||
              type_traits<DType>::TypeCode == __type_int4x2 ||
              type_traits<DType>::TypeCode == __type_uint4x2
          ? 4
          : type_traits<DType>::bits;
  static constexpr int CubeCellRows = [] {
    if constexpr (BFractal_ == BLayout::CubeM16) return 16;
    if constexpr (BFractal_ == BLayout::CubeM32) return 32;
    if constexpr (BFractal_ == BLayout::CubeN8)
      return CubeElementBits == 32 ? 4 : CubeElementBits == 16 ? 8
                                  : CubeElementBits == 8      ? 16
                                                               : 32;
    return 0;
  }();
  static constexpr int CubeCellCols = [] {
    if constexpr (BFractal_ == BLayout::CubeN8) return 8;
    if constexpr (BFractal_ == BLayout::CubeM16)
      return 128 * 8 / (16 * CubeElementBits);
    if constexpr (BFractal_ == BLayout::CubeM32)
      return 128 * 8 / (32 * CubeElementBits);
    return 0;
  }();
  static constexpr int CubeStorageRows = [] {
    if constexpr (BFractal_ == BLayout::CubeN8)
      return ((Rows + CubeCellRows - 1) / CubeCellRows) * CubeCellRows;
    if constexpr (IsCubeLayout) return CubeCellRows;
    return Rows;
  }();
  static constexpr int CubeStorageCols = IsCubeLayout
      ? ((Cols + CubeCellCols - 1) / CubeCellCols) * CubeCellCols : Cols;
  static constexpr int CubeCellCount = IsCubeLayout
      ? (CubeStorageRows / CubeCellRows) * (CubeStorageCols / CubeCellCols) : 0;
  static constexpr int CubeRequiredBytes =
      IsCubeLayout ? CubeCellCount * CubeCellBytes : 0;
  static constexpr int CubeLoadLayout =
      BFractal_ == BLayout::CubeM32 ? LayoutCvtEnum::ND2M32 :
      BFractal_ == BLayout::CubeM16 ? LayoutCvtEnum::ND2M16 :
      BFractal_ == BLayout::CubeN8  ? LayoutCvtEnum::ND2N8 : -1;
  static constexpr int CubeStoreLayout =
      BFractal_ == BLayout::CubeM32 ? LayoutCvtEnum::M322ND :
      BFractal_ == BLayout::CubeM16 ? LayoutCvtEnum::M162ND :
      BFractal_ == BLayout::CubeN8  ? LayoutCvtEnum::N82ND : -1;
  static constexpr int round_capacity(int bytes) {
    int capacity = 128;
    while (capacity < bytes) capacity *= 2;
    return capacity;
  }
  static constexpr int StorageBytes =
      round_capacity(IsCubeLayout
                         ? CubeRequiredBytes
                         : (Rows * Cols * type_traits<DType>::bits + 7) / 8);
  static constexpr int CubeStorageIndex(int row, int column) {
    const int cell_elements = CubeCellRows * CubeCellCols;
    if constexpr (BFractal_ == BLayout::CubeN8) {
      const int k_repeat = CubeStorageRows / CubeCellRows;
      const int cell_k = row / CubeCellRows;
      const int cell_n = column / CubeCellCols;
      const int inner_row = row % CubeCellRows;
      const int inner_column = column % CubeCellCols;
      const int local = inner_column * CubeCellRows + inner_row;
      return (cell_n * k_repeat + cell_k) * cell_elements + local;
    }
    int mapped_column = column % CubeCellCols;
    if constexpr (BFractal_ == BLayout::CubeM16) {
      if constexpr (CubeElementBits == 4) {
        if (mapped_column >= 4 && mapped_column < 8)
          mapped_column += 4;
        else if (mapped_column >= 8 && mapped_column < 12)
          mapped_column -= 4;
      }
    }
    const int cell_index = column / CubeCellCols;
    const int local = row * CubeCellCols + mapped_column;
    return cell_index * cell_elements + local;
  }

  static constexpr int kBytes = (Rows_ * Cols_ * type_traits<DType>::bits + 7) / 8;
  // static_assert(kBytes % 512 == 0, "Tile size must be 512 bytes aligned");
  // static_assert(((kBytes / 512 - 1) & (kBytes / 512)) == 0, "Tile size must by (512 * 2 ^ n) Bytes");

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
  static_assert(!IsCubeLayout ||
                    ((CubeElementBits == 4 || CubeElementBits == 8 ||
                      CubeElementBits == 16 || CubeElementBits == 32) &&
                     type_traits<DType>::TypeCode != __type_fp4_hif4x2),
                "CUBE CELL layouts support only 4/8/16/32-bit element widths "
                "and reject HiF4X2");
  static_assert(BFractal_ != BLayout::CubeM16 || Rows <= 16,
                "CUBE_M16 supports at most 16 logical rows");
  static_assert(BFractal_ != BLayout::CubeM32 || Rows <= 32,
                "CUBE_M32 supports at most 32 logical rows");

  static constexpr BLayout BFractal = BFractal_;
  static constexpr SLayout SFractal = SFractal_;
  static constexpr int Numel = Rows * Cols;
  static constexpr bool isRowMajor = BFractal_ == BLayout::RowMajor;

  static constexpr int SFractalSize = SFractalSize_;
  static constexpr PadValue PadVal = PadVal_;
  static constexpr CompactMode Compact = Compact_;
  static constexpr int LogicalTileBytes = StorageBytes;
  static constexpr int TilesizeCode =
      LogicalTileBytes == 128  ? __tilesize_128B :
      LogicalTileBytes == 256  ? __tilesize_256B :
      LogicalTileBytes == 512  ? __tilesize_512B :
      LogicalTileBytes == 1024 ? __tilesize_1KB :
      LogicalTileBytes == 2048 ? __tilesize_2KB :
      LogicalTileBytes == 4096 ? __tilesize_4KB :
      LogicalTileBytes == 8192 ? __tilesize_8KB :
      LogicalTileBytes == 16384 ? __tilesize_16KB :
      LogicalTileBytes == 32768 ? __tilesize_32KB :
      LogicalTileBytes == 65536 ? __tilesize_64KB :
      LogicalTileBytes == 131072 ? __tilesize_128KB :
      LogicalTileBytes == 262144 ? __tilesize_256KB : __tilesize_unknown;
  static constexpr bool IsValidActiveSize =
      TilesizeCode >= __tilesize_128B && TilesizeCode <= __tilesize_256KB;

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
      IsCubeLayout ||
      SFractal_ == SLayout::NoneBox ||
      (SFractal_ != SLayout::NoneBox) && (Rows % InnerRows == 0 && Cols % InnerCols == 0),
      "Boxed layouts require Rows/Cols to be integer multiples of their inner box."
        );

  static_assert(SFractalSize_ == 512 || SFractalSize_ == 1024,
                "SFractalSize_ illegal");

#ifdef __linx
  using TileDType = linx_tile_carrier<LogicalTileBytes>;
  using TileRegisterType = typename TileDType::RegisterType;
#else
  using TileDType = DType[StorageBytes * 8 / type_traits<DType>::bits];
#endif

#ifdef __linx
  TileRegisterType &data() { return data_.Register; }
  const TileRegisterType &data() const { return data_.Register; }
#else
  TileDType &data() { return data_; }
  const TileDType &data() const { return data_; }
#endif

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

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using CubeTileM16 =
  Tile<Location::Left, Element_, Rows_, Cols_, BLayout::CubeM16,
       RowValid_, ColValid_>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using CubeTileM32 =
  Tile<Location::Left, Element_, Rows_, Cols_, BLayout::CubeM32,
       RowValid_, ColValid_>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using CubeTileN8 =
  Tile<Location::Right, Element_, Rows_, Cols_, BLayout::CubeN8,
       RowValid_, ColValid_>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using CubeAccumulatorM16 =
  Tile<Location::Acc, Element_, Rows_, Cols_, BLayout::CubeM16,
       RowValid_, ColValid_>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using CubeAccumulatorM32 =
  Tile<Location::Acc, Element_, Rows_, Cols_, BLayout::CubeM32,
       RowValid_, ColValid_>;

// Cooperative Shared matrix primaries are published as ordinary RowMajor
// rectangles. Their Left/Right role controls CUBE operand ordering; CELL
// layout is materialized only for Local matrix storage and destinations.
template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using SharedMatrixLeft =
  Tile<Location::Left, Element_, Rows_, Cols_, BLayout::RowMajor,
       RowValid_, ColValid_>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using SharedMatrixRight =
  Tile<Location::Right, Element_, Rows_, Cols_, BLayout::RowMajor,
       RowValid_, ColValid_>;

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
  size_t GetStrideBytes(int dim) const { return impl_.GetStrideBytes(dim); }

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
  static constexpr int RowStride = LocalTile::RowStride;
  static constexpr int ColStride = LocalTile::ColStride;
  static constexpr int ValidRow = LocalTile::ValidRow;
  static constexpr int ValidCol = LocalTile::ValidCol;
  static constexpr BLayout BFractal = LocalTile::BFractal;
  static constexpr SLayout SFractal = LocalTile::SFractal;
  static constexpr int SFractalSize = LocalTile::SFractalSize;
  static constexpr PadValue PadVal = LocalTile::PadVal;
  static constexpr CompactMode Compact = LocalTile::Compact;
  static constexpr bool IsCubeLayout = LocalTile::IsCubeLayout;
  using TileDType = typename LocalTile::TileDType;
  static constexpr int LogicalTileBytes = LocalTile::LogicalTileBytes;
  static constexpr int TilesizeCode = LocalTile::TilesizeCode;
  static constexpr bool IsValidActiveSize = LocalTile::IsValidActiveSize;
  static constexpr bool isRowMajor = LocalTile::isRowMajor;
  static constexpr bool isBoxedLayout = LocalTile::isBoxedLayout;
  static constexpr bool isInnerRowMajor = LocalTile::isInnerRowMajor;
  static constexpr bool isInnerColMajor = LocalTile::isInnerColMajor;
  static constexpr int InnerRows = LocalTile::InnerRows;
  static constexpr int InnerCols = LocalTile::InnerCols;
  static constexpr int InnerNumel = LocalTile::InnerNumel;
  static constexpr int Numel = LocalTile::Numel;
  static constexpr int byteSize = LocalTile::byteSize;
  static constexpr int kBytes = LocalTile::kBytes;

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

// MX scale operands may be any tile in either storage class: Local (Vec or
// Left/Right) matching the plain TMatmulAllOptions usage, or Shared when the
// paired matrix/vector is Shared (handoff Sec 1.5: MX Shared pair is Shared
// B/ScaleB, or all four Shared). Storage pairing is resolved by the emitter's
// A/B dispatch; a mismatched storage fails at the asm constraint (Tr vs Sr),
// so the concept only accepts "some tile", not a specific role or storage.
template <typename T>
concept is_any_tile_data_v = is_tile<T>::value || is_shared_tile<T>::value;

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
                "FPATR config GroupN must be 8, 16, 32, 48, 64, 80, 96, "
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
          typename GroupMaxOut_ = NoOperand, typename CScaleTile_ = NoOperand>
struct Options {
  static constexpr FixpAttr Attr = Attr_;
  using QuantTile = QuantTile_;
  using ReluTile = ReluTile_;
  using RowMaxIn = RowMaxIn_;
  using RowMaxOut = RowMaxOut_;
  using GroupMaxOut = GroupMaxOut_;
  using CScaleTile = CScaleTile_;

  uint64_t QuantDescriptor = 0;
  uint64_t LReluDescriptor = 0;
  QuantTile *Quant = nullptr;
  ReluTile *Relu = nullptr;
  RowMaxIn *RowIn = nullptr;
  RowMaxOut *RowOut = nullptr;
  GroupMaxOut *GroupOut = nullptr;
  CScaleTile *CScale = nullptr;

  constexpr Options() = default;

  constexpr Options(uint64_t QuantDescriptor, uint64_t LReluDescriptor,
                    QuantTile *Quant, ReluTile *Relu, RowMaxIn *RowIn,
                    RowMaxOut *RowOut, GroupMaxOut *GroupOut,
                    CScaleTile *CScale = nullptr)
      : QuantDescriptor(QuantDescriptor), LReluDescriptor(LReluDescriptor),
        Quant(Quant), Relu(Relu), RowIn(RowIn), RowOut(RowOut),
        GroupOut(GroupOut), CScale(CScale) {}

  template <bool Enable = true> constexpr auto transpose_a() const {
    constexpr FixpAttr NewAttr = Attr.transpose_a(Enable);
    return Options<NewAttr, QuantTile, ReluTile, RowMaxIn, RowMaxOut,
                   GroupMaxOut, CScaleTile>(QuantDescriptor, LReluDescriptor,
                                            Quant, Relu, RowIn, RowOut,
                                            GroupOut, CScale);
  }

  template <bool Enable = true> constexpr auto transpose_b() const {
    constexpr FixpAttr NewAttr = Attr.transpose_b(Enable);
    return Options<NewAttr, QuantTile, ReluTile, RowMaxIn, RowMaxOut,
                   GroupMaxOut, CScaleTile>(QuantDescriptor, LReluDescriptor,
                                            Quant, Relu, RowIn, RowOut,
                                            GroupOut, CScale);
  }

  template <bool Enable = true> constexpr auto cscale_enable() const {
    constexpr FixpAttr NewAttr = Attr.cscale_enable(Enable);
    return Options<NewAttr, QuantTile, ReluTile, RowMaxIn, RowMaxOut,
                   GroupMaxOut, CScaleTile>(QuantDescriptor, LReluDescriptor,
                                            Quant, Relu, RowIn, RowOut,
                                            GroupOut, CScale);
  }

  constexpr auto relu() const {
    static_assert(Attr.Relu == FixpReluMode::None,
                  "FPATR config ReLU mode was already selected");
    constexpr FixpAttr NewAttr = with_relu(Attr, FixpReluMode::Relu);
    return Options<NewAttr, QuantTile, ReluTile, RowMaxIn, RowMaxOut,
                   GroupMaxOut, CScaleTile>(QuantDescriptor, LReluDescriptor,
                                            Quant, Relu, RowIn, RowOut,
                                            GroupOut, CScale);
  }

  constexpr auto lrelu(uint64_t Descriptor) const {
    static_assert(Attr.Relu == FixpReluMode::None,
                  "FPATR config ReLU mode was already selected");
    constexpr FixpAttr NewAttr = with_relu(Attr, FixpReluMode::LRelu);
    return Options<NewAttr, QuantTile, ReluTile, RowMaxIn, RowMaxOut,
                   GroupMaxOut, CScaleTile>(QuantDescriptor, Descriptor, Quant,
                                            Relu, RowIn, RowOut, GroupOut,
                                            CScale);
  }

  template <is_local_tile_v Tile>
  constexpr auto prelu(Tile &Parameter) const {
    static_assert(Attr.Relu == FixpReluMode::None,
                  "FPATR config ReLU mode was already selected");
    static_assert(std::is_same_v<ReluTile, NoOperand>,
                  "FPATR config PReLU Tile was already supplied");
    constexpr FixpAttr NewAttr = with_relu(Attr, FixpReluMode::PRelu);
    return Options<NewAttr, QuantTile, Tile, RowMaxIn, RowMaxOut,
                   GroupMaxOut, CScaleTile>(QuantDescriptor, LReluDescriptor,
                                            Quant, &Parameter, RowIn, RowOut,
                                            GroupOut, CScale);
  }

  template <is_local_tile_v Tile>
  constexpr auto row_max(Tile &Output) const {
    static_assert(!Attr.RowMaxEn,
                  "FPATR config RowMax was already enabled");
    static_assert(std::is_same_v<RowMaxIn, NoOperand> &&
                      std::is_same_v<RowMaxOut, NoOperand>,
                  "FPATR config RowMax operands were already supplied");
    constexpr FixpAttr NewAttr = with_row_max(Attr, false);
    return Options<NewAttr, QuantTile, ReluTile, NoOperand, Tile,
                   GroupMaxOut, CScaleTile>(QuantDescriptor, LReluDescriptor,
                                            Quant, Relu, nullptr, &Output,
                                            GroupOut, CScale);
  }

  template <is_local_tile_v InputTile, is_local_tile_v OutputTile>
  constexpr auto row_max(InputTile &Input, OutputTile &Output) const {
    static_assert(!Attr.RowMaxEn,
                  "FPATR config RowMax was already enabled");
    static_assert(std::is_same_v<RowMaxIn, NoOperand> &&
                      std::is_same_v<RowMaxOut, NoOperand>,
                  "FPATR config RowMax operands were already supplied");
    constexpr FixpAttr NewAttr = with_row_max(Attr, true);
    return Options<NewAttr, QuantTile, ReluTile, InputTile, OutputTile,
                   GroupMaxOut, CScaleTile>(QuantDescriptor, LReluDescriptor,
                                            Quant, Relu, &Input, &Output,
                                            GroupOut, CScale);
  }

  template <int GroupN, is_local_tile_v Tile>
  constexpr auto group_max(Tile &Output) const {
    static_assert(!Attr.GroupMaxEn,
                  "FPATR config GroupMax was already enabled");
    static_assert(std::is_same_v<GroupMaxOut, NoOperand>,
                  "FPATR config GroupMax output was already supplied");
    constexpr FixpAttr NewAttr =
        with_group_max(Attr, group_n_code<GroupN>());
    return Options<NewAttr, QuantTile, ReluTile, RowMaxIn, RowMaxOut, Tile,
                   CScaleTile>(QuantDescriptor, LReluDescriptor, Quant, Relu,
                               RowIn, RowOut, &Output, CScale);
  }

  constexpr auto max_abs() const {
    static_assert(Attr.RowMaxEn || Attr.GroupMaxEn,
                  "FPATR config max_abs requires RowMax or GroupMax");
    constexpr FixpAttr NewAttr = with_max_abs(Attr);
    return Options<NewAttr, QuantTile, ReluTile, RowMaxIn, RowMaxOut,
                   GroupMaxOut, CScaleTile>(QuantDescriptor, LReluDescriptor,
                                            Quant, Relu, RowIn, RowOut,
                                            GroupOut, CScale);
  }

  template <is_local_tile_v Tile>
  constexpr auto cscale(Tile &Scale) const {
    static_assert(std::is_same_v<CScaleTile, NoOperand>,
                  "FPATR CScale tile was already supplied");
    constexpr FixpAttr NewAttr = Attr.cscale_enable();
    return Options<NewAttr, QuantTile, ReluTile, RowMaxIn, RowMaxOut,
                   GroupMaxOut, Tile>(QuantDescriptor, LReluDescriptor, Quant,
                                      Relu, RowIn, RowOut, GroupOut, &Scale);
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
          typename RowMaxIn, typename RowMaxOut, typename GroupMaxOut,
          typename CScaleTile>
struct is_options<Options<Attr, QuantTile, ReluTile, RowMaxIn, RowMaxOut,
                          GroupMaxOut, CScaleTile>> : std::true_type {};

template <typename T>
concept is_options_v = is_options<std::remove_cv_t<T>>::value;

} // namespace fixp


template <typename shape> int index(int i, int j) {
  if constexpr (is_global_data_v<shape>) {
    return i * shape::RowStride + j * shape::ColStride;
  } else if constexpr (is_tile_data_v<shape>) {
    if constexpr (shape::IsCubeLayout) {
      return shape::CubeStorageIndex(i, j);
    } else
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

//===--- Tile datatype reinterpret view (PTO v0.58) ---===//
// reinterpret_tile<NewDType>(src): zero-instruction datatype reinterpret.
// The underlying Tile register/storage bit pattern is unchanged; only the
// static DType (and downstream ISA datatype encoding) is re-interpreted by
// the view. First phase: Local Tile -> Local view, equal-bit-width only,
// layout/shape/valid/Location preserved, no TCVT, no payload copy.
namespace pto {

// Whether T has a PTO TypeCode (has a type_traits specialization).
template <typename T, typename = void> struct has_ptotype_traits : std::false_type {};
template <typename T>
struct has_ptotype_traits<T, std::void_t<decltype(type_traits<T>::TypeCode)>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_supported_dtype_v =
    has_ptotype_traits<std::remove_cv_t<T>>::value &&
    (type_traits<std::remove_cv_t<T>>::bits > 0);

// Zero-instruction datatype view over an existing Local Tile. Storage carrier
// (TileDType) and data() forward to the source; DType and all shape/role
// statics are re-declared so downstream ops see NewDType while binding the
// source's exact Tile register.
template <typename NewDType, typename SourceTile>
class ReinterpretedTileView {
public:
  using DType = NewDType;
  using Source = SourceTile;
  // Same storage carrier as the source Tile, so its physical bytes and
  // tile_type_traits<...TileDType>::TilesizeCode are unchanged by the
  // reinterpret.
  using TileDType = typename SourceTile::TileDType;

  static constexpr Location Loc = SourceTile::Loc;
  static constexpr int Rows = SourceTile::Rows;
  static constexpr int Cols = SourceTile::Cols;
  static constexpr int RowStride = SourceTile::RowStride;
  static constexpr int ColStride = SourceTile::ColStride;
  static constexpr int ValidRow = SourceTile::ValidRow;
  static constexpr int ValidCol = SourceTile::ValidCol;
  static constexpr BLayout BFractal = SourceTile::BFractal;
  static constexpr SLayout SFractal = SourceTile::SFractal;
  static constexpr int SFractalSize = SourceTile::SFractalSize;
  static constexpr PadValue PadVal = SourceTile::PadVal;
  static constexpr CompactMode Compact = SourceTile::Compact;
  static constexpr bool isRowMajor = SourceTile::isRowMajor;
  static constexpr bool isBoxedLayout = SourceTile::isBoxedLayout;
  static constexpr bool isInnerRowMajor = SourceTile::isInnerRowMajor;
  static constexpr bool isInnerColMajor = SourceTile::isInnerColMajor;
  static constexpr int InnerRows = SourceTile::InnerRows;
  static constexpr int InnerCols = SourceTile::InnerCols;
  static constexpr int InnerNumel = SourceTile::InnerNumel;
  static constexpr int Numel = SourceTile::Numel;
  static constexpr int byteSize = SourceTile::byteSize;
  // Physical storage identity: the view occupies exactly the source bytes.
  static constexpr int kBytes = SourceTile::kBytes;
  static constexpr int LogicalTileBytes = SourceTile::LogicalTileBytes;
  static constexpr int TilesizeCode = SourceTile::TilesizeCode;
  static constexpr bool IsValidActiveSize = SourceTile::IsValidActiveSize;

  explicit constexpr ReinterpretedTileView(SourceTile &Source)
      : SourceValue(Source) {}

  // Same register carrier as the source (no copy). Only const access is
  // exposed for const sources; the non-const path keeps the same carrier.
  decltype(auto) data() { return SourceValue.data(); }
  decltype(auto) data() const { return SourceValue.data(); }

  template <int RowMask = ValidRow>
  static constexpr std::enable_if_t<(RowMask > 0), int> GetValidRow() {
    return SourceTile::template GetValidRow<RowMask>();
  }
  template <int RowMask = ValidRow>
  std::enable_if_t<RowMask == -1, int> GetValidRow() const {
    return SourceValue.GetValidRow();
  }
  template <int ColMask = ValidCol>
  static constexpr std::enable_if_t<(ColMask > 0), int> GetValidCol() {
    return SourceTile::template GetValidCol<ColMask>();
  }
  template <int ColMask = ValidCol>
  std::enable_if_t<ColMask == -1, int> GetValidCol() const {
    return SourceValue.GetValidCol();
  }

private:
  SourceTile &SourceValue;
};

// A ReinterpretedTileView is a Local tile-shaped operand (not Shared).
template <typename NewDType, typename SourceTile>
struct is_tile<ReinterpretedTileView<NewDType, SourceTile>> : std::true_type {
  static constexpr SLayout layout_enum = SourceTile::SFractal;
};

// ---- Range modifiers (PTO-ISA 0.58.4, ADR-0098) --------------------------
//
// B.SUBVIEW / B.ASSEMBLE attach to the immediately preceding B.IOT/B.IOS
// binder and carry a range descriptor (RegSrc + uimm11 offset + parent size).
// These wrappers model source (Subview) and destination (Assemble) range
// carriers without creating a second Tile register namespace: they forward
// the parent's shape/dtype/storage and expose the modifier runtime fields.
namespace range {

constexpr bool is_valid_parent_size_code(unsigned code) {
  return code <= 12; // 0 is legal on non-INIT modifiers; 13..15 reserved
}
constexpr bool is_valid_subview_size_code(unsigned code) {
  return code >= 1 && code <= 12;
}
constexpr bool is_valid_uimm11(unsigned u) { return u <= 2047; }

/// Source-side range carrier. Forwards every tile-shaped static member of
/// Parent so it can be bound as a Local operand; the B.SUBVIEW line is
/// emitted after the source binder by the consuming operation.
///
/// uimm11 Offset and RegSrc must be compile-time constants: the B.SUBVIEW
/// Modifier slots are inline-asm "i" constraints, so callers cannot thread
/// runtime values into a range descriptor (PTO-ISA 0.58.4 ADR-0098 models
/// the range descriptor as a static part of the binder contract). RegSrc is
/// the absolute GPR selector 0..23 for the base-address register (defaults
/// to a0/R2, the conventional base register).
template <typename Parent, unsigned SubviewSizeCode_, unsigned Offset_ = 0,
          unsigned RegSrc_ = 2>
class Subview {
  static_assert(is_valid_subview_size_code(SubviewSizeCode_),
                "B.SUBVIEW SubviewSizeCode must be 1..12 (128B..256KB per PE)");
  static_assert(is_valid_uimm11(Offset_),
                "B.SUBVIEW uimm11 offset must be 0..2047");
  static_assert(RegSrc_ <= 23,
                "B.SUBVIEW RegSrc must be an absolute GPR selector 0..23");
public:
  using DType = typename Parent::DType;
  using ParentTile = Parent;
  using TileDType = typename Parent::TileDType;

  static constexpr unsigned SubviewSizeCode = SubviewSizeCode_;
  static constexpr unsigned Offset = Offset_;
  static constexpr unsigned RegSrc = RegSrc_;
  static constexpr Location Loc = Parent::Loc;
  static constexpr int Rows = Parent::Rows;
  static constexpr int Cols = Parent::Cols;
  static constexpr int RowStride = Parent::RowStride;
  static constexpr int ColStride = Parent::ColStride;
  static constexpr int ValidRow = Parent::ValidRow;
  static constexpr int ValidCol = Parent::ValidCol;
  static constexpr BLayout BFractal = Parent::BFractal;
  static constexpr SLayout SFractal = Parent::SFractal;
  static constexpr int SFractalSize = Parent::SFractalSize;
  static constexpr bool isRowMajor = Parent::isRowMajor;
  static constexpr bool isBoxedLayout = Parent::isBoxedLayout;
  static constexpr bool isInnerRowMajor = Parent::isInnerRowMajor;
  static constexpr bool isInnerColMajor = Parent::isInnerColMajor;
  static constexpr int InnerRows = Parent::InnerRows;
  static constexpr int InnerCols = Parent::InnerCols;
  static constexpr int Numel = Parent::Numel;
  static constexpr int LogicalTileBytes = Parent::LogicalTileBytes;
  static constexpr int TilesizeCode = Parent::TilesizeCode;
  static constexpr bool IsValidActiveSize = Parent::IsValidActiveSize;

  Subview(Parent &parent, uintptr_t range_base = 0)
      : ParentValue(parent), RangeBaseValue(range_base) {}

  // A range carrier over an ordinary Local Tile binds through data(); a
  // carrier over a SharedTile binds through handle() (Shared uses the B.IOS
  // binder with the compiler's dedicated S register constraint, and has no
  // conventional data()).
  decltype(auto) data()
      requires(!is_shared_tile_v<Parent>) {
    return ParentValue.data();
  }
  unsigned long handle()
      requires(is_shared_tile_v<Parent>) {
    return ParentValue.handle();
  }
  unsigned long &handle_ref()
      requires(is_shared_tile_v<Parent>) {
    return ParentValue.handle_ref();
  }

  int GetValidRow() const { return ParentValue.GetValidRow(); }
  int GetValidCol() const { return ParentValue.GetValidCol(); }
  uintptr_t GetRangeBase() const { return RangeBaseValue; }

private:
  Parent &ParentValue;
  uintptr_t RangeBaseValue;
};

/// Destination-side range carrier. Capable of the multi-PE Shared
/// destination requirement (operation enforces it); carries INIT/LAST and
/// ParentSizeCode. INIT/LAST/Offset/RegSrc are compile-time constants for
/// the same inline-asm "i" constraint reason as Subview: the range
/// descriptor is a static part of the destination binder contract. RegSrc
/// is the absolute GPR selector 0..23 for the base-address register
/// (defaults to a0/R2, the conventional base register).
template <typename Parent, unsigned ParentSizeCode_, bool INIT_ = true,
          bool LAST_ = false, unsigned Offset_ = 0, unsigned RegSrc_ = 2>
class Assemble {
  static_assert(is_valid_parent_size_code(ParentSizeCode_),
                "B.ASSEMBLE ParentSizeCode must be 0..12; 13..15 reserved");
  static_assert(!((INIT_ == false) && (ParentSizeCode_ != 0)),
                "B.ASSEMBLE: non-INIT modifier requires ParentSizeCode=0");
  static_assert(!((INIT_ != false) && (ParentSizeCode_ == 0)),
                "B.ASSEMBLE: INIT modifier requires ParentSizeCode 1..12");
  static_assert(is_valid_uimm11(Offset_),
                "B.ASSEMBLE uimm11 offset must be 0..2047");
  static_assert(RegSrc_ <= 23,
                "B.ASSEMBLE RegSrc must be an absolute GPR selector 0..23");
public:
  using DType = typename Parent::DType;
  using ParentTile = Parent;
  using TileDType = typename Parent::TileDType;

  static constexpr unsigned ParentSizeCode = ParentSizeCode_;
  static constexpr bool INIT = INIT_;
  static constexpr bool LAST = LAST_;
  static constexpr unsigned Offset = Offset_;
  static constexpr unsigned RegSrc = RegSrc_;
  static constexpr Location Loc = Parent::Loc;
  static constexpr int Rows = Parent::Rows;
  static constexpr int Cols = Parent::Cols;
  static constexpr int RowStride = Parent::RowStride;
  static constexpr int ColStride = Parent::ColStride;
  static constexpr int ValidRow = Parent::ValidRow;
  static constexpr int ValidCol = Parent::ValidCol;
  static constexpr BLayout BFractal = Parent::BFractal;
  static constexpr SLayout SFractal = Parent::SFractal;
  static constexpr int SFractalSize = Parent::SFractalSize;
  static constexpr bool isRowMajor = Parent::isRowMajor;
  static constexpr bool isBoxedLayout = Parent::isBoxedLayout;
  static constexpr bool isInnerRowMajor = Parent::isInnerRowMajor;
  static constexpr bool isInnerColMajor = Parent::isInnerColMajor;
  static constexpr int InnerRows = Parent::InnerRows;
  static constexpr int InnerCols = Parent::InnerCols;
  static constexpr int Numel = Parent::Numel;
  static constexpr int LogicalTileBytes = Parent::LogicalTileBytes;
  static constexpr int TilesizeCode = Parent::TilesizeCode;
  static constexpr bool IsValidActiveSize = Parent::IsValidActiveSize;

  Assemble(Parent &parent, uintptr_t range_base = 0)
      : ParentValue(parent), RangeBaseValue(range_base) {}

  // Local binds through data(); SharedTile binds through handle() (B.IOS).
  decltype(auto) data()
      requires(!is_shared_tile_v<Parent>) {
    return ParentValue.data();
  }
  unsigned long handle()
      requires(is_shared_tile_v<Parent>) {
    return ParentValue.handle();
  }
  unsigned long &handle_ref()
      requires(is_shared_tile_v<Parent>) {
    return ParentValue.handle_ref();
  }

  int GetValidRow() const { return ParentValue.GetValidRow(); }
  int GetValidCol() const { return ParentValue.GetValidCol(); }
  uintptr_t GetRangeBase() const { return RangeBaseValue; }

private:
  Parent &ParentValue;
  uintptr_t RangeBaseValue;
};

// Ergonomic range factories. The common case derives the modifier size from
// the wrapped tile and keeps the descriptor details out of the call site.
template <typename Parent>
auto subview(Parent &parent, uintptr_t range_base = 0)
    -> Subview<Parent, Parent::TilesizeCode> {
  return {parent, range_base};
}

template <unsigned SubviewSizeCode_, typename Parent>
auto subview_sized(Parent &parent, uintptr_t range_base = 0)
    -> Subview<Parent, SubviewSizeCode_> {
  return {parent, range_base};
}

template <typename Parent>
auto assemble(Parent &parent, uintptr_t range_base = 0)
    -> Assemble<Parent, Parent::TilesizeCode> {
  return {parent, range_base};
}

template <typename Parent>
auto assemble_last(Parent &parent, uintptr_t range_base = 0)
    -> Assemble<Parent, 0, false, true> {
  return {parent, range_base};
}

template <typename Parent>
auto assemble_init_last(Parent &parent, uintptr_t range_base = 0)
    -> Assemble<Parent, Parent::TilesizeCode, true, true> {
  return {parent, range_base};
}

template <typename Parent>
auto assemble_middle(Parent &parent, uintptr_t range_base = 0)
    -> Assemble<Parent, 0, false, false> {
  return {parent, range_base};
}

template <unsigned Offset_, unsigned RegSrc_ = 2, typename Parent>
auto subview_at(Parent &parent, uintptr_t range_base = 0)
    -> Subview<Parent, Parent::TilesizeCode, Offset_, RegSrc_> {
  return {parent, range_base};
}

template <unsigned SubviewSizeCode_, unsigned Offset_,
          unsigned RegSrc_ = 2, typename Parent>
auto subview_sized_at(Parent &parent, uintptr_t range_base = 0)
    -> Subview<Parent, SubviewSizeCode_, Offset_, RegSrc_> {
  return {parent, range_base};
}

template <unsigned Offset_, unsigned RegSrc_ = 2, typename Parent>
auto assemble_at(Parent &parent, uintptr_t range_base = 0)
    -> Assemble<Parent, Parent::TilesizeCode, true, false, Offset_, RegSrc_> {
  return {parent, range_base};
}

template <unsigned Offset_, unsigned RegSrc_ = 2, typename Parent>
auto assemble_init_last_at(Parent &parent, uintptr_t range_base = 0)
    -> Assemble<Parent, Parent::TilesizeCode, true, true, Offset_, RegSrc_> {
  return {parent, range_base};
}

template <unsigned Offset_, unsigned RegSrc_ = 2, typename Parent>
auto assemble_middle_at(Parent &parent, uintptr_t range_base = 0)
    -> Assemble<Parent, 0, false, false, Offset_, RegSrc_> {
  return {parent, range_base};
}

template <unsigned Offset_, unsigned RegSrc_ = 2, typename Parent>
auto assemble_last_at(Parent &parent, uintptr_t range_base = 0)
    -> Assemble<Parent, 0, false, true, Offset_, RegSrc_> {
  return {parent, range_base};
}

} // namespace range

// Range wrappers count as Local tile-shaped operands for binding purposes.
template <typename Parent, unsigned SC, auto... Rest>
struct is_tile<range::Subview<Parent, SC, Rest...>> : std::true_type {
  static constexpr SLayout layout_enum = Parent::SFractal;
};
template <typename Parent, unsigned SC, auto... Rest>
struct is_tile<range::Assemble<Parent, SC, Rest...>> : std::true_type {
  static constexpr SLayout layout_enum = Parent::SFractal;
};

// Range-modifier traits: a Subview is a source-side range carrier and an
// Assemble is a destination-side range carrier.
template <typename T> struct is_subview : std::false_type {};
template <typename Parent, unsigned SC, auto... Rest>
struct is_subview<range::Subview<Parent, SC, Rest...>> : std::true_type {};
template <typename T> struct is_assemble : std::false_type {};
template <typename Parent, unsigned SC, auto... Rest>
struct is_assemble<range::Assemble<Parent, SC, Rest...>> : std::true_type {};

template <typename T>
concept is_subview_v = is_subview<T>::value;
template <typename T>
concept is_assemble_v = is_assemble<T>::value;
template <typename T>
concept is_range_v = is_subview<T>::value || is_assemble<T>::value;

// Equal bit-width is required: the reinterpret must not change the number of
// logical elements, physical bytes or TileSizeCode.
template <typename SourceTile, typename NewDType>
constexpr bool reinterpret_tile_equal_width_v =
    type_traits<typename SourceTile::DType>::bits ==
    type_traits<NewDType>::bits;

// The source must be an ordinary Local Tile (Shared is out of scope for the
// first phase; a Shared view would need the Sr binder contract).
template <typename SourceTile>
constexpr bool reinterpret_tile_source_is_local_v =
    is_tile<SourceTile>::value &&
    SourceTile::Loc != Location::Shared;

// The new dtype must be encodable in the source's layout. We accept any
// equal-width dtype whose type_traits exists; boxed/fractal layouts are
// preserved unchanged because rows/cols/inner box are untouched, so the
// existing Tile layout static_asserts remain valid for the same dimensions.
// A NewDType with no PTO TypeCode is rejected by is_supported_dtype_v.
template <typename SourceTile, typename NewDType>
constexpr bool reinterpret_tile_layout_legal_v =
    reinterpret_tile_equal_width_v<SourceTile, NewDType> &&
    is_supported_dtype_v<NewDType>;

// Physical storage preservation: same bytes, same TilesizeCode, same carrier.
template <typename SourceTile, typename NewDType>
constexpr bool reinterpret_tile_storage_compatible_v =
    reinterpret_tile_equal_width_v<SourceTile, NewDType>;

// The view must not dangle: it holds a reference, so it must not be bound to
// a temporary Tile. reinterpret_tile takes SourceTile& (non-const), which
// already rejects rvalues; the const overload takes const SourceTile&, which
// also rejects prvalue temporaries (they bind to const& only via materialized
// temporaries -- rejected by requiring a named lvalue at the call site).

/// Zero-instruction datatype reinterpret over a Local Tile.
template <typename NewDType, is_tile_data_v SourceTile>
inline auto reinterpret_tile(SourceTile &Source) {
  using OldDType = typename SourceTile::DType;
  static_assert(is_supported_dtype_v<NewDType>,
                "reinterpret_tile target dtype has no PTO TypeCode");
  static_assert(reinterpret_tile_source_is_local_v<SourceTile>,
                "reinterpret_tile first phase supports Local Tiles only"
                " (Shared requires a separate Shared view)");
  static_assert(reinterpret_tile_layout_legal_v<SourceTile, NewDType>,
                "reinterpret_tile requires equal-bit-width dtypes "
                "compatible with the source layout");
  static_assert(reinterpret_tile_storage_compatible_v<SourceTile, NewDType>,
                "reinterpret_tile must preserve the source Tile storage");
  return ReinterpretedTileView<NewDType, SourceTile>(Source);
}

} // namespace pto

#endif

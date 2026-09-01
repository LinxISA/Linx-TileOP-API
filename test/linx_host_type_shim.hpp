// Copyright (c) 2026 Huawei Technologies Co., Ltd.
// This program is free software, you can redistribute it and/or modify it under the terms and conditions of
// CANN Open Software License Agreement Version 2.0 (the "License").
// Please refer to the License for details. You may not use this file except in compliance with the License.
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
// See LICENSE in the root of the software repository for the full text of the License.

#ifndef PTO_TEST_LINX_HOST_TYPE_SHIM_HPP
#define PTO_TEST_LINX_HOST_TYPE_SHIM_HPP

#define PTO_LINX_COMPAT_TYPES_PROVIDED 1
#define PTO_LINX_HOST_CXX 1

inline unsigned __builtin_linx_get_thread_idx() { return 0; }

// The production Linx compiler provides these scalar types and the tile_size
// type modifier.  Define layout-compatible host substitutes so a normal C++
// compiler can validate the public template surface without pretending to
// validate Linx code generation.
using __fp32 = float;
struct __tf32 { unsigned value; };
struct __hf32 { unsigned value; };
using __half = _Float16;
struct __blkc_bf16 { unsigned short value; };
struct __hif8 { unsigned char value; };
struct __fp8_e4m3 { unsigned char value; };
struct __fp8_e5m2 { unsigned char value; };
struct __fp8_e6m2 { unsigned char value; };
struct __fp6_e3m2 { unsigned char value; };
struct __fp6_e2m3 { unsigned char value; };
struct __fp4_e2m1x2 { unsigned char value; };
struct __fp4_e1m2x2 { unsigned char value; };
struct __fp8_e8m0 { unsigned char value; };
struct __fp4_hif4x2 { unsigned char value; };
struct __int4x2 { unsigned char value; };
struct __uint4x2 { unsigned char value; };
struct __fp16x2 { unsigned value; };
struct __bf16x2 { unsigned value; };
struct __uint16x2 { unsigned value; };
struct __int16x2 { unsigned value; };
struct __fp8_e4m3x4 { unsigned value; };
struct __fp8_e5m2x4 { unsigned value; };
struct __uint8x4 { unsigned value; };
struct __int8x4 { unsigned value; };
struct __fp8_e6m2x2 { unsigned short value; };
struct __fp8_e4m3x2 { unsigned short value; };
struct __fp8_e5m2x2 { unsigned short value; };

#define tile_size(elements) [elements]

#endif

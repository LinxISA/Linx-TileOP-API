// Copyright (c) 2026 Huawei Technologies Co., Ltd.
// This program is free software, you can redistribute it and/or modify it under the terms and conditions of
// CANN Open Software License Agreement Version 2.0 (the "License").
// Please refer to the License for details. You may not use this file except in compliance with the License.
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
// See LICENSE in the root of the software repository for the full text of the License.

#include "common/pto_tile.hpp"

using PTOASVecTile =
    pto::Tile<pto::TileType::Vec, float, 16, 16, pto::BLayout::RowMajor,
              16, 16, pto::SLayout::NoneBox, 512, pto::PadValue::Null,
              pto::CompactMode::Null>;

using PTOASAccTile =
    pto::Tile<pto::TileType::Acc, float, 16, 16, pto::BLayout::RowMajor,
              16, 16, pto::SLayout::NoneBox, 1024, pto::PadValue::Null,
              pto::CompactMode::Null>;

static_assert(PTOASVecTile::Loc == pto::Location::Vec);
static_assert(PTOASAccTile::Loc == pto::Location::Acc);
static_assert(PTOASVecTile::Compact == pto::CompactMode::Null);

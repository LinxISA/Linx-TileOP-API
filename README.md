# Linx TileOP API

Header-only C++ TileOP bindings for the LinxISA / PTO ISA 0.58.7 architectural contract.
The checked-in catalog and documentation are generated against the PTO ISA 0.58.7
baseline (`0.58.7.0`, ABI `pto-isa-0.58.7-mode-function-v1`).

Installed headers expose their provenance through
`<common/pto_tileop_api_revision.hpp>`. `PTO_TILEOP_API_VERSION` and
`PTO_TILEOP_API_SPEC_VERSION` identify the API/spec contract, while
`PTO_TILEOP_API_REVISION` records the exact TileOP-API Git commit used by
`make install`. Feature macros such as `PTO_TILEOP_API_HAS_LOCAL_B_KN_FIX`
allow consumers to reject toolchains that predate a required fix.

The normative operation inventory comes from the checked-in PTO catalog at
[`contracts/pto-isa-0.58.7-tile-operations.json`](contracts/pto-isa-0.58.7-tile-operations.json).
The public bindings retain the unique compiled `BSTART.TEPL` carrier for VEC/SFU
source compatibility and emit the named TLSU/CUBE operation forms accepted by
the current Linx compiler. The generated engine index is a wrapper compatibility
view, not a replacement for the PTO ASL/NDF semantics.

## Documentation

- [Execution engines and operations](docs/tileop-usage/generated/engines.md)
- [TLSU operations](docs/tileop-usage/tlsu/load-store-move/TLOAD.md)
- [CUBE operations](docs/tileop-usage/cube/matrix-matrix/TMATMUL.md)
- [Tile and Shared-register constraints](docs/tileop-usage/concepts/tile-constraints.md)
- [Layout helpers](docs/tileop-usage/layout-and-rearrangement/layout/TCONCAT.md)
- [PTO ISA 0.58.7 CUBE layout operations](docs/tileop-usage/layout-and-rearrangement/layout/TPERMUTE.md)
- [Comparison operations](docs/tileop-usage/elementwise-tile-tile/logical/TCMP.md)
- [TEXPDIF](docs/tileop-usage/elementwise-tile-tile/transcendental/TEXPDIF.md)
- [Sorting operations](docs/tileop-usage/irregular-and-complex/sorting/TSORT.md)
- [Fixed-point matrix wrappers](docs/tileop-usage/options.md)

## Validation

The proposed lowercase scalar `__sync_*` API and MSGB/HSCB protocol are documented
in [the sync proposal](docs/proposals/sync-intrinsics/README.md) and its
[send/receive implementation contract](docs/proposals/sync-intrinsics/msgb-implementation.md).
These declaration-only APIs and the portable transport reference are not installed
or exported by the active TileOP headers. Run `make check-sync-proposal` for their
host checks; target ISA/compiler bindings remain tracked by
[issue #309](https://github.com/LinxISA/Linx-TileOP-API/issues/309).

```sh
make check
```

Target compilation requires a Linx LLVM build supporting the PTO ISA 0.58.7
encoding ABI. The old PTO ISA 0.58.3 Linx LLVM requirement belongs only to the
historical migration fixtures under `test/tileop_api` and
`docs/tileop-usage/migration/pto-0583-migration.md`; it is not the current API
baseline. Run the catalog and documentation checks before target compilation.

To refresh the pinned projection from the clean, exact reviewed LinxISA
authority commit recorded by the generator:

```sh
python3 tools/sync_linxisa_v058_contract.py \
  /path/to/linx-isa/isa/v0.58/linxisa-v0.58.json
python3 tools/generate_engine_docs.py
make check
```

Indexed TLSU 显式字节范围扩展见 [PR #424 使用说明](docs/tileop-usage/indexed-tlsu-range.md)。

`MGATHER_MASK` / `MSCATTER_MASK` 使用每个索引事务一个 U8 字节的 `0/1` 掩码；
RowMajor `TCMP/TCMPS` 的 packed predicate 不能直接传入。
合法 `TLOAD` 示例和有效区域要求见 [MGATHER_MASK](docs/tileop-usage/tlsu/memory/MGATHER_MASK.md)
与 [MSCATTER_MASK](docs/tileop-usage/tlsu/memory/MSCATTER_MASK.md)。
运行 `python3 test/test_indexed_mask.py -v` 检查载体和文档；设置 `LINX_CXX`、
`LINX_SYSROOT` 可增加目标编译检查，再设置 `LINX_MODEL_BIN=/path/to/gfrun`
可验证四个重载的数值结果及非法字节拒绝。主机检查只验证 C++ 契约，不模拟指令。

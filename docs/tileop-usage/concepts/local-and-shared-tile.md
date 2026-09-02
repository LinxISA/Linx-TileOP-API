# Local 与 Shared Tile

Local Tile 使用 `B.IOT`，Shared Tile 使用 `B.IOS`。Shared handle 必须遵守 inline SSA 和
PE participation 规则；任何具体操作的 Shared 支持以该操作页面为准。

## 使用要求

根据操作接口选择正确的 Tile location；不能把 Local Tile 当作 Shared Tile 传入，也不能在不支持 Shared 的操作中自行替换 binder。

## 约束、默认值、异常和边界行为

Local 与 Shared 对 SizeCode、容量、PE mask、handle 生命周期和参与 PE 集合有不同约束。省略的描述字段使用具体操作默认值；location、handle、容量或参与掩码非法时在执行前失败。边界区域、padding 和 fault 行为以对应操作页明确列出的说明为准。

## 使用示例

```cpp
// tileop-doc: fragment -- illustrative call; operand declarations are omitted.
// 具体 Tile 类型必须与操作页面声明的 location 约束一致。
TADD(local_dst, local_lhs, local_rhs);
```

## Local/Shared 的具体约束

- Local Tile 的 allocation mask 必须描述已分配的 Local 寄存器；Shared Tile
  是 core-private 的持久 descriptor-plus-payload 状态。
- Shared Tile 的 `initialized_mask` 记录已写入的固定 offset quarter；只有
  `initialized_mask == allocation_mask` 且 payload 已定义时，Shared Tile 才完全初始化。
- Cooperative matrix Shared Tile 必须使用 allocation mask `1111`、initialized
  mask `1111`、已发布且内容已定义的 descriptor。
- Shared Tile 的 descriptor 必须已分配，容量在 `128 B..256 KiB` 范围内，shape
  不超过容量，valid rows/columns 不超过物理 rows/columns，且禁止通用索引访问
  CUBE layout。非 CUBE 的兼容 descriptor 还必须匹配容量、shape、dtype 和 layout。
- 非零 PE mask 的部分更新先检查 descriptor 兼容性，再复制选定 quarter；完整记录
  赋值是架构提交点。`PE_MASK=0000` 是不改变状态的严格空操作。

## 生命周期和移动接口

- `SharedTile` 是架构 Shared descriptor handle，不具有普通 C++ 跨函数值 ABI。
  应在创建它的函数内消费，或只通过 `PTO_SHARED_INLINE` / `always_inline` wrapper
  传递；不要通过非内联函数的参数、返回值、普通对象存储或 spill 保留 handle。
- Local-to-Shared 使用 `TMOV_L2S_INSERT` 或 `TMOV_L2S_PUBLISH`；Shared-to-Local
  使用 `TMOV_S2L_BROADCAST` 或 `TMOV_S2L_EXTRACT`。普通 `TMOV(dst, src)` 仅是
  同一 Local Tile 类型之间的复制，不是 Local/Shared 转换。
- 输出参数形式会同步 Local Tile 的 runtime valid rows/columns 到 Shared descriptor。
  后续 `TSTORE`、`TSTORE_PART` 或 S2L 操作读取的是该 Shared valid metadata。
- Shared full store 使用 `TSTORE(gm, shared)` 和固定 mask `1111`；部分 store 使用
  `TSTORE_PART<PEMask>(gm, shared)`，且 `PEMask` 必须是公开接口接受的非零集合。

### C++ 接口

以下是 Local/Shared 移动接口的完整公开形式。`PEMask` 是编译期 PE
participation mask，默认值为 `15`（`1111`）；可用的非零值为
`1, 2, 4, 8, 12, 14, 15`。

```cpp
template <int PEMask = 15, is_tile_data_v LocalTile>
PTO_SHARED_INLINE void TMOV_L2S_INSERT(
    SharedTile<LocalTile> &dst, const LocalTile &src);
template <int PEMask = 15, is_tile_data_v LocalTile>
PTO_SHARED_INLINE SharedTile<LocalTile>
TMOV_L2S_INSERT(const LocalTile &src);

template <int PEMask = 15, is_tile_data_v LocalTile>
PTO_SHARED_INLINE void TMOV_L2S_PUBLISH(
    SharedTile<LocalTile> &dst, const LocalTile &src);
template <int PEMask = 15, is_tile_data_v LocalTile>
PTO_SHARED_INLINE SharedTile<LocalTile>
TMOV_L2S_PUBLISH(const LocalTile &src);

template <int PEMask = 15, is_tile_data_v LocalTile,
          is_tile_data_v LocalDst>
PTO_SHARED_INLINE void TMOV_S2L_BROADCAST(
    LocalDst &dst, const SharedTile<LocalTile> &src);
template <int PEMask = 15, is_tile_data_v LocalTile,
          is_tile_data_v LocalDst>
PTO_SHARED_INLINE void TMOV_S2L_EXTRACT(
    LocalDst &dst, const SharedTile<LocalTile> &src);
```

`INSERT`/`PUBLISH` 的返回值形式最适合普通局部使用；输出参数形式适合已有
`SharedTile` carrier。两种 S2L 形式都要求目标是普通 Local Tile，并且 Shared
handle 只能在当前函数或强制 inline 的调用链中消费。

### 最小构造与往返示例

Shared Tile 不是通过 `Tile<Location::Shared, ...>` 声明，而是用普通 Local
Tile 类型包装：

```cpp
using Local = Tile<Location::Vec, float, 8, 256, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<8, 256>>;

void shared_roundtrip(float *out, float *in) {
  GM dst(out), src_gm(in);
  Local src, restored;
  TLOAD(src, src_gm);

  auto shared = TMOV_L2S_INSERT(src);       // SharedTile<Local>
  TMOV_S2L_BROADCAST(restored, shared);
  TSTORE(dst, shared);                      // full mask 1111
}
```

需要只让部分 PE 写回 GM 时，将最后一行替换为
`TSTORE_PART<12>(dst, shared)` 等合法 mask 形式。不要把 `shared` 保存到
普通对象、通过非内联函数传递，或把它作为普通 Local Tile 传给 `TMOV`。

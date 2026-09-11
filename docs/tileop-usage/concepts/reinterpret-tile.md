# reinterpret_tile：零指令数据类型重解释

`reinterpret_tile<NewDType>(src)` 在**不生成任何指令**的情况下，把一个
Local Tile 的静态 `DType` 重解释为另一个**等位宽**的数据类型。底层 Tile
寄存器的存储位模式、物理 shape、valid region、layout 与 location 完全
不变；只有后续操作看到的 dtype（及其 ISA `DataType` 编码）改变。

它是一个**视图（view）**，不是转换：不分配新 Tile、不复制 payload、
不发射 `TCVT`。

## C++ 接口

```cpp
template <typename NewDType, is_tile_data_v SourceTile>
inline auto reinterpret_tile(SourceTile &Source);
```

返回 `ReinterpretedTileView<NewDType, SourceTile>`：持有源 Tile 的引用，
`data()` 直接转发源寄存器载体，shape/layout/valid 等 static 成员与源
一致，仅 `DType` 替换为 `NewDType`。

## 约束

- **等元素位宽**：`type_traits<NewDType>::bits ==
  type_traits<SourceTile::DType>::bits`。重解释不得改变逻辑元素个数、
  物理字节数或 `TileSizeCode`；
- **仅 Local Tile**：第一阶段不支持 Shared（Shared 需要 `Sr` binder 的
  独立 view 合同）；
- **NewDType 必须有 PTO TypeCode**：无编码的类型被拒绝；
- **打包容器不参与**：`FP4X2/S4X2/U4X2` 等 4-bit 打包类型不作为重解释
  目标（见 TCMP 页对 backing/操作类型分离的说明）；
- **视图不可悬空**：入参是非 const 左值引用，绑定临时对象会被拒绝。

违反上述约束在编译期由 `static_assert` 拒绝，错误信息指出具体原因。

## 与 TCVT 的区别

| | `reinterpret_tile` | `TCVT` |
| --- | --- | --- |
| 生成指令 | 零指令（纯编译期视图） | `BSTART.TEPL TCVT`（真实转换） |
| 位模式 | 不变 | 按数值语义转换（可能舍入/饱和） |
| dtype 关系 | 必须等位宽 | 按转换表（如 BF16→FP32 位宽可变） |
| 目的 | 让后续操作以另一种 dtype **解释**同一存储 | 计算出另一种 dtype 的**新值** |

## 使用示例

`reinterpret_tile` 返回的视图必须先存为**具名左值**再传给操作：操作接口
的 Tile 形参是非 const 左值引用，内联传递临时对象（`op(dst, a,
reinterpret_tile<T>(b))`）会被拒绝。

```cpp
// FP32 操作数以 U32 tile 承载：源 backing 类型与操作类型分离。
using F32T = Tile<Location::Vec, float, 4, 16, BLayout::RowMajor>;
using U32T = Tile<Location::Vec, uint32_t, 4, 16, BLayout::RowMajor>;
using OutT = Tile<Location::Vec, int32_t, 4, 16, BLayout::RowMajor>;

void carrier_view(F32T &f, U32T &u, OutT &out) {
  // u 的存储按 FP32 解释参与比较；不发生任何数据搬运。
  // TCMP 的两个源是独立模板参数，允许 Tile 与 view 混用。
  auto u_as_f32 = reinterpret_tile<float>(u);
  TCMP<CmpMode::LT>(out, f, u_as_f32);   // -> BSTART.TEPL TCMP, FP32
}

// 位模式检查/转换场景：以 U16 视读 BF16 存储位再转换。
using BF16T = Tile<Location::Vec, __bf16, 4, 16, BLayout::RowMajor>;
using F32Out = Tile<Location::Vec, float, 4, 16, BLayout::RowMajor>;

void bits_view(BF16T &b, F32Out &out) {
  auto b_as_u16 = reinterpret_tile<uint16_t>(b);
  TCVT(out, b_as_u16);                   // -> BSTART.TEPL TCVT, U16
}
```

注意 TEPL 的 tile/tile 操作（`TADD`/`TAND` 等）是单类型模板：三个操作数
必须是同一 C++ 类型。`reinterpret_tile` 的 view 与普通 `Tile` 混用这类
接口时模板推导会失败；混型场景请使用源类型独立的接口（`TCMP` 的双源、
`TCVT` 的 dst/src），或对全部操作数统一建立 view。

## 常见误用

```cpp
reinterpret_tile<uint16_t>(f32_tile);   // ❌ 位宽不等（32→16）
reinterpret_tile<float>(shared_tile);   // ❌ 第一阶段仅 Local
reinterpret_tile<int>(f32_tile);        // ✅ 位宽相等，合法
reinterpret_tile<float>(F32T{});        // ❌ 绑定临时对象，编译拒绝
```

注意与 `range::subview`/`range::assemble` carrier 的关系：
`reinterpret_tile` 作用于 Tile 本体，range carrier 不在其支持范围内。

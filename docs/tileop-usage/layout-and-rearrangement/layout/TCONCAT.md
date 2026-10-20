# TCONCAT（仅历史参考）

`TCONCAT` 已从 PTO ISA 0.59.0 active Tile catalog 删除。旧 TEPL selector 96
不得由当前 wrapper、associated wrapper 或手写兼容别名发出。

## 当前 API 行为

为给旧源码提供稳定、明确的迁移诊断，本仓库保留以下函数名：

```cpp
TCONCAT(dst, src0, src1);
TCONCAT_ASS(assembled_dst, src0, src1);
```

两种调用都会在模板实例化时触发编译错误；它们不是可执行 wrapper，也没有
PTO ISA 0.59.0 bundle 组成。`TCONCAT_ASS` 不得借 associated 路径重新启用
selector 96。

需要拼接数据时，应使用当前 golden 中已定义的操作或显式的数据搬移序列，
并按所选替代方案的 ASL/NDF 合法性、布局和生命周期约束重新验证。

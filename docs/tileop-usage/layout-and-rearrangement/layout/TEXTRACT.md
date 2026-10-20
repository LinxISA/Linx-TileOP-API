# TEXTRACT（仅历史参考）

`TEXTRACT` 已从 PTO ISA 0.59.0 active Tile catalog 删除。旧 SFU/TEPL
selector 98 不得由当前 wrapper 或手写别名发出。

本仓库只保留 `TEXTRACT(...)` 名称用于模板实例化时给出稳定的编译错误；它不是
可执行 wrapper，也没有 PTO ISA 0.59.0 bundle 组成。需要子区域访问时请使用当前
golden 定义的 `B.SUBVIEW`/region API，并重新验证布局、范围和生命周期约束。

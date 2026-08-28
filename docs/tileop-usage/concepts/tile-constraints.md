# Tile 约束

所有接口都必须满足 dtype、shape、valid region、layout、capacity、location 和 PE mask 约束。
具体操作的约束以对应页面、`template_asm.hpp` 和 PTO-SPEC contract 为准。

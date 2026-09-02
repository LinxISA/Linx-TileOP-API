#!/usr/bin/env python3
"""Generate self-contained developer-oriented TileOP pages from the API and standards sources."""
from __future__ import annotations
import json
import re
import textwrap
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC_ROOT = ROOT.parent / "pto-spec"
if not (SPEC_ROOT / "spec/catalog/tile-operations.json").exists():
    SPEC_ROOT = ROOT.parent.parent / "pto-spec"
CATALOG = SPEC_ROOT / "spec/catalog/tile-operations.json"
HEADER = ROOT / "include/jcore/template_asm.hpp"
DOC_ROOT = ROOT / "docs/tileop-usage"
SPEC_DOC_ROOT = SPEC_ROOT / "docs/tile"
ASL_ROOT = SPEC_ROOT / "asl/tile"
PTO_SPEC_REF = "v0.58.4.1"

# Names used by the TileDataType encoding.  Keeping this list here makes the
# documentation generator independent of prose wording in individual SPEC
# pages while still deriving the operation-specific subset from those pages.
DATA_TYPES = (
    "FP64", "FP32", "TF32", "HF32", "FP16", "BF16", "HiF8",
    "E4M3", "E5M2", "E3M2", "E2M3", "E8M0",
    "E2M1X2", "E1M2X2", "HiF4X2", "S4X2", "U4X2",
    "S64", "S32", "S16", "S8", "U64", "U32", "U16", "U8",
)

def section(text: str, title: str) -> str:
    m = re.search(rf"^## {re.escape(title)}\s*$", text, re.M)
    if not m: return ""
    rest = text[m.end():]
    n = re.search(r"^## ", rest, re.M)
    return rest[:n.start() if n else len(rest)].strip()

def clean(text: str) -> str:
    text = re.sub(r"<!--.*?-->", "", text, flags=re.S)
    text = re.sub(r"`([^`]+)`", r"\1", text)
    return re.sub(r"\s+", " ", text).strip()

def translate_default_rules(text: str) -> str:
    """将规范中的编码默认值归纳为少量中文开发说明。"""
    rules = []

    if "AType as BType" in text:
        rules.append("数据类型编码始终使用 `AType`；省略 `B.DATR` 时，`BType` 沿用 `AType`，舍入模式使用 `RNE`，并关闭饱和处理。")
    elif "PadValue=Null" in text or "PadValue is Null" in text:
        suffix = "，布局使用 `NORM`" if "Layout=NORM" in text or "selects NORM" in text else ""
        rules.append(f"省略 `B.DATR` 时，padding 值使用 `Null`{suffix}。")
    elif "Omitted B.DATR selects NORM" in text:
        rules.append("省略 `B.DATR` 时使用 `NORM` 布局。")
    elif "Omitted B.DATR" in text:
        rules.append("省略 `B.DATR` 时使用该操作规定的默认编码；若显式提供该描述符，未使用的字段必须保持为零。")

    if "LB0" in text or "LB1" in text or "LB2" in text:
        if re.search(r"default M, N, and K|default M, N, 和 K|defaults Local M", text):
            detail = "`M`、`N`、`K` 分别默认为 1"
            if "TGEMV fixes M to one" in text:
                detail += "，且 `TGEMV` 固定要求 `M=1`"
            rules.append(f"省略 `LB0`、`LB1`、`LB2` 时，{detail}；显式给出的维度必须为正值。")
        elif "ValidCol" in text:
            rules.append("`LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。")
        else:
            rules.append("省略维度描述符时使用本操作规定的维度默认值；显式给出的维度必须满足本操作的非零约束。")

    if "B.FPATR" in text:
        rules.append("全零 `B.FPATR` 表示不启用转换、激活和归约；只有后处理模式需要时才提供 `B.IOR` 或辅助 `B.IOT` 操作数。")
    elif "Omitted B.IOR" in text:
        rules.append("省略 `B.IOR` 时使用本操作规定的寄存器或控制默认值；显式编码为零表示实际的零值，不等同于省略该描述符。")
    elif "B.IOR" in text and ("required" in text or "mandatory" in text):
        rules.append("`B.IOR` 是必需描述符；未使用的选择器和字段必须编码为零。")

    if "PadValue 00" in text or "explicit 00, 01, 10" in text:
        rules.append("显式 padding 编码 `00`、`01`、`10`、`11` 分别选择 `Zero`、`Max`、`Min`、`Null`。")
    elif "padding is always Null" in text or "padding is Null" in text:
        rules.append("物理 padding 始终使用 `Null`。")

    if "TransA=0" in text and "TransB=0" in text:
        rules.append("`TransA=0`、`TransB=0` 表示不转置；非零转置控制仅可用于规范允许的 `Shared` 主操作数。")

    if not rules:
        rules.append("省略可选编码字段时使用该操作规定的默认值；显式编码为零是一个实际取值，不应默认视为省略字段。")
    return "\n".join(f"- {rule}" for rule in rules)

def supported_types(text: str, name: str) -> str:
    """Return only dtype claims that are directly stated by the SPEC contract.

    In particular, do not collect every dtype mentioned in a page: encoding
    tables, examples, and rejection diagnostics commonly mention types that
    are not legal for the operation.
    """
    packed = ("E2M1X2", "E1M2X2", "HiF4X2", "S4X2", "U4X2")
    assigned = list(DATA_TYPES)

    if re.search(r"Index Tiles use [`]?S32[`]?, [`]?U32[`]?, [`]?S64[`]?, or [`]?U64", text):
        transfer = [dtype for dtype in DATA_TYPES if dtype not in packed]
        return ("支持索引 Tile 类型 S32、U32、S64、U64；支持传输数据 Tile 类型 "
                + "、".join(transfer)
                + "。")

    # The ASL model is the normative source for legality.  In particular, a
    # number of operation pages only say "unsupported DataType" while their
    # InstructionContract function delegates to a shared legality helper.
    # Resolve that helper here instead of treating those pages as unspecified.
    asl_types = asl_operation_types(name)
    if asl_types:
        source, destination = asl_types
        if source is not None or destination is not None:
            if source is not None and destination is not None and source != destination:
                return ("支持源 Tile 类型 " + "、".join(source) +
                        "；支持目标 Tile 类型 " + "、".join(destination) + "。")
            values = source if source is not None else destination
            if values:
                return "支持" + "、".join(values) + "类型。"
    if (re.search(r"(?:accepts|accept) every assigned Tile DataType", text, re.I)
            or re.search(r"DataType accepts 0\.\.14, 16\.\.20, and 24\.\.28", text, re.I)
            or re.search(r"Every assigned type is accepted", text, re.I)):
        return "支持" + "、".join(assigned) + "类型。"

    def types_in(sentence: str) -> list[str]:
        return [dtype for dtype in DATA_TYPES
                if re.search(rf"\b{re.escape(dtype)}\b", sentence)]

    # These are the closed-set forms used by the normative reader blocks and
    # legality bullets.  Keep role labels when the contract distinguishes
    # source, destination, or BSTART/attribute types.
    accepted = re.findall(r"The accepted data-type set is (.*?\.)", text, re.I)
    if accepted:
        found = types_in(accepted[0])
        return "支持" + "、".join(found) + "类型。"

    # Conversion contracts often state the pair in one sentence rather than
    # using the generic source/destination wording below.
    pair = re.search(
        r"BSTART DataType is exactly (.*?) and .*?destination DataType (?:is|exactly) (.*?\.)",
        text, re.I,
    )
    if pair:
        source = types_in(pair.group(1))
        destination = types_in(pair.group(2))
        if source or destination:
            parts = []
            if source:
                parts.append("支持源 Tile 类型 " + "、".join(source))
            if destination:
                parts.append("支持目标 Tile 类型 " + "、".join(destination))
            return "；".join(parts) + "。"

    role_patterns = (
        ("BSTART", r"BSTART DataType is exactly (.*?\.)"),
        ("源 Tile", r"The source DataType is exactly (.*?\.)"),
        ("目标 Tile", r"destination DataType (?:is exactly|is) (.*?\.)"),
        ("选定", r"The selected DataType is exactly (.*?\.)"),
    )
    role_values = []
    for label, pattern in role_patterns:
        match = re.search(pattern, text, re.I)
        if match:
            found = types_in(match.group(1))
            if found:
                role_values.append("支持" + label + "类型 " + "、".join(found))
    if role_values:
        return "；".join(role_values) + "。"

    # No closed set or explicit all-assigned rule was found.  A generic
    # statement is safer than inferring a set from incidental examples or
    # shared model references.
    if not text:
        return "支持的 Tile 数据类型由该操作的 C++ 模板约束和本页约束共同限定。"
    return "支持的 Tile 数据类型由该操作的 C++ 模板约束和本页约束共同限定。"


def _asl_functions() -> dict[str, str]:
    """Collect ASL function bodies, including shared tile legality helpers."""
    functions = {}
    if not ASL_ROOT.exists():
        return functions
    pattern = re.compile(
        r"(?:pure|readonly)?\s*func\s+([A-Za-z_]\w*)\s*\([^)]*\)"
        r"(?:\s*=>[^\n]+)?\s*\nbegin\n(.*?)\nend;", re.S)
    for path in ASL_ROOT.rglob("*.asl"):
        for match in pattern.finditer(path.read_text(encoding="utf-8")):
            functions[match.group(1)] = match.group(2)
    return functions


ASL_FUNCTIONS = _asl_functions()


def _asl_type_set(expression: str, parameter: str = "data_type",
                  operation: str = "") -> set[str]:
    """Evaluate the finite TileDataType predicates used by the ASL model."""
    expression = re.sub(r"//.*", "", expression)
    if expression.strip() == "return TileTeplRawCarrierTypeSupported(data_type);":
        return _asl_type_set(ASL_FUNCTIONS["TileTeplRawCarrierTypeSupported"])
    calls = re.findall(r"([A-Za-z_]\w*DataType\w*)\s*\(", expression)
    for called in reversed(calls):
        if called in ASL_FUNCTIONS and called != "TileDataTypeEncodingValid":
            return _asl_type_set(ASL_FUNCTIONS[called], parameter, operation)
    # Encoding validity only rejects reserved encodings; the following
    # operation-specific predicate supplies the actual supported set.
    if "TileDataTypeEncodingValid" in expression:
        if "TileRegularTLSUDataTypeSupported" in expression:
            return _asl_type_set(ASL_FUNCTIONS["TileRegularTLSUDataTypeSupported"])
        if "TileCarrierOrPackedBaselineDataTypeSupported" in expression:
            return _asl_type_set(ASL_FUNCTIONS["TileCarrierOrPackedBaselineDataTypeSupported"])
    if "TileDataType !=" in expression or "data_type !=" in expression:
        excluded = set(re.findall(r"TileDataType_([A-Za-z0-9]+)", expression))
        return {x for x in DATA_TYPES if x not in excluded}
    direct = set(re.findall(r"TileDataType_([A-Za-z0-9]+)", expression))
    if direct:
        return {x for x in DATA_TYPES if x in direct}
    if re.fullmatch(r"\s*(?://[^\n]*\n\s*)*return\s+TRUE\s*;\s*", expression):
        return set(DATA_TYPES)
    # Helpers expressed by width/classes rather than enum constants.
    if "!TileDataTypeIsFourBit" in expression and "TileElementBytes" in expression:
        return {"FP32", "TF32", "HF32", "FP16", "BF16", "HiF8", "E4M3",
                "E5M2", "E3M2", "E2M3", "E8M0", "S32", "S16", "S8",
                "U32", "U16", "U8"}
    if "TileCarrierOnlyDataTypeSupported" in expression and "TileDataTypeIsFourBit" in expression:
        return _asl_type_set(ASL_FUNCTIONS["TileCarrierOnlyDataTypeSupported"]) | set(
            ("E2M1X2", "E1M2X2", "HiF4X2", "S4X2", "U4X2"))
    helper = re.search(r"return\s+([A-Za-z_]\w*DataType\w*)\s*\(", expression)
    if helper and helper.group(1) in ASL_FUNCTIONS:
        body = ASL_FUNCTIONS[helper.group(1)]
        # Predicates with an operation argument select a shared family.
        if "TileBinaryDataTypeSupported" in body and operation in {"AND", "OR", "XOR", "SHL", "SHR"}:
            return _asl_type_set(ASL_FUNCTIONS.get("TileVecScalarIntegerDataTypeSupported", ""))
        return _asl_type_set(body, parameter, operation)
    # Shared helpers may be called without the DataType suffix in a wrapper.
    for called in re.findall(r"([A-Za-z_]\w*DataType\w*)\s*\(", expression):
        if called in ASL_FUNCTIONS:
            return _asl_type_set(ASL_FUNCTIONS[called], parameter, operation)
    return set()


def asl_operation_types(name: str) -> tuple[list[str] | None, list[str] | None] | None:
    """Return (source types, destination types), or None when ASL has no set."""
    packed = ("E2M1X2", "E1M2X2", "HiF4X2", "S4X2", "U4X2")
    # Prefetch validates an encoding selector but has no Tile data operand.
    if name == "TPREFETCH":
        return None
    paths = list(ASL_ROOT.rglob(f"{name}.asl"))
    if not paths:
        return None
    body = paths[0].read_text(encoding="utf-8")
    matches = re.finditer(
        rf"InstructionContract(?:Source|Destination)?DataTypes?Legal_{re.escape(name)}"
        rf".*?\nbegin\n(.*?)\nend;", body, re.S)
    match = next(matches, None)
    if match:
        contract = match.group(1)
        args = re.search(r"\((.*?)\)", body[:match.start()], re.S)
        # Direct two-role contracts use source_type/destination_type.
        if "source_type" in contract and "destination_type" in contract:
            source = set(re.findall(r"source_type\s*==\s*TileDataType_([A-Za-z0-9]+)", contract))
            destination = set(re.findall(r"destination_type\s*==\s*TileDataType_([A-Za-z0-9]+)", contract))
            if source and destination:
                return sorted(source, key=DATA_TYPES.index), sorted(destination, key=DATA_TYPES.index)
        operation = next((x for x in ("AND", "OR", "XOR", "SHL", "SHR") if x in contract), "")
        values = _asl_type_set(contract, operation=operation)
        if values:
            return sorted(values, key=DATA_TYPES.index), None

    # Matrix operations delegate through TileOrdinaryMatrixInputTypeSupported.
    if name.startswith(("TMATMUL", "TGEMV")):
        helper = ASL_FUNCTIONS.get("TileOrdinaryMatrixInputTypeSupported", "")
        values = _asl_type_set(helper)
        return sorted(values, key=DATA_TYPES.index), None
    if name == "TMOV":
        return list(DATA_TYPES), None
    if name in {"TLOAD", "TSTORE"}:
        helper = ASL_FUNCTIONS.get({
            "TLOAD": "TileRegularTLSUDataTypeSupported",
            "TSTORE": "TileRegularTLSUDataTypeSupported",
        }[name], "")
        values = _asl_type_set(helper)
        if values:
            return sorted(values, key=DATA_TYPES.index), None
    if name in {"TGATHER", "TSCATTER", "MGATHER", "MGATHER_MASK", "MGATHER_CAS",
                "MSCATTER", "MSCATTER_MASK"}:
        return [x for x in DATA_TYPES if x not in packed], None
    return None

PARAMETER_MEANINGS = {
    "dst": "输出 Tile；成功调用后写入操作结果。",
    "d": "输出 Tile；成功调用后写入操作结果。",
    "c": "累加器或输出 Tile，具体角色由接口名称和重载决定。",
    "a": "左操作数或输入 Tile。",
    "b": "右操作数或输入 Tile。",
    "src": "输入 Tile 或源数据。",
    "source": "输入 Tile 或源数据。",
    "src0": "第一个输入 Tile。",
    "src1": "第二个输入 Tile。",
    "src2": "第三个输入 Tile。",
    "left": "左输入 Tile。",
    "right": "右输入 Tile。",
    "bias": "偏置 Tile，用于需要偏置的重载。",
    "scale_a": "A 操作数的缩放 Tile。",
    "scale_b": "B 操作数的缩放 Tile。",
    "ascale": "A 操作数的缩放 Tile。",
    "bscale": "B 操作数的缩放 Tile。",
    "scale_mtx": "矩阵缩放 Tile。",
    "scale_vec": "向量缩放 Tile。",
    "options": "`fixp::Options` 选项对象；携带量化、激活、转置、缩放以及可选辅助输出配置。",
    "base": "GM 基地址。",
    "validCol": "有效区域的列数。",
    "validRow": "有效区域的行数，省略时使用接口/规范默认值。",
    "byteDisplacements": "以字节为单位的地址位移索引 Tile。",
    "expected": "比较交换操作的期望值 Tile。",
    "replacement": "比较成功时写入 GM 的替换值 Tile。",
    "observedOld": "保存每个位置观察到的旧值的输出 Tile。",
    "multiplier": "量化或反量化使用的乘数。",
    "zeroPoint": "量化使用的零点。",
    "index": "索引 Tile。",
    "valueDst": "排序后的值输出 Tile；shape 与 source 相同。",
    "indexDst": "U32 索引输出 Tile；每个元素保存其在原行组内的原始索引。",
    "sortWidth": "每个独立行组的宽度；默认 32，必须满足该操作规定的分组宽度和源 Tile 列数约束。",
    "descending": "排序方向；`false`（默认）为升序，`true` 为降序；稳定性不因方向改变。",
    "indexRow": "行索引 Tile；其元素选择对应的源或目标行。",
    "indexCol": "列索引 Tile；其元素选择对应的源或目标列。",
    "mask": "逐元素掩码 Tile；仅掩码允许的位置参与该重载定义的读写。",
    "offset": "以元素或字节计的偏移；具体单位由该重载和本页约束定义。",
    "off": "编译期或运行期偏移参数；单位和取值范围由该重载定义。",
    "posM": "矩阵 M 方向的位置或偏移描述符。",
    "posK": "矩阵 K 方向的位置或偏移描述符。",
    "peer_tid": "目标 peer 的 Tile/线程标识；必须属于当前操作允许的 peer 集合。",
    "ByteId": "按字节寻址的标识或偏移；不得与元素索引混用。",
    "Idx": "索引 Tile 或索引描述符；其 dtype 和范围由对应重载约束。",
    "mtx": "矩阵 Tile 操作数。",
    "vec": "向量 Tile 操作数。",
    "s": "标量或标量 Tile 操作数；具体载体由重载决定。",
    "sa": "A 操作数对应的缩放或标量 Tile。",
    "sb": "B 操作数对应的缩放或标量 Tile。",
    "smtx": "矩阵操作数对应的缩放 Tile。",
    "svec": "向量操作数对应的缩放 Tile。",
    "valid_col": "运行时有效列数；不得超过 Tile 的物理列数。",
    "valid_row": "运行时有效行数；不得超过 Tile 的物理行数。",
    "tile_role_v<Mtx>": "由矩阵 Tile 角色约束推导的操作数；必须匹配该重载要求的矩阵布局和位置。",
}

def operation_contract(name: str) -> str:
    """Return the developer-relevant contract that is common to an operation family."""
    if name in {"TLOAD", "TSTORE"}:
        return ("`TLOAD/TSTORE` 的 GM 行 stride 以**字节**计，而不是元素数；普通 Tile 与 "
                "CUBE Tile 分别走普通传输和布局转换路径。source/destination 的顺序不可互换。")
    if name.startswith(("TMATMUL", "TGEMV")):
        if name.startswith("TMATMUL_MX"):
            return ("矩阵 MX 的 A/B 主输入分别独立选择 scale contract；普通 MX 类型使用 "
                    "E8M0/group-32，而 HiF4X2 只能用于 Matrix-MX，并使用 `uint32_t`/group-64。"
                    "ScaleA/ScaleB 的 shape、layout 和 storage 必须分别跟随 A/B 主输入。")
        return ("矩阵维度必须满足乘法关系（`M×K` 与 `K×N`，或对应 GEMV 形式）；A/B/D 的 "
                "CUBE layout、累加器类型和任何 scale/bias/options 必须构成该重载允许的组合。")
    if name in {"TSORT", "TMRGSORT"}:
        return ("排序输出值与输入的有效区域对应；索引输出使用 U32 的组内原始位置。排序宽度、行组形状和 "
                "输入的已排序前提（仅 merge sort）必须满足该操作定义。")
    if name.startswith(("TROW", "TCOL")):
        axis = "行" if name.startswith("TROW") else "列"
        return (f"该操作沿{axis}方向归约或广播；输入、输出及广播 Tile 的有效区域和 shape 必须满足 "
                "该方向的对应关系，且索引输出的整数类型必须符合本页约束。")
    if name in {"TGATHER", "TSCATTER", "TPARTADD", "TPARTMUL", "TPARTMAX", "TPARTMIN"}:
        return ("索引/地址 Tile 的 dtype、单位和取值范围必须有效；重复目标、越界、alias 与写入顺序的语义 "
                "由该操作的约束和边界行为定义，不能由普通逐元素操作的直觉推断。")
    if name.startswith(("MGATHER", "MSCATTER", "GMOV", "TPREFETCH")):
        return ("内存地址、byte displacement、mask 和 PE 参与集合必须符合 TLSU contract；地址单位和 "
                "fault 行为见本页的异常和边界行为说明。")
    if name.startswith("T"):
        return ("输入与输出的 Tile location、layout、dtype、物理 shape 和 valid region 必须满足该操作的 "
                "逐项规则；除非本页明确允许，不应假定可原地执行或允许 alias。")
    return "操作数角色、类型和形状必须满足本页的约束。"

def parameter_name(param: str) -> str:
    param = param.split("=")[0].strip()
    match = re.search(r"([A-Za-z_]\w*)\s*(?:\]|$)", param)
    return match.group(1) if match else param

def parameter_table(sigs: list[str]) -> str:
    rows = []
    seen = set()
    for signature in sigs:
        params = signature[signature.find("(") + 1:signature.rfind(")")]
        for param in split_top_level(params):
            name = parameter_name(param)
            if not name or name in seen:
                continue
            seen.add(name)
            meaning = PARAMETER_MEANINGS.get(name, "接口声明的输入参数；其类型、取值范围和作用由该重载及本页约束定义。")
            rows.append(f"| `{name}` | {meaning} |")
    rows.append("| `options` / `fixp::Options` | 选项对象不是普通占位参数：其模板属性 `FixpAttr` 决定 PreQuant、激活、转置、RowMax、GroupMax、MaxAbs 和 CScale 等模式；对象成员用于传递量化、PReLU、行最大值、组最大值及 CScale Tile。未启用的可选成员必须保持为空，启用的模式必须提供对应 Tile 或标量描述符。 |") if "options" not in seen and any("Options" in s for s in sigs) else None
    return "| 参数 | 说明 |\n| --- | --- |\n" + "\n".join(rows)

def overload_selection(sigs: list[str]) -> str:
    """Explain why near-identical overloads coexist instead of making readers diff signatures."""
    if len(sigs) < 2:
        return ""
    has_options = any("Options" in signature for signature in sigs)
    if has_options:
        return ("### 重载选择\n\n"
                "- **基础重载**：不传 `options`，使用该操作的默认后处理属性。\n"
                "- **带 `Options` 的重载**：需要量化、激活、转置、scale 或辅助输出时传入 `options`。"
                "它不是重复声明，而是在相同核心操作数上增加显式属性；仅可启用本操作支持的属性。"
                "详见 [fixp::Options 指南](../../options.md)。\n")
    return ("### 重载选择\n\n"
            "这些重载覆盖不同的 Tile location、返回方式或可选操作数。优先选择参数最少且能表达"
            "当前数据流的形式；不要通过传入无意义的零值来模拟另一个重载。\n")

def dtype_roles(name: str, types: str) -> str:
    if name.startswith(("TMATMUL", "TGEMV")):
        if name.startswith("TMATMUL_MX"):
            return ("| 操作数角色 | 类型要求 |\n| --- | --- |\n"
                    f"| A / B 主输入 | {types}；HiF4X2 仅允许 Matrix-MX。 |\n"
                    "| ScaleA / ScaleB | 与对应主输入独立校验；HiF4X2 为 `uint32_t`/group-64，"
                    "其他 scaled MX 为 E8M0/group-32，storage 必须分别匹配 A/B。 |\n"
                    "| C / D（累加器或结果） | Matrix-MX 默认使用 FP32 累加器。 |\n"
                    "| Bias / 辅助输出 | 必须使用该重载规定的 dtype、shape 与 layout。 |")
        return ("| 操作数角色 | 类型要求 |\n| --- | --- |\n"
                f"| A / B 主输入 | {types} A/B 必须属于该矩阵重载允许的数值类。 |\n"
                "| C / D（累加器或结果） | 类型由该数值类和重载确定；不能仅因列在支持列表中就任意组合。 |\n"
                "| Bias / Scale / 辅助输出 | 必须使用该重载规定的 dtype、shape 与 layout。 |")
    if name in {"TSORT", "TMRGSORT"}:
        return ("| 操作数角色 | 类型要求 |\n| --- | --- |\n"
                f"| 值输入 / 值输出 | {types} |\n"
                "| 索引输出 | `U32`，保存组内原始位置。 |")
    if name in {"TGATHER", "TSCATTER", "MGATHER", "MGATHER_MASK", "MGATHER_CAS", "MSCATTER", "MSCATTER_MASK"}:
        return ("| 操作数角色 | 类型要求 |\n| --- | --- |\n"
                f"| 数据 Tile | {types} |\n"
                "| 索引 / 地址位移 Tile | 必须使用该操作 contract 允许的整数 dtype 与单位。 |")
    return ""

def valid_region_rules(name: str) -> str:
    if name in {"TLOAD", "TSTORE"}:
        rule = "传输矩形由 Tile 的有效区域和 GM layout 共同限定；不要把物理 padding 当作需要传输的逻辑元素。"
    elif name.startswith(("TMATMUL", "TGEMV")):
        rule = "M/N/K 的有效维度必须与矩阵乘法关系一致；padding 不应被当作数学输入。"
    elif name.startswith(("TROW", "TCOL")):
        rule = "归约轴会收缩，广播轴会扩展；输出 valid region 由该操作的轴规则决定。"
    elif name in {"TTRANS", "TEXTRACT", "TINSERT", "TCONCAT", "TIMG2COL", "TFILLPAD"}:
        rule = "输出 valid region 由布局、偏移或拼接描述符计算；物理 padding 不等于有效数据。"
    elif name in {"TSORT", "TMRGSORT"}:
        rule = "仅在各独立行组的有效范围内排序或合并；索引以该有效行组为基准。"
    else:
        rule = "逐元素操作通常仅对输入和输出共同的有效区域定义结果；未明确规定的 padding 不应读取或依赖。"
    return ("### 有效区域与 padding\n\n"
            "| 项目 | 规则 |\n| --- | --- |\n"
            f"| 有效元素 | {rule} |\n"
            "| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |\n"
            "| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |")

def default_values(sigs: list[str], spec_text: str = "") -> str:
    rows = []
    for signature in sigs:
        params = signature[signature.find("(") + 1:signature.rfind(")")]
        for param in split_top_level(params):
            if "=" not in param:
                continue
            left, value = param.split("=", 1)
            rows.append((parameter_name(left), value.strip()))
    if rows:
        deduped = list(dict.fromkeys(rows))
        table = "\n".join(f"| `{name}` | `{value}` |" for name, value in deduped)
        result = ("以下是 C++ 声明中可直接省略的默认实参：\n\n"
                  "| 参数 | 默认值 |\n| --- | --- |\n" + table)
    else:
        result = "此页面列出的 C++ 形参没有默认实参；不要把省略某个操作数与传入零值视为等价。"

    encoded = section(spec_text, "Defaults and encoded zero")
    if encoded:
        encoded = re.sub(r"<!--.*?-->", "", encoded, flags=re.S).strip()
        encoded = re.sub(r"\bPTO-SPEC\b", "this page", encoded)
        encoded = re.sub(r"\bASL-bound\b", "", encoded)
        result += "\n\n### 编码字段和省略值\n\n" + translate_default_rules(encoded)

    result += "\n\n`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。"
    return result

def tlsu_stride_guide(name: str) -> str:
    if name not in {"TLOAD", "TSTORE"}:
        return ""
    return ("### GM 布局与 stride\n\n"
            "| 场景 | 传入 `global_tensor` 的值 | TLSU 使用的值 |\n"
            "| --- | --- | --- |\n"
            "| 连续 RowMajor | 使用连续构造形式，无需手写 stride | 相邻行的实际字节间距。 |\n"
            "| 带 pitch 的子矩阵 | 构造器仍按**元素 stride**接收行跨度 | wrapper 在 `B.IOR.RegSrc1` 中传递换算后的**字节 stride**。 |\n"
            "| range / subview | base address 与 byte offset 分别传递 | 最终地址为 base 加操作的 range offset。 |\n\n"
            "普通 Tile 使用常规 TLSU 传输；CUBE Tile 由统一 `TLOAD/TSTORE` 自动选择布局转换。"
            "需要在源码中显式表达该边界时，可使用 `TLOAD_CUBE/TSTORE_CUBE`。")

def chinese_intro(name: str, spec_text: str) -> str:
    """Translate the SPEC operation summary without replacing its semantics with boilerplate."""
    direct = {
        "TEXTRACT": "从编码的行、列偏移起始位置，将矩形区域复制到目标 Tile。",
        "TFILLPAD": "复制有效源区域，并将绑定的标量写入目标物理区域的 padding。",
        "TCVT": "将每个有效逻辑元素转换为单独选择的目标类型和布局。",
        "TNOT": "对 Local 整数 Tile 执行按元素宽度的按位取反。",
        "TNEG": "对每个元素执行对应类型的逐元素算术取负。",
        "TFMA": "对三个 Local Tile 源执行对应类型的逐元素融合乘加。",
        "TEXPANDS": "将低位元素宽度的原始标量编码复制到每个有效目标坐标。",
        "THISTOGRAM": "为每个源行构造一个包含 256 个 bin 的 U32 包含式前缀直方图。",
        "TEXTRACT": "从编码的行、列偏移起始位置，将矩形区域复制到目标 Tile。",
        "TFILLPAD": "复制有效源区域，并将绑定的标量写入目标物理区域的 padding。",
        "TNOT": "对 Local 整数 Tile 执行按元素宽度的按位取反。",
        "TNEG": "对每个元素执行对应类型的逐元素算术取负。",
        "TFMA": "对三个 Local Tile 源执行对应类型的逐元素融合乘加。",
        "TCONCAT": "沿列维度连接左、右源 Tile。",
        "TINSERT": "按编码偏移将源 Tile 插入目标 Tile 的旧值快照。",
        "TIMG2COL": "将描述符指定的 feature-map 窗口提取为标准 Left 矩阵顺序。",
        "TMRGSORT": "稳定合并两个已排序的单行 Local 数据流。",
        "TGATHER": "从各自独立选择的源行中收集值。",
        "TSCATTER": "将源值写入由索引 Tile 选择的各个目标行。",
        "TQUANT": "对 Local FP32 Tile 执行仿射量化，生成新的 Local S8 或 U8 Tile。",
        "TDEQUANT": "对每个有效的 S8 或 U8 源元素计算 FP32 结果 `(q - zero_point) * multiplier`。",
        "TCI": "从绑定的起始值构造一行指定类型的序列，并按逻辑列递增或递减。",
        "TTRANS": "将源 Tile 转置后写入目标 Tile。",
        "TROWEXPAND": "将单列广播值逐位复制到每个有效行。",
        "TROWMIN": "将每行归约为该行的最小值。",
        "TROWMAX": "将每行归约为该行的最大值。",
        "TROWSUM": "按顺序将每行归约为该行元素之和。",
        "TROWPROD": "按顺序将每行归约为该行元素之积。",
        "TROWARGMIN": "选择每行最小值对应的最小列索引。",
        "TROWARGMAX": "选择每行最大值对应的最小列索引。",
        "TROWEXPANDMIN": "将每个有效行的元素与该行的单列广播值取最小值。",
        "TROWEXPANDMAX": "将每个有效行的元素与该行的单列广播值取最大值。",
        "TROWEXPANDMUL": "将每个有效行的元素乘以该行的单列广播值。",
        "TROWEXPANDADD": "将每个有效行的元素加上该行的单列广播值。",
        "TROWEXPANDSUB": "从每个有效行的元素中减去该行的单列广播值。",
        "TROWEXPANDDIV": "将每个有效行的元素除以该行的单列广播值。",
        "TROWEXPANDEXPDIF": "计算每个有效行元素与该行单列广播值之差的指数。",
        "TCOLARGMAX": "按行号递增顺序扫描每列，返回该列最大值对应的最小行索引。",
        "TCOLARGMIN": "按行号递增顺序扫描每列，返回该列最小值对应的最小行索引。",
        "TCOLMAX": "按行号递增顺序，以对应类型的最大值运算归约每列。",
        "TCOLMIN": "按行号递增顺序，以对应类型的最小值运算归约每列。",
        "TCOLSUM": "按行号递增顺序，以对应类型的加法归约每列。",
        "TCOLPROD": "按行号递增顺序，以对应类型的乘法归约每列。",
        "TCOLEXPAND": "将单行广播源逐位复制到每个有效目标行。",
        "TCOLEXPANDMAX": "将每个完整形状元素与同列广播值取对应类型的最大值。",
        "TCOLEXPANDMIN": "将每个完整形状元素与同列广播值取对应类型的最小值。",
        "TCOLEXPANDADD": "将每个完整形状元素加上同列广播值。",
        "TCOLEXPANDSUB": "从每个完整形状元素中减去同列广播值。",
        "TCOLEXPANDMUL": "将每个完整形状元素乘以同列广播值。",
        "TCOLEXPANDDIV": "将每个完整形状元素除以同列广播值。",
        "TCOLEXPANDEXPDIF": "计算每个完整形状元素与同列广播值之差的对应类型指数。",
        "TIMG2COL": "将描述符指定的 feature-map 窗口提取为标准 Left 矩阵顺序。",
        "TCONCAT": "沿列维度连接左、右源 Tile。",
        "TINSERT": "按编码偏移将源 Tile 插入目标 Tile 的旧值快照。",
        "TRESHAPE": "在保持元素顺序不变的情况下重塑源 Tile。",
        "TMRGSORT": "稳定合并两个已排序的单行 Local 数据流。",
        "TGATHER": "从各自独立选择的源行中收集值。",
        "TSCATTER": "将源值写入由索引 Tile 选择的各个目标行。",
        "TQUANT": "对 Local FP32 Tile 执行仿射量化，生成新的 Local S8 或 U8 Tile。",
        "TDEQUANT": "对每个有效的 S8 或 U8 源元素计算 FP32 结果 `(q - zero_point) * multiplier`。",
        "TCI": "从绑定的起始值构造一行指定类型的序列，并按逻辑列递增或递减。",
        "THISTOGRAM": "为每个源行构造包含 256 个 U32 bin 的前缀直方图。",
        "TTRANS": "将源 Tile 转置后写入目标 Tile。",
    }
    scalar_actions = {
        "TADDS": "将参与运算的 PE 的 private-GPR 标量加到每个有效 Local 源元素上。",
        "TSUBS": "从每个有效源元素中减去一个标量，并发布新的 Local 目标。",
        "TMULS": "将每个有效 Local Tile 元素乘以 private-GPR 标量。",
        "TDIVS": "按照选定的数值解释，将每个有效 Tile 元素除以一个 private-GPR 标量。",
        "TREMS": "对每个有效元素与一个标量计算带除数符号的取模结果，并发布新的 Local 目标。",
        "TMAXS": "选择每个有效 Tile 元素与一个标量中对应类型的最大值。",
        "TMINS": "选择每个有效 Tile 元素与一个标量中对应类型的最小值。",
        "TANDS": "对每个有效整数元素与一个标量执行按元素宽度的按位与。",
        "TORS": "对每个有效整数 Tile 元素与一个标量执行按位或。",
        "TXORS": "对每个有效整数元素与一个标量执行按位异或，并发布新的 Local 目标。",
        "TSHLS": "将每个有效整数元素左移一个标量指定的位数，并发布新的 Local 目标。",
        "TSHRS": "将每个有效整数元素右移一个标量指定的位数，并发布新的 Local 目标。",
        "TSELS": "按照 packed predicate Tile，从 Tile 源或每 PE 标量中选择结果。",
        "TCMPS": "按照 CMode 将每个有效 Tile 元素与每 PE 标量比较，并将谓词结果编码为 0 或 1。",
    }
    matrix = {
        "TMATMUL_MX_BIAS": "将经过缩放的矩阵相乘后加上 Bias Tile。",
        "TMATMUL_MX_ACC": "将经过缩放的矩阵相乘，并把结果累加到提供的累加器 Tile 中。",
        "TGEMV": "将矩阵与向量相乘并发布新的目标。",
        "TGEMV_BIAS": "将矩阵与向量相乘后加上 bias Tile。",
        "TGEMV_ACC": "将矩阵与向量相乘，并把结果累加到提供的 Tile 中。",
        "TGEMV_MX": "将经过缩放的矩阵与向量相乘并发布新的目标。",
        "TGEMV_MX_BIAS": "将经过缩放的矩阵与向量相乘后加上 bias Tile。",
        "TGEMV_MX_ACC": "将经过缩放的矩阵与向量相乘，并把结果累加到提供的 Tile 中。",
    }
    if name in direct or name in scalar_actions or name in matrix:
        meaning = direct.get(name) or scalar_actions.get(name) or matrix.get(name)
        return f"`{name}` {meaning}"
    body = section(spec_text, f"What {name} does") or section(spec_text, "Purpose")
    paragraphs = [clean(item) for item in re.split(r"\n\s*\n", body)]
    source = next((item for item in paragraphs if item and
                   not item.startswith("This operation")), "")
    if not source:
        return f"`{name}` 的操作语义由本页的约束和结果说明定义。"

    # The SPEC summaries use a deliberately small vocabulary.  Translating
    # phrases (rather than inventing descriptions from the opcode) preserves
    # distinctions such as add vs. accumulate and gather vs. scatter.
    replacements = (
        ("is a selector-encoded Tile operation executed by", "是由"),
        ("its current instruction contract owns the exact bundle form and publication boundary", "其当前指令 contract 规定了确切的 bundle 形式和发布边界"),
        ("It adds corresponding valid elements from the ordered left and right Local sources", "它将有序的左、右 Local 源中对应的有效元素相加"),
        ("It divides corresponding numerator and denominator elements under the selected integer or floating interpretation", "它按照选定的整数或浮点解释对对应的分子和分母元素进行除法"),
        ("It applies element-width bitwise AND to corresponding valid integer elements", "它对对应的有效整数元素执行按元素宽度的按位与"),
        ("It compares corresponding numeric elements under CMode and packs zero-or-one predicate results", "它按照 CMode 比较对应的数值元素，并将谓词结果编码为 0 或 1"),
        ("It applies typed absolute value independently to every valid source coordinate", "它对每个有效源坐标独立执行对应类型的绝对值运算"),
        ("It applies the selected same-type natural exponential to every valid floating element", "它对每个有效浮点元素执行选定的同类型自然指数运算"),
        ("It uses each integer index as a signed or unsigned GM byte displacement and gathers the addressed elements into a new Local Tile", "它将每个整数索引作为有符号或无符号的 GM 字节位移，并把寻址得到的元素收集到新的 Local Tile 中"),
        ("It uses each integer index as a GM byte displacement and stores the corresponding valid source element", "它将每个整数索引作为 GM 字节位移，并存储对应的有效源元素"),
        ("It gathers only lanes whose predicate is exactly one and fills disabled lanes with the selected padding value", "它只收集谓词恰为 1 的 lane，并用选定的 padding 值填充禁用的 lane"),
        ("It stores only exact-one predicate lanes at their indexed GM byte displacements", "它只将谓词恰为 1 的 lane 存储到索引指定的 GM 字节位移处"),
        ("It uses index, expected, and replacement Tiles to perform per-element GM compare-and-swap and records each observed old value", "它使用 index、expected 和 replacement Tile 对 GM 中的每个元素执行比较交换，并记录观察到的旧值"),
        ("It resolves one peer-selected read-old Local fragment for each PE and byte-copies it into the selected new Local fragments", "它为每个 PE 解析一个由 peer 选择的 read-old Local fragment，并将其按字节复制到选定的新 Local fragment"),
        ("copies source payload and definedness into the destination", "将源 Tile 的 payload 和 definedness 复制到目标 Tile"),
        ("loads one ordinary Local or Shared rectangle, or converts GM data into persistent Local CUBE storage", "加载一个普通的 Local 或 Shared 矩形，或将 GM 数据转换为持久的 Local CUBE 存储"),
        ("stores one valid Local or Shared rectangle to GM without modifying the source Tile", "将一个有效的 Local 或 Shared 矩形存储到 GM，且不修改源 Tile"),
        ("prefetches a typed, strided GM rectangle for all four PEs without a Tile destination", "为全部四个 PE 预取一个带类型且带步长的 GM 矩形，不产生 Tile 目标"),
        ("multiplies matrices into one newly published destination Tile", "将矩阵相乘并发布一个新的目标 Tile"),
        ("multiplies matrices and adds the Bias Tile", "将矩阵相乘后加上 Bias Tile"),
        ("multiplies matrices and accumulates into the supplied accumulator Tile", "将矩阵相乘的结果累加到提供的累加器 Tile 中"),
        ("multiplies scaled matrices", "将经过缩放的矩阵相乘"),
        ("multiplies the matrix and vector", "将矩阵与向量相乘"),
        ("adds corresponding elements", "将对应元素相加"),
        ("subtracts each right-source element from the corresponding left-source element", "从对应的左源元素中减去右源元素"),
        ("multiplies corresponding elements of two Local Tiles", "将两个 Local Tile 的对应元素相乘"),
        ("computes divisor-signed modulo for corresponding elements", "对对应元素计算带除数符号的取模结果"),
        ("computes bitwise OR of corresponding integer elements", "对对应整数元素执行按位或"),
        ("computes bitwise XOR for corresponding integer elements", "对对应整数元素执行按位异或"),
        ("left-shifts each integer element by the corresponding masked count", "按照对应的掩码计数对每个整数元素执行左移"),
        ("right-shifts each signed or unsigned integer element by the corresponding masked count", "按照对应的掩码计数对每个有符号或无符号整数元素执行右移"),
        ("selects typed maxima from corresponding Local Tile elements", "从对应的 Local Tile 元素中选择对应类型的最大值"),
        ("selects typed minima from corresponding Local Tile elements", "从对应的 Local Tile 元素中选择对应类型的最小值"),
        ("computes a same-type reciprocal for every valid element", "对每个有效元素计算同类型倒数"),
        ("computes a same-type square root for every valid element", "对每个有效元素计算同类型平方根"),
        ("computes a same-type reciprocal square root for every valid element", "对每个有效元素计算同类型平方根的倒数"),
        ("applies an elementwise rectifier to every valid element", "对每个有效元素执行逐元素修正线性单元运算"),
        ("selects exact carrier bits from two Tile sources under a packed predicate Tile", "按照 packed predicate Tile 从两个 Tile 源中选择原始 carrier bit"),
        ("transposes the source Tile into its destination", "将源 Tile 转置后写入目标 Tile"),
        ("stably sorts independent row groups and publishes values with original within-group U32 indices", "稳定地排序相互独立的行组，并发布带有组内原始 U32 索引的值"),
        ("selects the lowest column index of each row maximum", "选择每行最大值对应的最小列索引"),
        ("forms an origin-anchored union and adds overlap elements", "构造以原点对齐的并集，并将重叠元素相加"),
        ("forms an origin-anchored union and multiplies overlap elements", "构造以原点对齐的并集，并将重叠元素相乘"),
        ("forms an origin-anchored union and selects overlap maxima", "构造以原点对齐的并集，并选择重叠位置的最大值"),
        ("forms an origin-anchored union and selects overlap minima", "构造以原点对齐的并集，并选择重叠位置的最小值"),
        ("publishes a new Local destination", "并发布新的 Local 目标"),
        ("publishes a new destination", "并发布新的目标"),
        ("into one newly published destination", "并发布到新的目标中"),
        ("every valid element", "每个有效元素"),
        ("corresponding valid elements", "对应的有效元素"),
    )
    translated = source
    for english, chinese in replacements:
        translated = translated.replace(english, chinese)
    # Complete the common selector prefix after its two operands have been
    # translated.  The exact execution unit remains useful information.
    translated = re.sub(r"是由 ([A-Za-z]+)\. (.*)", r"是由 \1 执行的选择器编码 Tile 操作：\2。", translated)
    translated = re.sub(r"^`([^`]+)` is ", r"`\1`：", translated)
    translated = translated.replace(";", "；").replace(" and ", "，并")
    translated = translated.replace("； ", "；").replace(".", "。")
    translated = re.sub(r"。+$", "。", translated)
    if translated.startswith(name + " "):
        translated = f"`{name}` " + translated[len(name) + 1:]
    translated = re.sub(r"\s+", " ", translated).strip()
    return translated

def spec_doc(name: str) -> Path:
    matches = list(SPEC_DOC_ROOT.rglob(f"{name}.md"))
    if not matches: raise FileNotFoundError(f"PTO-SPEC page not found: {name}")
    return matches[0]

def output_doc(name: str) -> Path:
    matches = [p for p in DOC_ROOT.rglob(f"{name}.md")
               if "concepts" not in p.parts and "generated" not in p.parts]
    if not matches: raise FileNotFoundError(f"usage page not found: {name}")
    return matches[0]

def signatures(name: str, header: str) -> list[str]:
    result = []
    for m in re.finditer(rf"\b{re.escape(name)}\s*\(", header):
        opening = header.find("(", m.start(), m.end())
        depth = 0; closing = None
        for i in range(opening, len(header)):
            if header[i] == "(": depth += 1
            elif header[i] == ")":
                depth -= 1
                if depth == 0: closing = i; break
        if closing is None: continue
        start = header.rfind("\n", 0, m.start()) + 1
        template = header.rfind("template", 0, start)
        if template >= 0 and "\n\n" not in header[template:start] and "{" not in header[template:start]:
            start = template
        value = re.sub(r"\s+", " ", header[start:closing + 1]).strip() + ";"
        if value.startswith(("void ", "inline ", "PTO_SHARED_INLINE", "template ")) and value not in result:
            result.append(value)
    return result

def split_top_level(value: str, delimiter: str = ",") -> list[str]:
    """Split a C++-like list without splitting nested template arguments."""
    result = []
    depth = 0
    start = 0
    pairs = {"<": ">", "(": ")", "[": "]", "{": "}"}
    openings = set(pairs)
    closings = set(pairs.values())
    for index, char in enumerate(value):
        if char in openings:
            depth += 1
        elif char in closings:
            depth -= 1
        elif char == delimiter and depth == 0:
            result.append(value[start:index].strip())
            start = index + 1
    result.append(value[start:].strip())
    return [item for item in result if item]

def format_signature(signature: str, width: int = 100) -> str:
    """Lay out generated declarations without changing their C++ meaning."""
    signature = re.sub(r"\s+", " ", signature).strip()
    template_prefix = ""
    if signature.startswith("template <"):
        opening = signature.find("<")
        depth = 0
        closing = None
        for index in range(opening, len(signature)):
            if signature[index] == "<":
                depth += 1
            elif signature[index] == ">":
                depth -= 1
                if depth == 0:
                    closing = index
                    break
        if closing is not None:
            template_items = split_top_level(signature[opening + 1:closing])
            compact = f"template <{', '.join(template_items)}>"
            if len(compact) <= width:
                template_prefix = compact + "\n"
            else:
                template_prefix = "template <\n    " + ",\n    ".join(template_items) + ">\n"
            signature = signature[closing + 1:].strip()

    function = re.search(r"\b[A-Z][A-Z0-9_]*\s*\(", signature)
    opening = function.end() - 1 if function else signature.find("(")
    closing = signature.rfind(")")
    if opening < 0 or closing < opening:
        return template_prefix + signature
    prefix = signature[:opening].rstrip()
    suffix = signature[closing + 1:].strip()
    requires = re.search(r"(?:^|\s)(requires\(.+?\))\s+", prefix)
    if requires and len(prefix) > width:
        before = prefix[:requires.start()].rstrip()
        prefix = ((before + "\n") if before else "") + requires.group(1) + "\n" + prefix[requires.end():].lstrip()
    parameters = split_top_level(signature[opening + 1:closing])
    compact = f"{prefix}({', '.join(parameters)}){suffix}"
    if len(compact) <= width:
        return template_prefix + compact
    return (template_prefix + prefix + "(\n    "
            + ",\n    ".join(parameters) + f"){suffix}")

def composition(text: str) -> str:
    body = section(text, "Block composition")
    m = re.search(r"```(?:asm)?\s*\n(.*?)```", body, re.S)
    value = m.group(1).replace("\r", "").strip() if m else clean(body)
    return format_composition(value)

def format_composition(value: str) -> str:
    """Format a bundle skeleton as one aligned instruction per line."""
    value = re.sub(r"^BSTART\s+\.", "BSTART.", value, flags=re.M)
    value = re.sub(r"^(B\.[A-Z]+)\s+/B\.", r"\1/B.", value, flags=re.M)
    logical_lines = []
    for raw_line in value.splitlines():
        line = raw_line.strip()
        if line and not line.startswith("B") and logical_lines:
            logical_lines[-1] += " " + line
        else:
            logical_lines.append(line)
    lines = []
    for line in logical_lines:
        if not line:
            continue
        match = re.match(
            r"(B(?:START(?:\.[A-Z0-9_.]+)?|STOP|\.[A-Z]+(?:/B\.[A-Z]+)?))\s*(.*)",
            line,
        )
        if not match:
            lines.append(line)
            continue
        mnemonic, operands = match.groups()
        dim = re.match(r"LB([0-2])=([A-Za-z][A-Za-z0-9_]*)\s*(.*)", operands)
        if mnemonic == "B.DIM" and dim:
            lb, name, remainder = dim.groups()
            register = name if name.startswith("r") else f"r{name}"
            operands = f"{register}, 0, ->LB{lb}"
            if remainder:
                operands += f"  ; {remainder}"
        operands = operands.replace("<last>", "last")
        separator = " " if mnemonic.startswith("BSTART") else " " * max(1, 12 - len(mnemonic))
        lines.append(mnemonic if not operands else f"{mnemonic}{separator}{operands}")
    return "\n".join(lines)

def call_arguments(signature: str) -> str:
    params = signature[signature.find("(") + 1:signature.rfind(")")]
    names = []
    depth = 0
    start = 0
    chunks = []
    for i, char in enumerate(params + ","):
        if char in "<({": depth += 1
        elif char in ">)}": depth -= 1
        elif char == "," and depth == 0:
            chunks.append(params[start:i].strip())
            start = i + 1
    for param in chunks:
        param = param.split("=")[0].strip()
        match = re.search(r"([A-Za-z_]\w*)\s*(?:\]|$)", param)
        if match:
            names.append(match.group(1))
    return ", ".join(names)

def render(name: str, spec: Path, header: str, existing: str = "") -> str:
    text = spec.read_text(encoding="utf-8")
    intro = chinese_intro(name, text)
    sigs = signatures(name, header)
    if not sigs: raise ValueError(f"No C++ declaration found: {name}")
    link = str(spec.relative_to(SPEC_ROOT)).replace("\\", "/")
    invocation = f"{name}({call_arguments(sigs[0])});"
    operation_constraints = operation_contract(name)
    rendered = f'''# {name}

{intro}

## C++ 接口

当前 API 中可用的调用形式：

```cpp
{chr(10).join(format_signature(signature) for signature in sigs)}
```

### 支持的数据类型

{supported_types(text, name)}

{dtype_roles(name, supported_types(text, name))}

### 参数说明

{parameter_table(sigs)}

{overload_selection(sigs)}

## 使用要求

- Tile 类型必须满足接口模板约束；
- 数据类型、形状、有效区域、布局、容量和存储位置必须满足该操作要求；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

{operation_constraints}

    操作数角色、数据类型组合、容量、PE mask 和 alias 必须符合上方约束；只能使用所选重载声明的操作数形式。

{valid_region_rules(name)}

{tlsu_stride_guide(name)}

## 默认值

 {default_values(sigs, text)}

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`{name}` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
{composition(text)}
```

## 使用示例

```cpp
#include "jcore/template_asm.hpp"

// 使用满足 {name} 约束的具体 Tile 类型和参数。
{invocation}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。

'''
    # Keep hand-written examples.  The generated fallback is useful for new
    # pages, but must not erase a verified data-flow example on regeneration.
    old_example = section(existing, "使用示例")
    if old_example and "使用满足" not in old_example and "请替换" not in old_example:
        rendered = re.sub(
            r"## 使用示例\n.*$",
            "## 使用示例\n\n" + old_example,
            rendered,
            flags=re.S,
        )
    # Keep reviewed instruction skeletons, especially for operations whose
    # specification describes Local/Shared variants in prose instead of an
    # assembly code block.
    old_composition = (section(existing, "Bundle 组成") or
                       section(existing, "Bundle composition"))
    old_asm = re.search(r"```asm\s*\n(.*?)```", old_composition, re.S)
    if old_asm and "BSTART" in old_asm.group(1):
        reviewed = format_composition(old_asm.group(1))
        rendered = re.sub(
            r"(## (?:Bundle 组成|Bundle composition)\n.*?```asm\n).*?(\n```)",
            lambda match: match.group(1) + reviewed + match.group(2),
            rendered,
            count=1,
            flags=re.S,
        )
    return rendered

def main() -> None:
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    header = HEADER.read_text(encoding="utf-8")
    for op in catalog["operations"]:
        target = output_doc(op["name"])
        existing = target.read_text(encoding="utf-8")
        target.write_text(render(op["name"], spec_doc(op["name"]), header, existing), encoding="utf-8")
        print(target.relative_to(ROOT))

if __name__ == "__main__": main()

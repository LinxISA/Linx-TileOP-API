.text

# PTO ISA 0.58.3 descriptor surface used by the TileOP headers.  This fixture
# is intentionally register-concrete so the integrated assembler/disassembler
# can be validated even when the C++ Tile register frontend is unavailable.
BSTART.TLOAD FP32
BSTART.TSTORE FP32
BSTART.TMATMUL FP32
B.DATR layout21, DTYPE_NONE, Null
B.DATR layout26, DTYPE_NONE, Null
B.FPATR 0, 0, 0, 0, 0, 0, 0, 1, 1
B.IOR [a0, a1], []
B.IOT t#1, mask=1100, last, ->m<64KB>
B.IOS S1, mask=1111
B.IOS mask=1111, ->S255<256KB>

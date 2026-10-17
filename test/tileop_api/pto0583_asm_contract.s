.text

# PTO ISA 0.58.7 development descriptor surface used by the TileOP headers.  This fixture
# is intentionally register-concrete so the integrated assembler/disassembler
# can be validated even when the C++ Tile register frontend is unavailable.
BSTART.TLOAD FP32
B.DATR ND2M32.normal, Zero
BSTART.TSTORE FP32
B.DATR N82ND.normal, Null
BSTART.TMATMUL FP32
B.FPATR 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
B.IOR [a0, a1], []
B.IOT t#1.reuse, mask=1100, last, ->m<64KB>
B.IOS S1.reuse, mask=1111
B.IOT t#1.reuse, u#2.reuse, mask=1111, last
B.IOS mask=1111, ->S63<256KB>
BSTART.TIMG2COL FP32
B.DATR ND2M32, DTYPE_NONE, Zero
B.DIM a0, 0, ->lb0
B.DIM a1, 0, ->lb1
B.DIM zero, 256, ->lb2
B.IOR [a2, zero, zero], []
B.IOR [a3, a4, a5], []
B.IOT mask=1111, last, ->t<64KB>

# Optional MX scale binders use constant assembler conditions after inline-asm
# substitution.  Prove the matching MC accepts the form and fully elides the
# absent source while retaining the present source.
.if 0
B.IOT t#2.reuse, mask=1111
.endif
.if 1
B.IOT t#3.reuse, mask=1111, last, ->t<1KB>
.endif

# PTO-ISA 0.58.7 range modifiers (ADR-0098): each modifier is immediately
# adjacent to the binder and uses the binder's storage class and role.
B.IOT t#4.reuse, mask=1111, last, ->t<1KB>
B.SUBVIEW 0, a0, 0, 1
B.IOS mask=1111, ->S2<1KB>
B.ASSEMBLE 1, 0, zero, 100, 12
B.ASSEMBLE 1, 0, a0, 100, 12

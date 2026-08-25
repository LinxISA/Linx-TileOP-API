.text

# PTO ISA 0.58.3 descriptor surface used by the TileOP headers.  This fixture
# is intentionally register-concrete so the integrated assembler/disassembler
# can be validated even when the C++ Tile register frontend is unavailable.
BSTART.TLSU TLOAD, FP32
B.DATR ND2M32.normal, Zero
BSTART.TLSU TSTORE, FP32
B.DATR N82ND.normal, Null
BSTART.CUBE TMATMUL, FP32
B.FPATR 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
B.IOR [a0, a1], []
B.IOT t#1, mask=1100, last, ->m<64KB>
B.IOS S1, mask=1111
B.IOS mask=1111, ->S255<256KB>

# Optional MX scale binders use constant assembler conditions after inline-asm
# substitution.  Prove the matching MC accepts the form and fully elides the
# absent source while retaining the present source.
.if 0
B.IOT t#2, mask=1111
.endif
.if 1
B.IOT t#3, mask=1111, last, ->t<1KB>
.endif

# PTO-ISA 0.58.4 range modifiers (ADR-0098): standalone block commands whose
# B.SUBVIEW/B.ASSEMBLE lines attach to the immediately preceding binder in a
# real bundle; here they are validated as independently assemblable commands
# so the MC/disassembler round-trip covers their fields.
B.SUBVIEW 0, a0, 0, 1
B.ASSEMBLE 1, 0, a0, 100, 12

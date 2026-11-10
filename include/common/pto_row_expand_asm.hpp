#ifndef PTO_ROW_EXPAND_ASM_HPP
#define PTO_ROW_EXPAND_ASM_HPP

// TROWEXPAND* interprets RMode[17:15] as BroadcastByteOffset, not rounding.
// LLVM's RMode operand accepts these symbolic spellings, whose encoding is
// exactly 0..7. Keep the existing two-field spelling for offset zero.
// Layout and Offset are string literals (Offset may name an asm operand).
#define PTO_ROW_EXPAND_CUBE_DATR_ASM(Layout, Offset)                            \
  ".if " Offset " == 0\nB.DATR " Layout ", Null\n"                           \
  ".elseif " Offset " == 1\nB.DATR " Layout ", DTYPE_NONE, Null, EQ, RNE\n"   \
  ".elseif " Offset " == 2\nB.DATR " Layout ", DTYPE_NONE, Null, EQ, RTZ\n"   \
  ".elseif " Offset " == 3\nB.DATR " Layout ", DTYPE_NONE, Null, EQ, RTM\n"   \
  ".elseif " Offset " == 4\nB.DATR " Layout ", DTYPE_NONE, Null, EQ, RTP\n"   \
  ".elseif " Offset " == 5\nB.DATR " Layout ", DTYPE_NONE, Null, EQ, RNA\n"   \
  ".elseif " Offset " == 6\nB.DATR " Layout ", DTYPE_NONE, Null, EQ, RTO\n"   \
  ".elseif " Offset " == 7\nB.DATR " Layout ", DTYPE_NONE, Null, EQ, RHB\n"   \
  ".else\n.error \"TROWEXPAND broadcast byte offset must be in 0..7\"\n"      \
  ".endif\n"

#endif // PTO_ROW_EXPAND_ASM_HPP

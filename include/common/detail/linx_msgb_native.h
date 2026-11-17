#ifndef LINX_MSGB_NATIVE_H
#define LINX_MSGB_NATIVE_H

#include <stdint.h>
#include "linx_msgb_registers.h"

#if !defined(__linx_v5) || __linx_xlen != 64
#error "The MSGB native transport requires a Linx V5 64-bit target"
#endif

/* Each register access executes in a sequential SYS block. Restore STD for
 * the compiler's following scalar instructions. The memory clobber orders C
 * accesses around the command at compile time; it emits no GM fence. */
#define LINX_MSGB_SYS(instruction) \
    "C.BSTART.SYS FALL\n\t" instruction "\n\tC.BSTART.STD FALL"

#define LINX_MSGB_READ_CASE(address) \
    case address: \
        __asm__ volatile (LINX_MSGB_SYS("SSRGET %1, ->%0") \
                          : "=r"(value) : "i"(address) : "memory"); \
        return value

#define LINX_MSGB_WRITE_CASE(address) \
    case address: \
        __asm__ volatile (LINX_MSGB_SYS("SSRSET %0, %1") \
                          : : "r"(value), "i"(address) : "memory"); \
        return

/* SSRGET and SSRSET take an immediate address. D1..D10 read from the
 * current RX snapshot and write to TX staging. */
#define LINX_MSGB_DATA_CASES(op) \
    op(LINX_MSGB_DATA1); op(LINX_MSGB_DATA1 + 1); op(LINX_MSGB_DATA1 + 2); \
    op(LINX_MSGB_DATA1 + 3); op(LINX_MSGB_DATA1 + 4); \
    op(LINX_MSGB_DATA1 + 5); op(LINX_MSGB_DATA1 + 6); op(LINX_MSGB_DATA1 + 7); \
    op(LINX_MSGB_DATA1 + 8); op(LINX_MSGB_DATA1 + 9)

static inline uint64_t linx_msgb_io_read(unsigned reg)
{
    uint64_t value;
    switch (reg) {
        LINX_MSGB_DATA_CASES(LINX_MSGB_READ_CASE);
        LINX_MSGB_READ_CASE(LINX_MSGB_RXSTATE);
        LINX_MSGB_READ_CASE(LINX_MSGB_RXTOKEN);
        LINX_MSGB_READ_CASE(LINX_MSGB_RXID_HI);
        LINX_MSGB_READ_CASE(LINX_MSGB_RXID_LO);
        LINX_MSGB_READ_CASE(LINX_MSGB_RXCONTROL);
        LINX_MSGB_READ_CASE(LINX_MSGB_ACKSTAT);
        LINX_MSGB_READ_CASE(LINX_MSGB_ACKID);
        LINX_MSGB_READ_CASE(LINX_MSGB_TXID_HI);
        LINX_MSGB_READ_CASE(LINX_MSGB_TXID_LO);
        LINX_MSGB_READ_CASE(LINX_MSGB_TXSTAT);
        LINX_MSGB_READ_CASE(LINX_MSGB_ENDPOINT);
        default: __builtin_trap();
    }
}

static inline void linx_msgb_io_write(unsigned reg, uint64_t value)
{
    switch (reg) {
        LINX_MSGB_DATA_CASES(LINX_MSGB_WRITE_CASE);
        LINX_MSGB_WRITE_CASE(LINX_MSGB_RXCLAIM);
        LINX_MSGB_WRITE_CASE(LINX_MSGB_RXRELEASE);
        LINX_MSGB_WRITE_CASE(LINX_MSGB_ACK);
        default: __builtin_trap();
    }
}

static inline void linx_msgb_io_send(uint64_t control)
{
    __asm__ volatile (LINX_MSGB_SYS("BSE %0") : : "r"(control) : "memory");
}

static inline void linx_msgb_io_stage(unsigned index, uint64_t value)
{
    if (index >= 10) __builtin_trap();
    linx_msgb_io_write(LINX_MSGB_DATA1 + index, value);
}

static inline uint64_t linx_msgb_io_load(unsigned index)
{
    if (index >= 10) __builtin_trap();
    return linx_msgb_io_read(LINX_MSGB_DATA1 + index);
}

#undef LINX_MSGB_DATA_CASES
#undef LINX_MSGB_READ_CASE
#undef LINX_MSGB_WRITE_CASE
#undef LINX_MSGB_SYS

#endif

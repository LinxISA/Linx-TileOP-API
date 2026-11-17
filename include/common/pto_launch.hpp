#ifndef PTO_LAUNCH_HPP
#define PTO_LAUNCH_HPP

#include <stdint.h>

#if !defined(__linx_v5) || __linx_xlen != 64
#error "PTO launch context requires a Linx V5 64-bit target"
#endif

namespace pto {

// Number of logical block instances participating in the current launch.
inline uint32_t GetBlockNum()
{
    uint64_t value;
    __asm__ volatile(
        "C.BSTART.SYS FALL\n\tSSRGET 0x0050, ->%0\n\tC.BSTART.STD FALL"
        : "=r"(value) : : "memory");
    return static_cast<uint32_t>(value);
}

// Zero-based logical block instance assigned to the current kernel call.
inline uint32_t GetBlockIdx()
{
    uint64_t value;
    __asm__ volatile(
        "C.BSTART.SYS FALL\n\tSSRGET 0x0051, ->%0\n\tC.BSTART.STD FALL"
        : "=r"(value) : : "memory");
    return static_cast<uint32_t>(value);
}

} // namespace pto

#endif

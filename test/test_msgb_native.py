#!/usr/bin/env python3
"""Check native #426 transport instructions and freestanding C++ message linkage."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
PROBE = r"""
#include <common/linx_msgb.h>

int send_probe(uint32_t endpoint, const uint64_t *data, size_t count,
               linx_msgb_id *id) {
    return linx_msgb_try_send(endpoint, data, count, id);
}
int claim_probe(linx_msgb_message *message) { return linx_msgb_try_claim(message); }
int recover_probe(linx_msgb_message *message) { return linx_msgb_read_claim(message); }
int ack_probe(linx_msgb_claim claim) { return linx_msgb_ack(claim); }
int release_probe(linx_msgb_claim claim) { return linx_msgb_release(claim); }
"""


TYPED_PROBE = r"""
#include <common/pto_message.hpp>

struct Message { unsigned char bytes[PAYLOAD_BYTES]; };

extern "C" void kernel(uint32_t destination, const Message &out, Message &in) {
    (void)pto::PTOSendMessage(destination, out);
    (void)pto::PTORecvMessage(in);
}
"""


class MsgbNativeTest(unittest.TestCase):
    def setUp(self):
        self.compiler = os.environ.get("LINX_CXX")
        if not self.compiler:
            self.skipTest("set LINX_CXX to a Linx target clang++")

    def build_probe(self, source, obj, language, standard, *flags):
        subprocess.run([self.compiler, "--target=linx64v5-unknown-linux-musl",
                        "-x", language, "-std=" + standard, "-ffreestanding",
                        "-O2", "-I", str(ROOT / "include"), *flags,
                        "-c", str(source), "-o", str(obj)], check=True)

    def disassemble(self, obj):
        objdump = Path(self.compiler).parent / "llvm-objdump"
        return subprocess.check_output([str(objdump), "-d", str(obj)], text=True)

    def test_operational_registers_and_sys_blocks(self):
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory)
            source = work / "probe.c"
            obj = work / "probe.o"
            source.write_text(PROBE)
            for language, standard in (("c", "c11"), ("c++", "c++20")):
                with self.subTest(language=language):
                    self.build_probe(source, obj, language, standard)
                    self.check_encodings(self.disassemble(obj))

    def test_freestanding_typed_messages(self):
        linker = Path(self.compiler).parent / "ld.lld"
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory)
            source = work / "typed.cpp"
            obj = work / "typed.o"
            elf = work / "typed.elf"
            source.write_text(TYPED_PROBE)
            for size in (9, 80):
                with self.subTest(payload_bytes=size):
                    self.build_probe(source, obj, "c++", "c++17", "-nostdinc++",
                                     "-fno-exceptions", "-fno-rtti",
                                     f"-DPAYLOAD_BYTES={size}")
                    # Link the object alone to expose compiler-generated libc
                    # calls such as memcpy or memset in either wrapper.
                    subprocess.run([str(linker), "-e", "kernel", str(obj),
                                    "-o", str(elf)], check=True)
                    self.check_encodings(self.disassemble(elf), send_words=(size + 7) // 8)

    def check_encodings(self, listing, send_words=10):
        # Use the ASL masks and encoded header words. Some Linx tools print
        # the SYS header as C.BSTART.AUX.
        instructions = [int(word, 16) for word in re.findall(
            r"^\s*[0-9a-f]+:\s+([0-9a-f]+)\s+", listing, re.MULTILINE)]
        reads, writes = set(), set()
        sends = 0
        for index, word in enumerate(instructions):
            if word & 0x000ff07f == 0x0000003b:       # SSRGET
                reads.add(word >> 20)
                selector = (word >> 7) & 0x1f
            elif word & 0x00007fff == 0x0000103b:     # SSRSET
                writes.add(word >> 20)
                selector = (word >> 15) & 0x1f
            elif word & 0xfff07fff == 0x0000002b:     # BSE
                sends += 1
                selector = (word >> 15) & 0x1f
            else:
                continue
            # T/U selectors 24..31 are block-local. Each operand crosses
            # a block boundary and must use a persistent scalar register.
            self.assertLess(selector, 24)
            self.assertGreater(index, 0)
            self.assertLess(index + 1, len(instructions))
            self.assertEqual(instructions[index - 1], 0x0840)  # C.BSTART.SYS FALL
            self.assertEqual(instructions[index + 1], 0x0800)  # C.BSTART.STD FALL
        data = set(range(0x831, 0x83b))
        status_reads = set(range(0x83c, 0x841)) | set(range(0x843, 0x849))
        required_data = set(range(0x831, 0x831 + send_words))
        self.assertTrue(required_data | status_reads <= reads <= data | status_reads)
        # A fixed-size send may fold unused cases out of its TX staging switch.
        required_writes = required_data | {0x83b, 0x841, 0x842}
        self.assertTrue(required_writes <= writes <= data | {0x83b, 0x841, 0x842})
        self.assertGreater(sends, 0)


if __name__ == "__main__":
    unittest.main()

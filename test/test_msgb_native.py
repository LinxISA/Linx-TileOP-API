#!/usr/bin/env python3
"""Check #426 transport instructions in an object built from the public C API."""
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


class MsgbNativeTest(unittest.TestCase):
    def test_operational_registers_and_sys_blocks(self):
        compiler = os.environ.get("LINX_CXX")
        if not compiler:
            self.skipTest("set LINX_CXX to a Linx target clang++")
        objdump = Path(compiler).parent / "llvm-objdump"
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory)
            source = work / "probe.c"
            obj = work / "probe.o"
            source.write_text(PROBE)
            for language, standard in (("c", "c11"), ("c++", "c++20")):
                with self.subTest(language=language):
                    subprocess.run([compiler, "--target=linx64v5-unknown-linux-musl",
                                    "-x", language, "-std=" + standard, "-ffreestanding",
                                    "-O2", "-I", str(ROOT / "include"), "-c", str(source),
                                    "-o", str(obj)], check=True)
                    listing = subprocess.check_output(
                        [str(objdump), "-d", str(obj)], text=True)
                    self.check_encodings(listing)

    def check_encodings(self, listing):
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
        self.assertEqual(reads, data | set(range(0x83c, 0x841)) | set(range(0x843, 0x849)))
        self.assertEqual(writes, data | {0x83b, 0x841, 0x842})
        self.assertGreater(sends, 0)


if __name__ == "__main__":
    unittest.main()

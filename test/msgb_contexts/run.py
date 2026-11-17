#!/usr/bin/env python3
"""Build one freestanding ELF, exercise MSGB workers and verify their GM dump."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


HERE = Path(__file__).resolve().parent


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


# Frozen, little-endian wire examples independent of the target and its dump.
# The first pair retains the original 9-byte request and 80-byte reply.
WIRE_EXAMPLES = [
    ("d300ff718019aac435",
     "35d3dcba40302010d3f7683c7eb071ebfb9f30c4065819b383a7d8ec2e60215b"
     "ab4fe0b4f608c9637317885c9ed0910b1b3f5064a6f8b9d323c7780c4e8041fb"
     "cbef00d416a8698393b728fc3e7031ab"),
    ("f000ff718019aac436",
     "36f0dcba40302010f0d44b1f5d9352c9d8bc13e7257b3a91a084fbcf0d430279"
     "886cc397d52bea415034ab7fbdf3b229381c734785db9af100e45b2f6da362d9"
     "e8cc23f7358b4aa1b0940bdf1d531289"),
    ("9500ff718019aac437",
     "3795dcba4030201095b12e7a38f637afbdd97682401e5ff7c5e19eaa6826671f"
     "ed09a6f2b04e8f273551ce1ad896d74f5d791622e0beff9765813e4a08c607bf"
     "8da9469250ee2fc7d5f16eba783677ef"),
    ("ba00ff718019aac438",
     "38badcba40302010ba9e015517d9188192f659ad6f3170d9eaceb18547094831"
     "c22689dd9f61a0091a7ee135f7b9f8617256390dcf91d0b94aae116527e92891"
     "a28669bd7fc100e9fade4195571958c1"),
]


def golden_fields(pairs):
    fields = []
    for (sender, receiver), (request_hex, reply_hex) in zip(pairs, WIRE_EXAMPLES):
        request, reply = bytes.fromhex(request_hex), bytes.fromhex(reply_hex)
        assert len(request) == 9 and len(reply) == 80
        prefix = f"tid{sender}_tid{receiver}."
        fields.extend((prefix + name, value) for name, value in [
            ("head_guard", bytes.fromhex("b5 1d e2 78")),
            ("sent_request", request), ("received_request", request),
            ("middle_guard", bytes.fromhex("de ad be ef")),
            ("sent_reply", reply), ("received_reply", reply),
            ("end_guard", bytes.fromhex("55 aa c3 3c")),
            ("ready", bytes(0x31 + tid for tid in (sender, receiver))),
            ("endpoints", bytes(tid * 16 for tid in (sender, receiver))),
            ("done", bytes(0xa0 + tid for tid in (sender, receiver))),
        ])
    return fields


def build(tools, out, runtime, workers):
    obj = out / "kernel.o"
    include = HERE.parents[1] / "include"
    flags = [f"-DMSGB_WORKERS={workers}"]
    if runtime:
        flags.append("-DMSGB_RESIDENT_FIXTURE")
    subprocess.run([
        tools / "clang++", "--target=linx64v5-unknown-linux-musl", "-std=c++17", "-ffreestanding",
        "-nostdinc++", "-fno-vectorize", "-fno-slp-vectorize", "-fno-exceptions", "-fno-rtti",
        "-O2", "-I", include, *flags, "-c", HERE / "kernel.cpp", "-o", obj,
    ], check=True)
    if runtime:
        subprocess.run(["make", "-C", runtime.resolve(), "msgb-image", f"OUTPUT={out}",
                        f"LINX_CXX={tools / 'clang++'}", f"LINX_LD={tools / 'ld.lld'}",
                        f"TILEOP_INCLUDE={include}", f"KERNEL_OBJECTS={obj}"], check=True)
        return out / "workers.elf"
    elf = out / "msgb.elf"
    subprocess.run([tools / "ld.lld", "-T", HERE / "link.ld", obj, "-o", elf], check=True)
    return elf


def prepare_dispatch(symbols, manifest, pairs):
    role_pcs = {}
    for name in ("msgb_requester", "msgb_responder"):
        match = re.search(rf"^([0-9a-f]+)\s+g\s+F\s+\.text\s+[0-9a-f]+\s+{name}$",
                          symbols, re.M)
        if not match:
            raise RuntimeError(f"missing kernel {name}")
        role_pcs[name] = int(match[1], 16)

    events = {}
    jobs = []
    for sender, receiver in pairs:
        for tid, name in ((sender, "msgb_requester"), (receiver, "msgb_responder")):
            pc = role_pcs[name]
            dispatch_id = 90 + tid
            argument = (0x1012345678900000 if tid == sender else 0xfedcba9876543200) + tid
            jobs.append(f"{tid} {dispatch_id} {pc:#x} {argument:#x}\n")
            unit = f"worker={tid} pe={tid % 4} slot={tid // 4}"
            events[tid] = [f"HSCB READY {unit} endpoint={tid * 16}",
                           f"HSCB LAUNCH {unit} id={dispatch_id} pc={pc} arg={argument}",
                           f"HSCB DONE {unit} id={dispatch_id} result={argument ^ (0xa0 + tid)}"]
    manifest.write_text("".join(jobs))
    return events


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--tool-bin", required=True, type=Path, help="Linx LLVM bin directory")
    cli.add_argument("--gfrun", required=True, type=Path, help="gfrun with enabled MSGB support")
    cli.add_argument("--output", required=True, type=Path, help="fresh result directory")
    cli.add_argument("--runtime", type=Path, help="linx-runtime source tree for resident HSCB dispatch")
    cli.add_argument("--workers", choices=(2, 8), default=8, type=int,
                     help="use the default 4 PE x 2 hardware threads, or a reduced 2-worker control")
    args = cli.parse_args()
    workers = args.workers
    pairs = [(0, 4), (1, 2), (3, 7), (5, 6)] if workers == 8 else [(0, 1)]
    out, tools, model = (p.resolve() for p in (args.output, args.tool_bin, args.gfrun))
    out.mkdir(parents=True, exist_ok=False)
    elf = build(tools, out, args.runtime, workers)
    compiler = tools / "clang++"
    symbols = subprocess.check_output([tools / "llvm-objdump", "-t", elf], text=True)
    symbol = re.search(r"^([0-9a-f]+)\s+g\s+O\s+\.data\s+([0-9a-f]+)\s+msgb_results$",
                       symbols, re.M)
    if not symbol:
        raise RuntimeError("missing msgb_results")
    address, size = (int(value, 16) for value in symbol.groups())
    fields = golden_fields(pairs)
    golden = b"".join(value for _, value in fields)
    if size != len(golden):
        raise RuntimeError(f"result symbol size {size} differs from golden {len(golden)}")
    (out / "golden.bin").write_bytes(golden)
    actual = out / "actual.bin"
    state_file = out / "run-state.json"
    manifest = out / "jobs.txt"
    events = prepare_dispatch(symbols, manifest, pairs) if args.runtime else []
    options = ["--hscb-jobs", str(manifest)] if args.runtime else ["--test-finisher", "true"]
    command = [str(model), "-f", str(elf), "--msgb", *options,
               "--dump-memory", f"0x{address:x}:{size}:{actual}", "--dump-run-state", str(state_file),
               "-m", str(10000 * workers)]
    if workers == 2:
        command.extend(["-s", "softcore.multiThreadNum=2"])
    report = {"command": command, "elf_sha256": sha256(elf), "gfrun_sha256": sha256(model),
              "compiler": subprocess.check_output([compiler, "--version"], text=True).splitlines()[0],
              "address": hex(address), "size": size, "golden_sha256": sha256(out / "golden.bin")}
    with (out / "gfrun.log").open("w") as log:
        try:
            run = subprocess.run(command, cwd=model.parent.parent, stdout=log,
                                 stderr=subprocess.STDOUT, timeout=60)
            report["returncode"] = run.returncode
        except subprocess.TimeoutExpired:
            report["timeout"] = True
    data = actual.read_bytes() if actual.is_file() else b""
    report["actual_sha256"] = hashlib.sha256(data).hexdigest()
    comparisons = {}
    pos = 0
    for name, expected in fields:
        observed = data[pos:pos + len(expected)]
        comparisons[name] = {"expected": expected.hex(), "actual": observed.hex(),
                             "pass": observed == expected}
        pos += len(expected)
    report["fields"] = comparisons
    state = json.loads(state_file.read_text()) if state_file.is_file() else {}
    threads = state.get("threads", [])
    topology = [(t["thread"], t["pe"], t["slot"]) for t in threads]
    report["topology"] = topology
    report["active_workers"] = (topology == [(i, i % 4, i // 4) for i in range(workers)]
                                and all(t["retired_micro_instructions"] > 0 for t in threads))
    report["finisher"] = state.get("test_finisher_reached", False)
    if args.runtime:
        lines = (out / "gfrun.log").read_text().splitlines()
        scheduler = [line for line in lines if line.startswith("HSCB ")]
        report["scheduler"] = scheduler
        events_match = all(
            [line for line in scheduler if re.match(fr"^HSCB \w+ worker={tid} ", line)] == expected
            for tid, expected in events.items())
        report["parked"] = report["active_workers"] and all(
            not t["sim_end"] and t["message_wait"] and t["message_parks"] == 2 and
            t["message_wakes"] == 1 for t in threads)
        idle = f"HSCB IDLE workers={workers} completed={workers}"
        completed = (events_match and len(scheduler) == 3 * workers + 1
                     and scheduler[-1] == idle and report["parked"] and not report["finisher"])
    else:
        completed = report["finisher"]
    report["pass"] = (report.get("returncode") == 0 and data == golden and
                      report["active_workers"] and completed)
    (out / "result.json").write_text(json.dumps(report, indent=2) + "\n")
    if not report["pass"]:
        raise SystemExit(f"FAIL: inspect {out / 'result.json'} and {out / 'gfrun.log'}")
    print(f"PASS: {workers} workers, {len(pairs)} distinct 9-byte requests and 80-byte replies, "
          f"all {size} dump bytes match ({out})")


if __name__ == "__main__":
    main()

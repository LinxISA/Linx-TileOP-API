#!/usr/bin/env python3
"""Build one freestanding ELF, execute two gfrun PEs and verify their GM dump."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


HERE = Path(__file__).resolve().parent


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def golden_fields():
    # Independent little-endian wire examples, including the reply computed
    # from the request by PE1. Neither object is embedded as a golden on target.
    request = bytes.fromhex("d3 00 ff 71 80 19 aa c4 35")
    reply = bytes.fromhex(
        "35 d3 dc ba 40 30 20 10 "
        "d3 f7 68 3c 7e b0 71 eb fb 9f 30 c4 06 58 19 b3 "
        "83 a7 d8 ec 2e 60 21 5b ab 4f e0 b4 f6 08 c9 63 "
        "73 17 88 5c 9e d0 91 0b 1b 3f 50 64 a6 f8 b9 d3 "
        "23 c7 78 0c 4e 80 41 fb cb ef 00 d4 16 a8 69 83 "
        "93 b7 28 fc 3e 70 31 ab"
    )
    return [
        ("head_guard", bytes.fromhex("b5 1d e2 78")),
        ("pe0_sent_request", request), ("pe1_received_request", request),
        ("middle_guard", bytes.fromhex("de ad be ef")),
        ("pe1_sent_reply", reply), ("pe0_received_reply", reply),
        ("end_guard", bytes.fromhex("55 aa c3 3c")),
        ("ready", bytes.fromhex("31 32")), ("endpoints", bytes.fromhex("00 10")),
        ("done", bytes.fromhex("a0 a1")),
    ]


def build(tools, out, runtime):
    obj = out / "kernel.o"
    include = HERE.parents[1] / "include"
    flags = ["-DMSGB_RESIDENT_FIXTURE"] if runtime else []
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


def prepare_dispatch(symbols, manifest):
    # Both workers have distinct entry PCs, dispatch IDs and 64-bit arguments.
    events = []
    jobs = []
    for pe, (name, argument) in enumerate([("msgb_requester", 0x1012345678900000),
                                           ("msgb_responder", 0xfedcba9876543200)]):
        match = re.search(rf"^([0-9a-f]+)\s+g\s+F\s+\.text\s+[0-9a-f]+\s+{name}$",
                          symbols, re.M)
        if not match:
            raise RuntimeError(f"missing kernel {name}")
        pc = int(match[1], 16)
        dispatch_id = 90 + pe
        jobs.append(f"{pe} {dispatch_id} {pc:#x} {argument:#x}\n")
        events.append([f"HSCB READY pe={pe} endpoint={pe * 16}",
                       f"HSCB LAUNCH pe={pe} id={dispatch_id} pc={pc} arg={argument}",
                       f"HSCB DONE pe={pe} id={dispatch_id} result={argument ^ (0xa0 + pe)}"])
    manifest.write_text("".join(jobs))
    return events


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--tool-bin", required=True, type=Path, help="Linx LLVM bin directory")
    cli.add_argument("--gfrun", required=True, type=Path, help="gfrun with enabled MSGB support")
    cli.add_argument("--output", required=True, type=Path, help="fresh result directory")
    cli.add_argument("--runtime", type=Path, help="linx-runtime source tree for resident HSCB dispatch")
    args = cli.parse_args()
    out, tools, model = (p.resolve() for p in (args.output, args.tool_bin, args.gfrun))
    out.mkdir(parents=True, exist_ok=False)
    elf = build(tools, out, args.runtime)
    compiler = tools / "clang++"
    symbols = subprocess.check_output([tools / "llvm-objdump", "-t", elf], text=True)
    symbol = re.search(r"^([0-9a-f]+)\s+g\s+O\s+\.data\s+([0-9a-f]+)\s+msgb_results$",
                       symbols, re.M)
    if not symbol:
        raise RuntimeError("missing msgb_results")
    address, size = (int(value, 16) for value in symbol.groups())
    fields = golden_fields()
    golden = b"".join(value for _, value in fields)
    if size != len(golden):
        raise RuntimeError(f"result symbol size {size} differs from golden {len(golden)}")
    (out / "golden.bin").write_bytes(golden)
    actual = out / "actual.bin"
    state_file = out / "run-state.json"
    manifest = out / "jobs.txt"
    events = prepare_dispatch(symbols, manifest) if args.runtime else []
    options = ["--hscb-jobs", str(manifest)] if args.runtime else ["--test-finisher", "true"]
    command = [str(model), "-f", str(elf), "--msgb", *options,
               "--dump-memory", f"0x{address:x}:{size}:{actual}", "--dump-run-state", str(state_file),
               "-m", "20000",
               "-s", "softcore.multiThreadNum=2"]
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
    report["two_active_pes"] = ([t["thread"] for t in threads] == [0, 1] and
                                all(t["retired_micro_instructions"] > 0 for t in threads))
    report["finisher"] = state.get("test_finisher_reached", False)
    if args.runtime:
        lines = (out / "gfrun.log").read_text().splitlines()
        report["scheduler"] = [line for line in lines if line.startswith("HSCB ")]
        events_match = all(
            [line for line in lines if re.match(fr"^HSCB \w+ pe={pe} ", line)] == expected
            for pe, expected in enumerate(events))
        report["parked"] = report["two_active_pes"] and all(
            not t["sim_end"] and t["message_wait"] and t["message_parks"] == 2 and
            t["message_wakes"] == 1 for t in threads)
        completed = (events_match and lines.count("HSCB IDLE workers=2 completed=2") == 1
                     and report["parked"] and not report["finisher"])
    else:
        completed = report["finisher"]
    report["pass"] = (report.get("returncode") == 0 and data == golden and
                      report["two_active_pes"] and completed)
    (out / "result.json").write_text(json.dumps(report, indent=2) + "\n")
    if not report["pass"]:
        raise SystemExit(f"FAIL: inspect {out / 'result.json'} and {out / 'gfrun.log'}")
    print(f"PASS: 9 bytes PE0→PE1, 80 bytes PE1→PE0, all {size} dump bytes match ({out})")


if __name__ == "__main__":
    main()

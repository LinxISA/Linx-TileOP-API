#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github/workflows/pr-gate.yml"


def workflow_run_block(step_name: str) -> str:
    lines = WORKFLOW.read_text(encoding="utf-8").splitlines()
    name_line = f"      - name: {step_name}"
    start = lines.index(name_line)
    run = next(index for index in range(start + 1, len(lines))
               if lines[index] == "        run: |")
    block = []
    for line in lines[run + 1:]:
        if line.startswith("      - name:"):
            break
        block.append(line[10:] if line.startswith("          ") else line)
    return "\n".join(block).rstrip() + "\n"


class PRGateCacheOrderTest(unittest.TestCase):
    def test_automerge_skips_draft_pull_requests(self):
        workflow = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn(
            "if: github.event_name == 'pull_request' && "
            "github.event.pull_request.draft == false",
            workflow,
        )

    def test_candidate_overlay_happens_after_clean_cache_save(self):
        workflow = WORKFLOW.read_text(encoding="utf-8")
        save = workflow.index("      - name: Save LLVM build cache")
        overlay = workflow.index(
            "      - name: Replace TileOP source with candidate content"
        )
        install = workflow.index("      - name: Install candidate TileOP headers")
        parse = workflow.index("      - name: Parse template_asm.hpp with Linx Clang")
        self.assertLess(save, overlay)
        self.assertLess(overlay, install)
        self.assertLess(install, parse)

        install_block = workflow_run_block("Install candidate TileOP headers")
        self.assertIn('CANDIDATE_SHA="$(git -C tileop-api rev-parse HEAD)"',
                      install_block)
        self.assertIn("TILEOP_API_REVISION_IS_EXACT=1", install_block)
        self.assertIn("pto_tile_region.hpp", install_block)
        self.assertIn("template_asm.hpp", install_block)

    def test_dirty_cached_api_is_preserved_before_init_src(self):
        init_block = workflow_run_block("Initialize component sources")
        quarantine = init_block.split("make init-src", 1)[0]

        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            toolchain = temp / "toolchain"
            cached_api = toolchain / "src/Linx-TileOP-API"
            runner_temp = temp / "runner-temp"
            cached_api.mkdir(parents=True)
            runner_temp.mkdir()

            subprocess.run(["git", "init", "-q"], cwd=cached_api, check=True)
            tracked = cached_api / "candidate.hpp"
            tracked.write_text("baseline\n", encoding="utf-8")
            subprocess.run(["git", "add", "candidate.hpp"],
                           cwd=cached_api, check=True)
            subprocess.run(
                ["git", "-c", "user.name=CI", "-c", "user.email=ci@example.com",
                 "-c", "commit.gpgsign=false", "commit", "-q", "-m", "baseline"],
                cwd=cached_api,
                check=True,
            )
            tracked.write_text("candidate overlay\n", encoding="utf-8")

            env = os.environ.copy()
            env["RUNNER_TEMP"] = str(runner_temp)
            subprocess.run(
                ["bash", "-c", quarantine],
                cwd=toolchain,
                env=env,
                check=True,
            )

            preserved = runner_temp / "cached-Linx-TileOP-API/candidate.hpp"
            self.assertFalse(cached_api.exists())
            self.assertEqual(preserved.read_text(encoding="utf-8"),
                             "candidate overlay\n")


if __name__ == "__main__":
    unittest.main()

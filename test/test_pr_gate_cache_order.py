#!/usr/bin/env python3
import os
import re
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github/workflows/pr-gate.yml"
SHARED_ACTION = ROOT / ".github/actions/prepare-toolchain/action.yml"


def workflow_run_block(step_name: str) -> str:
    lines = SHARED_ACTION.read_text(encoding="utf-8").splitlines()
    lines = ["  " + line for line in lines]
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

    def test_element_objects_enable_generic_lowering_only(self):
        compile_all = (ROOT / "test/tileop_api/compile.all").read_text(
            encoding="utf-8"
        )
        self.assertIn('if [[ "$testcase" == Element* ]]', compile_all)
        self.assertIn("-linxv5-enable-generic-element=true", compile_all)
        self.assertNotIn(
            'CC_OPTS="${CC_OPTS:-} -resource-dir $RESOURCE_OVERLAY '
            '-mllvm -linxv5-enable-generic-element=true"',
            compile_all,
        )

    def test_candidate_overlay_happens_after_clean_cache_save(self):
        workflow = SHARED_ACTION.read_text(encoding="utf-8")
        workflow = "\n".join("  " + line for line in workflow.splitlines())
        save = workflow.index("      - name: Save LLVM build cache")
        overlay = workflow.index(
            "      - name: Replace TileOP source with candidate content"
        )
        install = workflow.index("      - name: Install candidate TileOP headers")
        parse = WORKFLOW.read_text(encoding="utf-8").index("      - name: Parse template_asm.hpp with Linx Clang")
        self.assertLess(save, overlay)
        self.assertLess(overlay, install)
        gate = WORKFLOW.read_text(encoding="utf-8")
        self.assertLess(gate.index("uses: ./tileop-api/.github/actions/prepare-toolchain"), parse)

        install_block = workflow_run_block("Install candidate TileOP headers")
        self.assertIn('CANDIDATE_SHA="$(git -C tileop-api rev-parse HEAD)"',
                      install_block)
        self.assertIn("TILEOP_API_REVISION_IS_EXACT=1", install_block)
        self.assertIn("pto_tile_region.hpp", install_block)
        self.assertIn("template_asm.hpp", install_block)

    def test_cached_sdk_uses_exact_component_and_environment_identity(self):
        action = SHARED_ACTION.read_text()
        restore = action.split('    - name: Restore complete ABI-scoped toolchain cache\n', 1)[1].split('    - name:', 1)[0]
        save = action.split('    - name: Save LLVM build cache\n', 1)[1].split('    - name:', 1)[0]
        key = lambda block: re.search(r'^        key: (.*)$', block, re.M)[1]
        self.assertEqual(key(restore), key(save))
        self.assertNotIn('restore-keys', restore)
        self.assertIn('steps.host-abi.outputs.key', key(restore))
        self.assertIn('steps.llvm-source.outputs.content', key(restore))
        for block in (restore, save):
            self.assertIn('path: toolchain/output/linx_blockisa_llvm_musl', block)

    def test_source_pins_determine_cache_identity(self):
        # Run the actual resolver with fixed source revisions: a libc-only update must
        # invalidate the SDK even when LLVM has not changed.
        script = workflow_run_block('Resolve component revisions and cache identity')
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            output = temp / 'outputs'
            env = dict(os.environ, GITHUB_OUTPUT=str(output),
                       LLVM_REPO='unused', LLVM_REF='0.59',
                       LLVM_PIN='1'*40, MUSL_PIN='2'*40, LINUX_PIN='3'*40,
                       JEMALLOC_PIN='4'*40, TOOLCHAIN_REF='5'*40,
                       TOOLCHAIN_PROFILE='compile-runtime')
            # Workflow paths are relative to a checkout named tileop-api.
            (temp / 'tileop-api').symlink_to(ROOT, target_is_directory=True)
            def resolve():
                output.write_text('')
                subprocess.run(['bash', '-euo', 'pipefail', '-c', script],
                               cwd=temp, env=env, check=True,
                               capture_output=True, text=True)
                return dict(line.split('=', 1) for line in output.read_text().splitlines())
            first = resolve()
            self.assertEqual(first, resolve())
            env['MUSL_PIN'] = '6'*40
            second = resolve()
            self.assertEqual(first['sha'], second['sha'])
            self.assertNotEqual(first['content'], second['content'])
            env['TOOLCHAIN_PROFILE'] = 'full'
            self.assertNotEqual(second['content'], resolve()['content'])

    def test_gate_requires_compile_success_without_model_dependency(self):
        workflow = WORKFLOW.read_text()
        gate = workflow.split('  gate:\n', 1)[1].split('  automerge:\n', 1)[0]
        self.assertIn('needs: compile', gate)
        self.assertIn('if: always()', gate)
        self.assertNotIn('SuperScalarModel', workflow)
        self.assertNotIn('SSM_READ_TOKEN', workflow)
        self.assertNotIn('needs.multi-pe', workflow)
        script = gate.split('        run: |\n', 1)[1]
        for status in ('success', 'failure', 'cancelled', 'skipped'):
            with self.subTest(status=status):
                result = subprocess.run(
                    ['bash', '-e', '-c', script],
                    env=dict(os.environ, COMPILE_RESULT=status))
                self.assertEqual(result.returncode == 0, status == 'success')
        build = workflow_run_block('Build toolchain on cache miss')
        self.assertNotIn('retrying', build)
        self.assertNotIn('rm -rf', build)


if __name__ == "__main__":
    unittest.main()

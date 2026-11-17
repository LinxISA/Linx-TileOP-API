#!/usr/bin/env python3
"""Compile public MX overloads and check their operation-owned source roles."""
import re
import unittest

import test_llvm138_narrow_tcmp as target


class MXOperandOrderTest(unittest.TestCase):
    setUpClass = classmethod(target.LLVM138NarrowTCMPTest.setUpClass.__func__)
    compile = target.LLVM138NarrowTCMPTest.compile

    def test_public_overload_source_streams(self):
        source = ['#include <common/pto_tileop.hpp>\nusing namespace pto;']
        cases = {}
        for storage in ('local', 'shared_b', 'shared_ab'):
            for mask in range(4):
                for op in ('plain', 'acc', 'bias'):
                    modes = ('basic', 'options', 'rowmax')
                    if op == 'acc':
                        modes += ('cscale',)
                    if storage == 'shared_b' and mask == 3:
                        modes += ('groupm', 'groupm_options')
                    for mode in modes:
                        name = f'mx_{storage}_{mask}_{op}_{mode}'
                        shared_a = storage == 'shared_ab'
                        shared_b = storage != 'local'
                        at = '__fp8_e4m3' if mask & 1 else '__half'
                        bt = '__fp8_e5m2' if mask & 2 else '__half'
                        types = {
                            'A': (f'CubeTileM16<{at},16,32>',
                                  f'Tile<Location::Left,{at},16,32>', shared_a),
                            'SA': ('Tile<Location::Scaling,__fp8_e8m0,32,4,BLayout::CubeM32,16,1>',
                                   'Tile<Location::Scaling,__fp8_e8m0,16,1>', shared_a),
                            'B': (f'CubeTileN8<{bt},32,8>',
                                  f'Tile<Location::Right,{bt},8,32>', shared_b),
                            'SB': ('Tile<Location::Scaling,__fp8_e8m0,32,4,BLayout::CubeM32,8,1>',
                                   'Tile<Location::Scaling,__fp8_e8m0,8,1>', shared_b),
                            'C': ('CubeAccumulatorM16<float,16,8>', '', False),
                            'Bias': ('CubeBias<float,8>', '', False),
                            'CS': ('Tile<Location::Scaling,uint8_t,32,4,BLayout::CubeM32,16,1>', '', False),
                        }
                        roles = ['A'] + (['SA'] if mask & 1 else [])
                        roles += ['B'] + (['SB'] if mask & 2 else [])
                        if op == 'acc':
                            roles.insert(0, 'C')
                        if op == 'bias':
                            roles.append('Bias')
                        if mode == 'cscale':
                            roles.append('CS')
                        source.append(f'extern "C" void {name}() {{')
                        source.append('CubeAccumulatorM16<float,16,8> d;')
                        source.append('CubeAccumulatorM16<float,16,1> row;')
                        for role in roles:
                            local, shared, is_shared = types[role]
                            tile = f'SharedTile<{shared}>' if is_shared else local
                            accessor = 'handle_ref()' if is_shared else 'data()'
                            constraint = 'Sr' if is_shared else 'Tr'
                            # Distinct SSA producers let the test resolve each
                            # generated asm operand back to its public API role.
                            # These markers are used for IR inspection only.
                            source.append(f'{tile} {role};')
                            source.append(f'asm volatile("// mx-role {role}" : '
                                          f'"={constraint}"({role}.{accessor}));')
                        args = ['d'] + (['C'] if op == 'acc' else []) + ['A']
                        args += ['SA'] if mask & 1 else []
                        args += ['B'] + (['SB'] if mask & 2 else [])
                        args += ['Bias'] if op == 'bias' else []
                        if mode not in ('basic', 'groupm'):
                            options = 'fixp::keep_acc()'
                            if mode == 'cscale':
                                options += '.cscale(CS)'
                            if mode == 'rowmax':
                                options += '.row_max(row)'
                            args.append(options)
                        if mode.startswith('groupm'):
                            args.append('64')
                        api = {'plain': 'TMATMUL_MX', 'acc': 'TMATMUL_MX_ACC',
                               'bias': 'TMATMUL_MX_BIAS'}[op]
                        source.append(f'{api}({", ".join(args)});\n}}')
                        local = [r for r in roles if not types[r][2]]
                        shared = [r for r in roles if types[r][2]]
                        cases[name] = local, shared
        path = self.output / 'mx-order.cpp'
        path.write_text('\n'.join(source))
        result = self.compile('-S', '-emit-llvm', '-mllvm',
                              '-enable-all-vector-as-tilereg=true', str(path), '-o', '-')
        self.assertEqual(result.returncode, 0, result.stdout)
        bodies = dict(re.findall(r'^define [^\n]*@(\w+)\([^\n]*\{\n(.*?)^\}',
                                 result.stdout, re.M | re.S))
        for name, (expected_local, expected_shared) in cases.items():
            with self.subTest(case=name):
                body = bodies[name]
                roles = dict(re.findall(
                    r'(%[\w.]+) = .*?asm sideeffect "// mx-role (\w+)"', body))
                call = re.search(r'asm sideeffect "(BSTART.CUBE.*?)", "([^"]*)"\((.*?)\)', body)
                self.assertIsNotNone(call, body)
                template, constraints, arguments = call.groups()
                outputs = sum(c.startswith('=') for c in constraints.split(','))
                values = [None] * outputs + [a.rsplit(' ', 1)[-1]
                                              for a in arguments.split(', ')]
                # Only CScale conditionals remain after the dimension header.
                stream = template[template.rindex('->lb2') + len('->lb2'):]
                local, shared = [], []
                enabled = True
                for line in stream.split('\\0A'):
                    if line.startswith('.if '):
                        index = int(re.search(r'\$\{(\d+):c\}', line)[1])
                        enabled = values[index] not in ('false', '0')
                    elif line == '.else':
                        enabled = not enabled
                    elif line == '.endif':
                        enabled = True
                    elif enabled and line.startswith(('B.IOT ', 'B.IOS ')):
                        operands = line.split('mask=')[0]
                        for index in re.findall(r'\$(?:\{(\d+):S\}|(\d+))', operands):
                            value = values[int(index[0] or index[1])]
                            self.assertIn(value, roles, line)
                            (shared if line.startswith('B.IOS') else local).append(roles[value])
                self.assertEqual(local, expected_local, body)
                self.assertEqual(shared, expected_shared, body)

    def test_local_kernels_assemble(self):
        # Real producers exercise target allocation/encoding independently of
        # the IR markers above. Numerical execution needs a compatible model.
        source = '''#include <common/pto_tileop.hpp>
using namespace pto;
extern "C" void mx_kernel(__fp8_e4m3 *left, __fp8_e5m2 *right,
                          __fp8_e8m0 *scale_a, __fp8_e8m0 *scale_b,
                          float *output) {
  CubeTileM16<__fp8_e4m3,16,32> a;
  CubeTileN8<__fp8_e5m2,32,8> b;
  Tile<Location::Scaling,__fp8_e8m0,32,4,BLayout::CubeM32,16,1> sa;
  Tile<Location::Scaling,__fp8_e8m0,32,4,BLayout::CubeM32,8,1> sb;
  CubeAccumulatorM16<float,16,8> c,d;
  CubeAccumulatorM16<float,16,1> row;
  CubeBias<float,8> bias;
  Tile<Location::Scaling,uint8_t,32,4,BLayout::CubeM32,16,1> cs;
  global_tensor<__fp8_e4m3,RowMajor<16,32>> ma(left);
  global_tensor<__fp8_e5m2,RowMajor<32,8>> mb(right);
  global_tensor<__fp8_e8m0,RowMajor<16,1>> msa(scale_a);
  global_tensor<__fp8_e8m0,RowMajor<8,1>> msb(scale_b);
  global_tensor<float,RowMajor<16,8>> out(output);
  TLOAD(a,ma); TLOAD(b,mb); TLOAD(sa,msa); TLOAD(sb,msb);
  TEXPANDS(c,2.f); TEXPANDS(bias,3.f); TEXPANDS(cs,uint8_t(1));
  CALL;
  TSTORE(out,d);
}
'''
        for op, call in {
            'plain': 'TMATMUL_MX(d,a,sa,b,sb)',
            'acc': 'TMATMUL_MX_ACC(d,c,a,sa,b,sb)',
            'bias': 'TMATMUL_MX_BIAS(d,a,sa,b,sb,bias)',
            'cscale': 'TMATMUL_MX_ACC(d,c,a,sa,b,sb,fixp::keep_acc().cscale(cs))',
            'options': 'TMATMUL_MX_ACC(d,c,a,sa,b,sb,fixp::keep_acc())',
            'no_scale_rowmax': 'TMATMUL_MX_ACC(d,c,a,b,fixp::keep_acc().row_max(row))',
        }.items():
            with self.subTest(op=op):
                path = self.output / f'mx-{op}.cpp'
                text = source.replace('CALL', call)
                if op == 'no_scale_rowmax':
                    text = text.replace('__fp8_e4m3', '__half').replace('__fp8_e5m2', '__half')
                path.write_text(text)
                result = self.compile('-c', '-mllvm', '-enable-all-vector-as-tilereg=true',
                                      str(path), '-o', str(path.with_suffix('.o')))
                self.assertEqual(result.returncode, 0, result.stdout)


if __name__ == '__main__':
    unittest.main()

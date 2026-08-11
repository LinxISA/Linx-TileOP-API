# Matrix post-processing compatibility surface

The active CUBE contract defines twelve named matrix operations and no separate
post-processing attribute command. The historical fixed-point wrapper names are
retained only to produce a compile-time diagnostic; they do not emit an
instruction bundle.

Requirements:

- Use one of the twelve named matrix operations for executable code.
- Use only default matrix attributes; non-default post-processing options fail
  during template instantiation.
- Shared matrix sources also fail closed until the compiler exposes a unique
  source-selection contract.

No compatibility wrapper may synthesize a removed machine operation or silently
drop a requested attribute.

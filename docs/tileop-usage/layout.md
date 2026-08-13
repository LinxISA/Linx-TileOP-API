# Layout helpers

`TMOV` layout helpers convert between supported tile layouts without
introducing a separate engine class. They are TLSU operations and use
`B.DATR` to select the source and destination layout. The PTO-ISA v0.58
catalog classifies `TMOV` as layout-and-rearrangement while executing it on
the TLSU engine, and additionally assigns TLSU functions 9..12 for the Shared
Local-to-Shared / Shared-to-Local forms.

`TTRANS`, `TFILLPAD`, extraction, insertion, and concatenation are SFU
operations because their hardware is more complex than elementwise VEC
execution. The canonical classification is generated in
[engines.md](engines.md).

The C++ helper `TReshape` is a host/API shape-copy utility and is not an ISA
mnemonic (the reshape operation was removed from the catalog before v0.58).
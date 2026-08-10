# Layout helpers

`TMOV` layout helpers convert between supported tile layouts without introducing a separate engine
class. They are TLSU operations and use `B.DATR` to select the source and destination layout.

`TTRANS`, `TFILLPAD`, extraction, insertion, and concatenation are SFU operations because their
hardware is more complex than elementwise VEC execution. The canonical classification is generated
in [engines.md](engines.md).

The C++ helper `TReshape` is a host/API shape-copy utility and is not an ISA mnemonic.

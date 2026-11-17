OUTPUT_ARCH(linxv5)
ENTRY(_start)
PHDRS { text PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); }
SECTIONS {
  . = 0x10000 + SIZEOF_HEADERS;
  .note.pto.isa : ALIGN(4) { KEEP(*(.note.pto.isa)) } :text
  .text : ALIGN(16) { *(.text .text.*) } :text
  .rodata : ALIGN(16) { *(.rodata .rodata.* .srodata .srodata.*) } :text
  . = ALIGN(4096);
  .data : { __DATA_BEGIN__ = .; *(.data .data.* .sdata .sdata.*) } :data
  .bss (NOLOAD) : { *(.bss .bss.* .sbss .sbss.* COMMON) } :data
  __global_pointer$ = __DATA_BEGIN__ + 0x800;
  _end = .;
}

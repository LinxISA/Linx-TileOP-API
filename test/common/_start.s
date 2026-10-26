# Legacy direct-entry fixture. It is intentionally not linked by the current
# gfrun benchmark path; use gfrun_runtime_adapter.c for hosted completion.
.global  _start
  .type   _start,@function
  .text
_start:
  bstart.std call main
  c.setret 2, ->ra
_end:
  bstart.aux fall
  acrc 3
  c.bstop

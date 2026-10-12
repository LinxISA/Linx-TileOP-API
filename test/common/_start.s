.global  _start
  .type   _start,@function
  .text
_start:
	# This test entry point starts one context. A four-PE launcher must provide
	# its own strong __pto_runtime_pe_context_count returning 4 and establish
	# PE IDs 0..3 before invoking code that calls SYNC_CORE.
  bstart.std call main
  c.setret 2, ->ra
_end:
  bstart.aux fall
  acrc 3
  c.bstop

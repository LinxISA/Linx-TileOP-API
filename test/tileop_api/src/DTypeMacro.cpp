// Issue #28: common consumer macros must not rewrite inline-asm labels.
// The test intentionally does not instantiate TQUANT; parsing the public
// header is sufficient to catch a [DType] symbolic-label collision.
#include <common/pto_tileop.hpp>

int main() { return 0; }

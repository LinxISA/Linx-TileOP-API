#include <common/pto_tileop.hpp>

uint32_t read_pe_id() { return get_thread_idx(); }
uint32_t read_pe_id_alias() { return get_thread_id(); }

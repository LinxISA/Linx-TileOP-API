#ifndef PTO_TILEOP_API_REVISION_HPP
#define PTO_TILEOP_API_REVISION_HPP

#define PTO_TILEOP_API_VERSION "0.58.6"
#define PTO_TILEOP_API_SPEC_VERSION "0.58.6"

// Source-tree consumers do not necessarily build from a Git checkout. The
// install target replaces these two macros with the exact checked-out commit.
#define PTO_TILEOP_API_REVISION "source-tree"
#define PTO_TILEOP_API_REVISION_IS_EXACT 0

// Feature gates let consumers fail fast without comparing revision strings.
#define PTO_TILEOP_API_HAS_LOCAL_B_KN_FIX 1

#endif

#ifndef PTO_ROUNDING_HPP
#define PTO_ROUNDING_HPP

namespace pto {
// Public bundle/API selector namespace, distinct from scalar CORE_STATE.FRM.
enum LinxRMode {
  LINX_RNONE = 0,
  LINX_RNE = 1,
  LINX_RTZ = 2,
  LINX_RDN = 3,
  LINX_RUP = 4,
  LINX_RNA = 5,
  LINX_RTO = 6,
  LINX_RHB = 7,
};
} // namespace pto
#endif

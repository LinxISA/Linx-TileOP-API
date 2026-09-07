// Per-dimension four-combination sweep: every four-branch B.DIM interface
// instantiated with all ValidCol/ValidRow combinations (SS/SD/DS/DD).
//   SS -> C.B.DIMI lb0 + C.B.DIMI lb1
//   SD -> C.B.DIMI lb0 + B.DIM reg,0 lb1
//   DS -> B.DIM reg,0 lb0 + C.B.DIMI lb1
//   DD -> B.DIM reg,0 lb0 + B.DIM reg,0 lb1
// lb2 always stays immediate-form (C.B.DIMI).
#include <jcore/template_asm.hpp>

using namespace pto;

template <int VR,int VC> using T =
  Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, VR, VC>;

#define SWEEP3(FN, TY) \
  __attribute__((noinline)) void FN##_##TY(T<TY##_r, TY##_c> &a, T<TY##_r, TY##_c> &b, T<TY##_r, TY##_c> &c){FN(a,b,c);}
// 简化: 每组合独立函数
#define SWEEP2(FN) \
  __attribute__((noinline)) void FN##_ss(T<16,32> &a,T<16,32> &b,T<16,32> &c){FN(a,b,c);} \
  __attribute__((noinline)) void FN##_sd(T<16,-1> &a,T<16,-1> &b,T<16,-1> &c){FN(a,b,c);} \
  __attribute__((noinline)) void FN##_ds(T<-1,32> &a,T<-1,32> &b,T<-1,32> &c){FN(a,b,c);} \
  __attribute__((noinline)) void FN##_dd(T<-1,-1> &a,T<-1,-1> &b,T<-1,-1> &c){FN(a,b,c);}

SWEEP2(TADD) SWEEP2(TSUB) SWEEP2(TMUL) SWEEP2(TDIV) SWEEP2(TMAX) SWEEP2(TMIN)
SWEEP2(TAND) SWEEP2(TOR) SWEEP2(TXOR) SWEEP2(TSHL) SWEEP2(TSHR) SWEEP2(TREM)



template <int VR,int VC> using T =
  Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, VR, VC>;

#define SWEEP1(FN) \
  __attribute__((noinline)) void FN##_ss(T<16,32> &a,T<16,32> &b){FN(a,b);} \
  __attribute__((noinline)) void FN##_sd(T<16,-1> &a,T<16,-1> &b){FN(a,b);} \
  __attribute__((noinline)) void FN##_ds(T<-1,32> &a,T<-1,32> &b){FN(a,b);} \
  __attribute__((noinline)) void FN##_dd(T<-1,-1> &a,T<-1,-1> &b){FN(a,b);}

SWEEP1(TABS) SWEEP1(TEXP) SWEEP1(TLOG) SWEEP1(TNEG) SWEEP1(TNOT)
SWEEP1(TRECIP) SWEEP1(TRELU) SWEEP1(TRSQRT) SWEEP1(TSQRT)

SWEEP1(TCVT_T) SWEEP1(TMOV) SWEEP1(TTRANS)


// TTRI(dst) single-arg
__attribute__((noinline)) void ttri_ss(T<16,32> &a){TTRI(a);}
__attribute__((noinline)) void ttri_sd(T<16,-1> &a){TTRI(a);}
__attribute__((noinline)) void ttri_ds(T<-1,32> &a){TTRI(a);}
__attribute__((noinline)) void ttri_dd(T<-1,-1> &a){TTRI(a);}

// row-reduce: dst must be N x 1 (Cols==1, so only ValidRow varies)
template <int VR,int VC> using R1 =
  Tile<Location::Vec, float, 32, 1, BLayout::RowMajor, VR, VC>;
#define SWEEPR(FN) \
  __attribute__((noinline)) void FN##_ss(R1<16,1> &a,R1<16,1> &b){FN(a,b);} \
  __attribute__((noinline)) void FN##_sd(R1<16,-1> &a,R1<16,-1> &b){FN(a,b);} \
  __attribute__((noinline)) void FN##_ds(R1<-1,1> &a,R1<-1,1> &b){FN(a,b);} \
  __attribute__((noinline)) void FN##_dd(R1<-1,-1> &a,R1<-1,-1> &b){FN(a,b);}
SWEEPR(TROWMAX) SWEEPR(TROWMIN) SWEEPR(TROWPROD) SWEEPR(TROWSUM)

template <int VR,int VC> using T =
  Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, VR, VC>;
template <int VR,int VC> using TQ =
  Tile<Location::Vec, int8_t, 32, 32, BLayout::RowMajor, VR, VC>;

#define SWEEPS(FN) \
  __attribute__((noinline)) void FN##_ss(T<16,32> &a,T<16,32> &b){FN(a,b,1.0f);} \
  __attribute__((noinline)) void FN##_sd(T<16,-1> &a,T<16,-1> &b){FN(a,b,1.0f);} \
  __attribute__((noinline)) void FN##_ds(T<-1,32> &a,T<-1,32> &b){FN(a,b,1.0f);} \
  __attribute__((noinline)) void FN##_dd(T<-1,-1> &a,T<-1,-1> &b){FN(a,b,1.0f);}
SWEEPS(TADDS) SWEEPS(TSUBS) SWEEPS(TMULS) SWEEPS(TDIVS) SWEEPS(TREMS)
SWEEPS(TANDS) SWEEPS(TORS) SWEEPS(TXORS) SWEEPS(TSHLS) SWEEPS(TSHRS)
SWEEPS(TMAXS) SWEEPS(TMINS) SWEEPS(TCMPS)

#define SWEEPQ(FN) \
  __attribute__((noinline)) void FN##_ss(TQ<16,32> &d,T<16,32> &s){FN(d,s,1.0f,2);} \
  __attribute__((noinline)) void FN##_sd(TQ<16,-1> &d,T<16,-1> &s){FN(d,s,1.0f,2);} \
  __attribute__((noinline)) void FN##_ds(TQ<-1,32> &d,T<-1,32> &s){FN(d,s,1.0f,2);} \
  __attribute__((noinline)) void FN##_dd(TQ<-1,-1> &d,T<-1,-1> &s){FN(d,s,1.0f,2);}
SWEEPQ(TQUANT)


// TSELS(dst, src0, s, src1)
__attribute__((noinline)) void tsels_ss(T<16,32>&a,T<16,32>&b,T<16,32>&c){TSELS(a,b,1.0f,c);}
__attribute__((noinline)) void tsels_sd(T<16,-1>&a,T<16,-1>&b,T<16,-1>&c){TSELS(a,b,1.0f,c);}
__attribute__((noinline)) void tsels_ds(T<-1,32>&a,T<-1,32>&b,T<-1,32>&c){TSELS(a,b,1.0f,c);}
__attribute__((noinline)) void tsels_dd(T<-1,-1>&a,T<-1,-1>&b,T<-1,-1>&c){TSELS(a,b,1.0f,c);}

template <int VR,int VC> using T =
  Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, VR, VC>;
template <int VR,int VC> using TCUBE =
  Tile<Location::Vec, float, 16, 64, BLayout::CubeM16, VR, VC, SLayout::NoneBox, 512, PadValue::Null, CompactMode::Null>;

// TCMP needs CmpMode template arg
#define SWEEPC(FN) \
  __attribute__((noinline)) void FN##_ss(T<16,32> &a,T<16,32> &b,T<16,32> &c){FN<CmpMode::EQ>(a,b,c);} \
  __attribute__((noinline)) void FN##_sd(T<16,-1> &a,T<16,-1> &b,T<16,-1> &c){FN<CmpMode::EQ>(a,b,c);} \
  __attribute__((noinline)) void FN##_ds(T<-1,32> &a,T<-1,32> &b,T<-1,32> &c){FN<CmpMode::EQ>(a,b,c);} \
  __attribute__((noinline)) void FN##_dd(T<-1,-1> &a,T<-1,-1> &b,T<-1,-1> &c){FN<CmpMode::EQ>(a,b,c);}
SWEEPC(TCMP)

// THISTOGRAM(dst, src, Idx, ByteId)
#define SWEEPH(FN) \
  __attribute__((noinline)) void FN##_ss(T<16,32> &a,T<16,32> &b,T<16,32> &c){FN(a,b,c,0);} \
  __attribute__((noinline)) void FN##_sd(T<16,-1> &a,T<16,-1> &b,T<16,-1> &c){FN(a,b,c,0);} \
  __attribute__((noinline)) void FN##_ds(T<-1,32> &a,T<-1,32> &b,T<-1,32> &c){FN(a,b,c,0);} \
  __attribute__((noinline)) void FN##_dd(T<-1,-1> &a,T<-1,-1> &b,T<-1,-1> &c){FN(a,b,c,0);}
SWEEPH(THISTOGRAM)

// TLOAD_CUBE / TSTORE_CUBE
using gmc = GlobalTensor<float, Shape<1,1,1,16,64>, Stride<1,1,16*64,64,1>, Layout::ND>;
__attribute__((noinline)) void tlcb_ss(gmc &g, TCUBE<16,64> &d){TLOAD_CUBE(d,g);}
__attribute__((noinline)) void tlcb_sd(gmc &g, TCUBE<16,-1> &d){TLOAD_CUBE(d,g);}
__attribute__((noinline)) void tlcb_ds(gmc &g, TCUBE<-1,64> &d){TLOAD_CUBE(d,g);}
__attribute__((noinline)) void tlcb_dd(gmc &g, TCUBE<-1,-1> &d){TLOAD_CUBE(d,g);}
__attribute__((noinline)) void tscb_ss(gmc &g, TCUBE<16,64> &d){TSTORE_CUBE(g,d);}
__attribute__((noinline)) void tscb_sd(gmc &g, TCUBE<16,-1> &d){TSTORE_CUBE(g,d);}
__attribute__((noinline)) void tscb_ds(gmc &g, TCUBE<-1,64> &d){TSTORE_CUBE(g,d);}
__attribute__((noinline)) void tscb_dd(gmc &g, TCUBE<-1,-1> &d){TSTORE_CUBE(g,d);}



template <int VR,int VC> using T = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, VR, VC>;



int main() { return 0; }

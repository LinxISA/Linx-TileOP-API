#!/usr/bin/env bash
# Build only the SDK components exercised by the required gate. The macro
# coverage workflow keeps the full SDK profile for benchmark link checks.
set -euo pipefail
root="$(cd "$1" && pwd)"
llvm="$root/src/llvm-project"
install="$root/output/linx_blockisa_llvm_musl"
threads="${CMAKE_BUILD_PARALLEL_LEVEL:-4}"
export CCACHE_BASEDIR="$root"
export CCACHE_NOHASHDIR=true
export PATH="/usr/lib/ccache:$PATH"

if [ "${TOOLCHAIN_PROFILE:-full}" = full ]; then
  make -C "$root" WITH_TARGET=linx64v5-linux-musl ENABLE_CCACHE=on \
    THREADS="$threads" LLVM_MAKE="ninja -j$threads"
  exit 0
fi

mkdir -p "$install"
cmake -S "$llvm/llvm" -B "$root/build/build-llvm-musl" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$install" \
  -DLLVM_ENABLE_PROJECTS='clang;lld' -DLLVM_TARGETS_TO_BUILD=LinxV5 \
  -DLLVM_DEFAULT_TARGET_TRIPLE=linx64v5-linux-musl \
  -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_CCACHE_BUILD=ON \
  -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLINX_USE_LIBC=musl
# Avoid building unrelated LLVM utilities. The gate needs all listed tools.
cmake --build "$root/build/build-llvm-musl" --parallel "$threads" --target \
  clang lld llvm-ar llvm-ranlib llvm-nm llvm-mc llvm-objdump llvm-readobj \
  llvm-config clang-resource-headers
for component in clang lld llvm-ar llvm-ranlib llvm-nm llvm-mc llvm-objdump llvm-readobj llvm-config clang-resource-headers; do
  cmake --install "$root/build/build-llvm-musl" --component "$component"
done
for tool in clang clang++; do
  ln -sf "$tool" "$install/bin/linx64v5-linux-musl-$tool"
done
# Reject an incompatible compiler before starting any target runtime build.
"$install/bin/clang" --target=linx64v5-linux-musl \
  -mlinx-runtime-intra-abi -fsyntax-only -x c /dev/null

make -C "$root/src/linux-linxisa" headers_install ARCH=linx \
  INSTALL_HDR_PATH="$install/sysroot/usr"
mkdir -p "$root/build/build-musl"
(
  cd "$root/build/build-musl"
  "$root/src/musl/configure" --host=linx64v5-linux-musl --disable-shared \
    --prefix="$install/sysroot/usr" \
    CC="$install/bin/clang --target=linx64v5-linux-musl -mcpu=janus -mlinx-single-layer -fno-pie" \
    AR="$install/bin/llvm-ar" RANLIB="$install/bin/llvm-ranlib"
  make -j"$threads"
  # syslibdir normally defaults to /lib; keep every installed file inside SDK.
  make install syslibdir="$install/sysroot/usr/lib"
)

runtime_flags="-O2 -nostdlib -mcpu=janus -mlinx-single-layer -mlinx-runtime-intra-abi --sysroot=$install/sysroot"
cmake -S "$llvm/compiler-rt" -B "$root/build/build-compiler-rt-musl" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=linx64v5 \
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
  -DCMAKE_C_COMPILER="$install/bin/clang" -DCMAKE_CXX_COMPILER="$install/bin/clang++" \
  -DCMAKE_ASM_COMPILER="$install/bin/clang" \
  -DCMAKE_AR="$install/bin/llvm-ar" -DCMAKE_RANLIB="$install/bin/llvm-ranlib" \
  -DCMAKE_C_COMPILER_TARGET=linx64v5-linux-musl \
  -DCMAKE_CXX_COMPILER_TARGET=linx64v5-linux-musl \
  -DCMAKE_ASM_COMPILER_TARGET=linx64v5-linux-musl \
  -DLLVM_CONFIG_PATH="$root/build/build-llvm-musl/bin/llvm-config" \
  -DCMAKE_C_FLAGS="$runtime_flags" -DCMAKE_CXX_FLAGS="$runtime_flags" \
  -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON -DCOMPILER_RT_BUILD_BUILTINS=ON \
  -DCOMPILER_RT_BUILD_SANITIZERS=OFF -DCOMPILER_RT_BUILD_XRAY=OFF \
  -DCOMPILER_RT_BUILD_LIBFUZZER=OFF -DCOMPILER_RT_BUILD_PROFILE=OFF \
  -DCOMPILER_RT_EXCLUDE_ATOMIC_BUILTIN=ON -DCOMPILER_RT_BUILTINS_HIDE_SYMBOLS=OFF
cmake --build "$root/build/build-compiler-rt-musl" --parallel "$threads" --target builtins
resource="$("$install/bin/clang" -print-resource-dir)"
mkdir -p "$resource/lib/linux"
cp "$root/build/build-compiler-rt-musl/lib/linux/libclang_rt.builtins-linx64v5.a" "$resource/lib/linux/"

# Existing gate tests compile C++ objects and syntax; they do not link libc++.
# Generate/install the real target headers without building unused C++ runtimes.
cmake -S "$llvm/libcxx" -B "$root/build/build-libcxx-headers" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_INSTALL_PREFIX="$install/sysroot/usr" \
  -DCMAKE_C_COMPILER="$install/bin/clang" -DCMAKE_CXX_COMPILER="$install/bin/clang++" \
  -DCMAKE_C_COMPILER_TARGET=linx64v5-linux-musl \
  -DCMAKE_CXX_COMPILER_TARGET=linx64v5-linux-musl \
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
  -DCMAKE_SYSROOT="$install/sysroot" -DLLVM_PATH="$llvm/llvm" \
  -DLIBCXX_CXX_ABI=libcxxabi -DLIBCXX_CXX_ABI_INCLUDE_PATHS="$llvm/libcxxabi/include" \
  -DLIBCXX_ENABLE_SHARED=OFF -DLIBCXX_ENABLE_STATIC=ON \
  -DLIBCXX_HAS_MUSL_LIBC=ON -DLIBCXX_ENABLE_FILESYSTEM=OFF -DLIBCXX_INCLUDE_TESTS=OFF \
  -DLIBCXX_INCLUDE_BENCHMARKS=OFF
cmake --build "$root/build/build-libcxx-headers" --parallel "$threads" --target install-cxx-headers

/*
 * config.h — Android shadow of engine-core/config.h
 * Included instead of engine-core/config.h because jni-bridge/ is first
 * in the include path via -iquote. Pulls in the real config then overrides
 * what Android/aarch64 cannot support.
 */
#include "../engine-core/config.h"

/* GMP unavailable on Android */
#undef HAVE_LIBGMP

/* No libcurl on Android — random.org dice source disabled */
#undef LIBCURL_PROTOCOL_HTTPS
#undef HAVE_LIBCURL

/* x86-specific SIMD — invalid on aarch64.
 * Undefining USE_SIMD_INSTRUCTIONS takes the plain malloc/free path in simd.h.
 * HAVE_NEON could be enabled later for an aarch64 SIMD optimisation pass. */
#undef USE_SIMD_INSTRUCTIONS
#undef HAVE_SSE
#undef USE_SSE2
#undef USE_AVX

/* No Python, SQLite, or audio at the C layer */
#undef HAVE_PYTHON
#undef HAVE_SQLITE
#undef HAVE_CANBERRA

/* ARM NEON SIMD: enabled via CMakeLists.txt target_compile_definitions.
 * USE_SIMD_INSTRUCTIONS, HAVE_NEON, USE_NEON are passed as -D flags.
 * Do not define them here to avoid double-definition warnings. */

/* Disable GLib assertions — alignment asserts in NeuralNetEvaluateSSE
 * fire on aarch64 due to VLA alignment not guaranteed by clang.
 * vld1q_f32/vst1q_f32 handle unaligned access on aarch64 correctly. */
#define G_DISABLE_ASSERT 1

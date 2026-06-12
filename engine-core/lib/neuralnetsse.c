/*
 * Copyright (C) 2006-2009 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2007-2021 the AUTHORS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "common.h"

#if defined(USE_SIMD_INSTRUCTIONS)

#define DEBUG_SSE 0

#include "simd.h"
#include "neuralnet.h"
#include <string.h>

#if defined(USE_NEON)
#include <arm_neon.h>
#elif defined(USE_AVX)
#include <immintrin.h>
#elif defined(USE_SSE2)
#include <emmintrin.h>
#else
#include <xmmintrin.h>
#endif

#include <glib.h>
#include "sigmoid.h"

#if defined(HAVE_NEON)
#include <signal.h>
#include <setjmp.h>
#endif

float *
sse_malloc(size_t size)
{
#if defined(HAVE_POSIX_MEMALIGN)
    void *ptr = NULL;
    int ret;

    ret = posix_memalign(&ptr, ALIGN_SIZE, size);

    if (ret == 0)
        return (float *)ptr;

    /* mimic g_malloc() error message */
    g_error("%s: failed to allocate %"G_GSIZE_FORMAT" bytes", G_STRLOC, size);

    /* avoid warning: "control reaches end of non-void function" for
       compilers that don't understand that g_error() never returns. */
    return NULL;

#elif defined(HAVE__ALIGNED_MALLOC)
    return (float *) _aligned_malloc(size, ALIGN_SIZE);
#else
    return (float *) _mm_malloc(size, ALIGN_SIZE);
#endif
}

void
sse_free(float *ptr)
{
#if defined(HAVE_POSIX_MEMALIGN)
    free(ptr);
#elif defined(HAVE__ALIGNED_MALLOC)
    _aligned_free(ptr);
#else
    _mm_free(ptr);
#endif
}

#if defined(HAVE_NEON)
static jmp_buf env;

static void
my_handler(int status)
{
    longjmp(env, status);
}

int
CheckNEON(void)
{
    void (*oldsig)(int);

    oldsig = signal(SIGILL, my_handler);
    if (setjmp(env)) {
        signal(SIGILL, oldsig);
        return 0;
    } else {
#if defined(__ARM_FEATURE_SIMD32)
        /* v7 */
        __asm__ __volatile__ ("vmin.f32 q0, q0, q0");
#elif defined(__ARM_NEON)
        /* aarch64 */
        __asm__ __volatile__ ("fmin v0.4s, v0.4s, v0.4s");
#else
        #error "Unexpected ARM architecture"
#endif
        signal(SIGILL, oldsig);
        return 1;
    }
}
#endif

#if defined(USE_AVX) || defined(USE_SSE2) || defined(USE_NEON)
#include <stdint.h>

static const union {
    float f[VEC_SIZE];
    float_vector ps;
#if defined(USE_AVX)
} ones = { {
1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}};
#else
} ones = { {
1.0f, 1.0f, 1.0f, 1.0f}};
#endif

static const union {
    float f[VEC_SIZE];
    float_vector ps;
#if defined(USE_AVX)
} tens = { {
10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f}};
#else
} tens = { {
10.0f, 10.0f, 10.0f, 10.0f}};
#endif

static const union {
    int32_t i32[VEC_SIZE];
    float_vector ps;
#if defined(USE_AVX)
} abs_mask = { {
0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF}};
#else
} abs_mask = { {
0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF}};
#endif

static inline float_vector
sigmoid_positive_ps(float_vector xin)
{
    union {
        int_vector i;
        int32_t i32[VEC_SIZE];
    } i;
    float_vector ex;
    float *ex_elem = (float *) &ex;
#if defined(USE_AVX)
    float_vector x1 = _mm256_min_ps(xin, tens.ps);
#elif defined(HAVE_SSE)
    float_vector x1 = _mm_min_ps(xin, tens.ps);
#else
    float_vector x1 = vminq_f32(xin, tens.ps);
    float_vector rec;
#endif

#if defined(USE_AVX)
    x1 = _mm256_mul_ps(x1, tens.ps);
    i.i = _mm256_cvttps_epi32(x1);
#elif defined(HAVE_SSE)
    x1 = _mm_mul_ps(x1, tens.ps);
    i.i = _mm_cvttps_epi32(x1);
#else
    x1 = vmulq_f32(x1, tens.ps);
    i.i = vcvtq_s32_f32(x1);
#endif
    ex_elem[0] = e[i.i32[0]];
    ex_elem[1] = e[i.i32[1]];
    ex_elem[2] = e[i.i32[2]];
    ex_elem[3] = e[i.i32[3]];
#if defined(USE_AVX)
    ex_elem[4] = e[i.i32[4]];
    ex_elem[5] = e[i.i32[5]];
    ex_elem[6] = e[i.i32[6]];
    ex_elem[7] = e[i.i32[7]];
#endif

#if defined(USE_AVX)
    x1 = _mm256_sub_ps(x1, _mm256_cvtepi32_ps(i.i));
    x1 = _mm256_add_ps(x1, tens.ps);
#if defined(USE_FMA3)
    x1 = _mm256_fmadd_ps(x1, ex, ones.ps);
#else
    x1 = _mm256_mul_ps(x1, ex);
    x1 = _mm256_add_ps(x1, ones.ps);
#endif
#ifdef __FAST_MATH__
    return _mm256_rcp_ps(x1);
#else
    return _mm256_div_ps(ones.ps, x1);
#endif
#elif defined(HAVE_SSE)
    x1 = _mm_sub_ps(x1, _mm_cvtepi32_ps(i.i));
    x1 = _mm_add_ps(x1, tens.ps);
    x1 = _mm_mul_ps(x1, ex);
    x1 = _mm_add_ps(x1, ones.ps);
#ifdef __FAST_MATH__
    return _mm_rcp_ps(x1);
#else
    return _mm_div_ps(ones.ps, x1);
#endif
#else
    x1 = vsubq_f32(x1, vcvtq_f32_s32(i.i));
    x1 = vaddq_f32(x1, tens.ps);
    x1 = vmulq_f32(x1, ex);
    x1 = vaddq_f32(x1, ones.ps);

    /* TODO: Check how many Newton-Raphson iterations are needed to match x86 rcp and div accuracy */
#ifdef __FAST_MATH__
    rec = vrecpeq_f32(x1);
    return vmulq_f32(vrecpsq_f32(x1, rec), rec);
#else
    rec = vrecpeq_f32(x1);
    rec = vmulq_f32(vrecpsq_f32(x1, rec), rec);
    return vmulq_f32(vrecpsq_f32(x1, rec), rec);
#endif
#endif
}

static inline float_vector
sigmoid_ps(float_vector xin)
{
#if defined(USE_AVX)
    float_vector mask = _mm256_cmp_ps(xin, _mm256_setzero_ps(), _CMP_LT_OS);
    float_vector c;
    xin = _mm256_and_ps(xin, abs_mask.ps);      /* Abs. value by clearing signbit */
    c = sigmoid_positive_ps(xin);
    return _mm256_blendv_ps(c, _mm256_sub_ps(ones.ps, c), mask);
#elif defined(HAVE_SSE)
    float_vector mask = _mm_cmplt_ps(xin, _mm_setzero_ps());
    float_vector c;
    xin = _mm_and_ps(xin, abs_mask.ps); /* Abs. value by clearing signbit */
    c = sigmoid_positive_ps(xin);
    /* _mm_blendv_ps() is only available with SSE4.1 or later */
    return _mm_or_ps(_mm_andnot_ps(mask, c), _mm_and_ps(mask, _mm_sub_ps(ones.ps, c)));
#else
    int_vector mask = (int_vector)vcltq_f32(xin, vdupq_n_f32(0.0f));
    float_vector c;
    xin = (float_vector)vandq_s32((int_vector)xin, (int_vector)abs_mask.ps); /* Abs. value by clearing signbit */
    c = sigmoid_positive_ps(xin);
    return vbslq_f32((uint32x4_t)mask, vsubq_f32(ones.ps, c), c);
#endif
}

#endif                          // USE_SSE2 or USE_AVX

#if defined(USE_SSE2)
#define INPUT_ADD() \
for (j = (cHidden >> LOG2VEC_SIZE); j; j--, pr += VEC_SIZE, prWeight += VEC_SIZE) { \
    vec0 = _mm_load_ps(pr); \
    vec1 = _mm_load_ps(prWeight); \
    sum = _mm_add_ps(vec0, vec1); \
    _mm_store_ps(pr, sum); \
}
#define INPUT_MULTADD() \
for (j = (cHidden >> LOG2VEC_SIZE); j; j--, pr += VEC_SIZE, prWeight += VEC_SIZE) { \
    vec0 = _mm_load_ps(pr); \
    vec1 = _mm_load_ps(prWeight); \
    vec3 = _mm_mul_ps(vec1, scalevec); \
    sum = _mm_add_ps(vec0, vec3); \
    _mm_store_ps(pr, sum); \
}
#endif
#if defined(USE_AVX)
#define INPUT_ADD() \
for (j = (cHidden >> LOG2VEC_SIZE); j; j--, pr += VEC_SIZE, prWeight += VEC_SIZE) { \
    vec0 = _mm256_load_ps(pr); \
    vec1 = _mm256_load_ps(prWeight); \
    sum = _mm256_add_ps(vec0, vec1); \
    _mm256_store_ps(pr, sum); \
}
#if defined(USE_FMA3)
#define INPUT_MULTADD() \
for (j = (cHidden >> LOG2VEC_SIZE); j; j--, pr += VEC_SIZE, prWeight += VEC_SIZE) { \
    vec0 = _mm256_load_ps(pr); \
    vec1 = _mm256_load_ps(prWeight); \
    sum = _mm256_fmadd_ps(vec1, scalevec, vec0); \
    _mm256_store_ps(pr, sum); \
}
#else
#define INPUT_MULTADD() \
for (j = (cHidden >> LOG2VEC_SIZE); j; j--, pr += VEC_SIZE, prWeight += VEC_SIZE) { \
    vec0 = _mm256_load_ps(pr); \
    vec1 = _mm256_load_ps(prWeight); \
    vec3 = _mm256_mul_ps(vec1, scalevec); \
    sum = _mm256_add_ps(vec0, vec3); \
    _mm256_store_ps(pr, sum); \
}
#endif
#endif
#if defined(USE_NEON)
#define INPUT_ADD() \
for (j = (cHidden >> LOG2VEC_SIZE); j; j--, pr += VEC_SIZE, prWeight += VEC_SIZE) { \
    vec0 = vld1q_f32(pr); \
    vec1 = vld1q_f32(prWeight); \
    sum = vaddq_f32(vec0, vec1); \
    vst1q_f32(pr, sum); \
}
#define INPUT_MULTADD() \
for (j = (cHidden >> LOG2VEC_SIZE); j; j--, pr += VEC_SIZE, prWeight += VEC_SIZE) { \
    vec0 = vld1q_f32(pr); \
    vec1 = vld1q_f32(prWeight); \
    vec3 = vmulq_f32(vec1, scalevec); \
    sum = vaddq_f32(vec0, vec3); \
    vst1q_f32(pr, sum); \
}
#endif

static void
EvaluateSSE(const neuralnet * restrict pnn, const float arInput[], float ar[], float arOutput[])
{
    const unsigned int cHidden = pnn->cHidden;
    unsigned int i, j;
    float *prWeight;
#if defined(USE_SSE2) || defined(USE_AVX) || defined(USE_NEON)
    float *par;
#if defined(USE_FMA3)
    float_vector vec0, vec1, scalevec, sum;
#else
    float_vector vec0, vec1, vec3, scalevec, sum;
#endif
#endif

    /* Calculate activity at hidden nodes */
    memcpy(ar, pnn->arHiddenThreshold, cHidden * sizeof(float));

    prWeight = pnn->arHiddenWeight;

    if (pnn->cInput != 214) {   /* everything but the racing net */
        for (i = 0; i < 200;) { /* base inputs */
            float ari = arInput[i++];

            /* 3 binaries, 1 float */

            if (likely(ari == 0.0f))
                prWeight += cHidden;
            else {
                float *pr = ar;
                INPUT_ADD();
            }

            ari = arInput[i++];

            if (likely(ari == 0.0f))
                prWeight += cHidden;
            else {
                float *pr = ar;
                INPUT_ADD();
            }

            ari = arInput[i++];

            if (likely(ari == 0.0f)) {
                prWeight += cHidden;
                /* If 3rd element is 0, so is 4th. Skip it */
                prWeight += cHidden;
                i++;
                continue;
            } else {
                float *pr = ar;
                INPUT_ADD();
            }

            ari = arInput[i++];

            if (likely(ari == 0.0f))
                prWeight += cHidden;
            else {
                float *pr = ar;

#if defined(USE_FMA3)
                scalevec = _mm256_set1_ps(ari);
                INPUT_MULTADD();
#elif defined(USE_NEON)
                scalevec = vdupq_n_f32(ari);
                INPUT_MULTADD();
#else
                if (unlikely(ari == 1.0f)) {
                    INPUT_ADD();
                } else {
#if defined(USE_AVX)
                    scalevec = _mm256_set1_ps(ari);
#elif defined(HAVE_SSE)
                    scalevec = _mm_set1_ps(ari);
#endif
                    INPUT_MULTADD();
                }
#endif
            }                   /* base inputs are done */
        }

        if (pnn->cInput == 250) /* Pruning nets are over, contact/crashed still have 2 * 25 floats */
            for (i = 200; i < 250; i++) {
                float const ari = arInput[i];

                if (unlikely(ari == 0.0f))
                    prWeight += cHidden;
                else {
                    float *pr = ar;

#if defined(USE_AVX)
                    scalevec = _mm256_set1_ps(ari);
#elif defined(HAVE_SSE)
                    scalevec = _mm_set1_ps(ari);
#else
                    scalevec = vdupq_n_f32(ari);
#endif
                    INPUT_MULTADD();
                }
            }
    }

    else                        /* racing net */
        for (i = 0; i < pnn->cInput; i++) {
            float const ari = arInput[i];

            if (likely(ari == 0.0f))
                prWeight += cHidden;
            else {
                float *pr = ar;
#if defined(USE_FMA3)
                scalevec = _mm256_set1_ps(ari);
                INPUT_MULTADD();
#elif defined(USE_NEON)
                scalevec = vdupq_n_f32(ari);
                INPUT_MULTADD();
#else
                if (likely(ari == 1.0f)) {
                    INPUT_ADD();
                } else {
#if defined(USE_AVX)
                    scalevec = _mm256_set1_ps(ari);
#elif defined(HAVE_SSE)
                    scalevec = _mm_set1_ps(ari);
#endif
                    INPUT_MULTADD();
                }
#endif
            }
        }

#if defined(USE_SSE2) || defined(USE_AVX) || defined(USE_NEON)
#if defined(USE_AVX)
    scalevec = _mm256_set1_ps(-pnn->rBetaHidden);
#elif defined(HAVE_SSE)
    scalevec = _mm_set1_ps(-pnn->rBetaHidden);
#else
    scalevec = vdupq_n_f32(-pnn->rBetaHidden);
#endif

    for (par = ar, i = (cHidden >> LOG2VEC_SIZE); i; i--, par += VEC_SIZE) {
#if defined(USE_AVX)
        float_vector vec = _mm256_load_ps(par);
        vec = _mm256_mul_ps(vec, scalevec);
        vec = sigmoid_ps(vec);
        _mm256_store_ps(par, vec);
#elif defined(HAVE_SSE)
        float_vector vec = _mm_load_ps(par);
        vec = _mm_mul_ps(vec, scalevec);
        vec = sigmoid_ps(vec);
        _mm_store_ps(par, vec);
#else
        float_vector vec = vld1q_f32(par);
        vec = vmulq_f32(vec, scalevec);
        vec = sigmoid_ps(vec);
        vst1q_f32(par, vec);
#endif
    }
#else
    for (i = 0; i < cHidden; i++)
        ar[i] = sigmoid(-pnn->rBetaHidden * ar[i]);
#endif

    /* Calculate activity at output nodes */
    prWeight = pnn->arOutputWeight;

    for (i = 0; i < pnn->cOutput; i++) {

#if defined(USE_AVX)
        SSE_ALIGN(float r[8]);
#else
        float r;
#endif
        float *pr = ar;
#if defined(USE_AVX)
        sum = _mm256_setzero_ps();
#elif defined(HAVE_SSE)
        sum = _mm_setzero_ps();
#else
        sum = vdupq_n_f32(0.0f);
#endif
        for (j = (cHidden >> LOG2VEC_SIZE); j; j--, prWeight += VEC_SIZE, pr += VEC_SIZE) {
#if defined(USE_AVX)
            vec0 = _mm256_load_ps(pr);  /* Eight floats into vec0 */
            vec1 = _mm256_load_ps(prWeight);    /* Eight weights into vec1 */
#if defined(USE_FMA3)
            sum = _mm256_fmadd_ps(vec0, vec1, sum);
#else
            vec3 = _mm256_mul_ps(vec0, vec1);   /* Multiply */
            sum = _mm256_add_ps(sum, vec3);     /* Add */
#endif
#elif defined(HAVE_SSE)
            vec0 = _mm_load_ps(pr);     /* Four floats into vec0 */
            vec1 = _mm_load_ps(prWeight);       /* Four weights into vec1 */
            vec3 = _mm_mul_ps(vec0, vec1);      /* Multiply */
            sum = _mm_add_ps(sum, vec3);        /* Add */
#else
            vec0 = vld1q_f32(pr);     /* Four floats into vec0 */
            vec1 = vld1q_f32(prWeight);       /* Four weights into vec1 */
            vec3 = vmulq_f32(vec0, vec1);      /* Multiply */
            sum = vaddq_f32(sum, vec3);        /* Add */
#endif
        }

#if defined(USE_AVX)
        vec0 = _mm256_hadd_ps(sum, sum);
        vec1 = _mm256_hadd_ps(vec0, vec0);
        _mm256_store_ps(r, vec1);

        arOutput[i] = sigmoid(-pnn->rBetaOutput * (r[0] + r[4] + pnn->arOutputThreshold[i]));
#elif defined(HAVE_SSE)
        vec0 = _mm_shuffle_ps(sum, sum, _MM_SHUFFLE(2, 3, 0, 1));
        vec1 = _mm_add_ps(sum, vec0);
        vec0 = _mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(1, 1, 3, 3));
        sum = _mm_add_ps(vec1, vec0);
        _mm_store_ss(&r, sum);

        arOutput[i] = sigmoid(-pnn->rBetaOutput * (r + pnn->arOutputThreshold[i]));

#else
        {
            float32x2_t vec0_h, vec0_l, vec1;

            vec0_h = vget_high_f32(sum);
            vec0_l = vget_low_f32(sum);
            vec1 = vpadd_f32(vec0_h, vec0_l);
            vec1 = vpadd_f32(vec1, vec1);
            vst1_lane_f32(&r, vec1, 0);

            arOutput[i] = sigmoid(-pnn->rBetaOutput * (r + pnn->arOutputThreshold[i]));
        }
#endif
    }
#if defined(USE_AVX)
    _mm256_zeroupper();
#endif
}


extern int
NeuralNetEvaluateSSE(const neuralnet * restrict pnn, /*lint -e{818} */ float arInput[],
                     float arOutput[], NNState * UNUSED(pnState))
{
    SSE_ALIGN(float ar[pnn->cHidden]);

#if DEBUG_SSE
    g_assert(sse_aligned(arOutput));
    g_assert(sse_aligned(ar));
    g_assert(sse_aligned(arInput));
#endif

    EvaluateSSE(pnn, arInput, ar, arOutput);
    return 0;
}

#endif

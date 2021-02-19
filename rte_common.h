/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */

#ifndef _RTE_COMMON_H_
#define _RTE_COMMON_H_

/**
 * @file
 *
 * Generic, commonly-used macro and inline function definitions
 * for DPDK.
 */

#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#define MPLOCKED        "lock ; "       /**< Insert MP lock prefix. */

/* $getconf LEVEL1_DCACHE_LINESIZE */
#define RTE_CACHE_LINE_SIZE 64

#define RTE_CACHE_LINE_MASK (RTE_CACHE_LINE_SIZE-1) /**< Cache line mask. */

/**
 * Macro to align a value to a given power-of-two. The resultant value
 * will be of the same type as the first parameter, and will be no
 * bigger than the first parameter. Second parameter must be a
 * power-of-two value.
 */
#define RTE_ALIGN_FLOOR(val, align) \
  (typeof(val))((val) & (~((typeof(val))((align) - 1))))

/**
 * Macro to align a value to a given power-of-two. The resultant value
 * will be of the same type as the first parameter, and will be no lower
 * than the first parameter. Second parameter must be a power-of-two
 * value.
 */
#define RTE_ALIGN_CEIL(val, align) \
  RTE_ALIGN_FLOOR(((val) + ((typeof(val)) (align) - 1)), align)

/**
 * Macro to align a value to a given power-of-two. The resultant
 * value will be of the same type as the first parameter, and
 * will be no lower than the first parameter. Second parameter
 * must be a power-of-two value.
 * This function is the same as RTE_ALIGN_CEIL
 */
#define RTE_ALIGN(val, align) RTE_ALIGN_CEIL(val, align)


/**
 * Force alignment
 */
#define __rte_aligned(a) __attribute__((__aligned__(a)))

/**
 * Force alignment to cache line.
 */
#define __rte_cache_aligned __rte_aligned(RTE_CACHE_LINE_SIZE)

/**
 * Force a function to be inlined
 */
#define __rte_always_inline inline __attribute__((always_inline))

/**
 * Triggers an error at compilation time if the condition is true.
 */
#define RTE_BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

/* true if x is a power of 2 */
#define POWEROF2(x) ((((x)-1) & (x)) == 0)

/**
 * General memory barrier.
 *
 * Guarantees that the LOAD and STORE operations generated before the
 * barrier occur before the LOAD and STORE operations generated after.
 */
#define	rte_mb()  __sync_synchronize()

/**
 * Write memory barrier.
 *
 * Guarantees that the STORE operations generated before the barrier
 * occur before the STORE operations generated after.
 */
//#define	rte_wmb() do { asm volatile ("dmb st" ::: "memory"); } while (0)
#define	rte_wmb() do { asm volatile ("" ::: "memory"); } while (0)

/**
 * Read memory barrier.
 *
 * Guarantees that the LOAD operations generated before the barrier
 * occur before the LOAD operations generated after.
 */
#define	rte_rmb() __sync_synchronize()

#define rte_smp_mb() rte_mb()
#define rte_smp_wmb() rte_wmb()
#define rte_smp_rmb() rte_rmb()

#define MS_PER_S 1000
#define US_PER_S 1000000
#define NS_PER_S 1000000000


/**
 * Combines 32b inputs most significant set bits into the least
 * significant bits to construct a value with the same MSBs as x
 * but all 1's under it.
 *
 * @param x
 *    The integer whose MSBs need to be combined with its LSBs
 * @return
 *    The combined value.
 */
static inline uint32_t rte_combine32ms1b(register uint32_t x)
{
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;

  return x;
}

/**
 * Aligns input parameter to the next power of 2
 *
 * @param x
 *   The integer value to algin
 *
 * @return
 *   Input parameter aligned to the next power of 2
 */
static inline uint32_t rte_align32pow2(uint32_t x)
{
  x--;
  x = rte_combine32ms1b(x);

  return x + 1;
}

static inline int rte_atomic32_cmpset(volatile uint32_t *dst, uint32_t exp, uint32_t src)
{
  uint8_t res;

  asm volatile(
      MPLOCKED
      "cmpxchgl %[src], %[dst];"
      "sete %[res];"
      : [res] "=a" (res),     /* output */
      [dst] "=m" (*dst)
      : [src] "r" (src),      /* input */
      "a" (exp),
      "m" (*dst)
      : "memory");            /* no-clobber list */
  return res;
}

static inline uint64_t rte_rdtsc(void)
{
  union {
    uint64_t tsc_64;
    struct {
      uint32_t lo_32;
      uint32_t hi_32;
    };
  } tsc;

  asm volatile("rdtsc" :
      "=a" (tsc.lo_32),
      "=d" (tsc.hi_32));
  return tsc.tsc_64;
}

static inline uint64_t rte_get_tsc_hz(void)
{
  struct timespec t_start;
  if (clock_gettime(CLOCK_MONOTONIC_RAW, &t_start) == 0)
  {
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = NS_PER_S / 10; /* 1/10 second */

    uint64_t ns, end, start = rte_rdtsc();
    nanosleep(&sleeptime,NULL);

    struct timespec t_end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t_end);
    end = rte_rdtsc();
    ns = ((t_end.tv_sec - t_start.tv_sec) * NS_PER_S);
    ns += (t_end.tv_nsec - t_start.tv_nsec);

    double secs = (double)ns/NS_PER_S;
    return(uint64_t)((end - start)/secs);
  }
  else
  {
    uint64_t start = rte_rdtsc();
    sleep(1);
    return rte_rdtsc() - start;
  }
}
#endif


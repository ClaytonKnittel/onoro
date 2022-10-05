#ifndef _RANDOM_H
#define _RANDOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// seeds random number generator with given value
void seed_rand(uint64_t init_seed, uint64_t seq_num);

// generates next random number in sequence
uint32_t gen_rand();

// same as gen_rand, but gives a 64-bit number
uint64_t gen_rand64();

// generates a random number from 0 to max - 1
uint32_t gen_rand_r(uint32_t max);

// generates a random number from 0 to max - 1
uint64_t gen_rand_r64(uint64_t max);

#ifdef __cplusplus
}
#endif

#endif /* _RANDOM_H */

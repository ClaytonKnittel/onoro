
#include "random.h"

#include <pthread.h>

struct rand_state {
  // tracks state of RNG
  uint64_t state;
  // sequence number for RNG (will determine how to get from one state
  // to the next, generates unique sequences for different initial states)
  uint64_t seq_num;
};

/*
 * strong declaration of __seed (default to 0)
 */
__thread struct rand_state __state = {
  .state = 0LU,
  .seq_num = 1LU,
};

uint32_t __rand_hash(uint32_t seed) {
  uint64_t v = (uint64_t) seed;
  v = (~v) + (v << 18);
  v ^= v >> 31;
  v *= 21;
  v &= v >> 11;
  v += v << 6;
  v ^= v >> 22;
  return (uint32_t) v;
}

static uint32_t _gen_rand(struct rand_state* state);

static void _seed_rand(struct rand_state* state, uint64_t init_seed,
                       uint64_t seq_num) {
  // hash seed 3 times for maximal randomness!
  state->state = 0LU;
  // sequence number must be odd
  state->seq_num = (seq_num << 1LU) | 1LU;

  _gen_rand(state);
  state->state += init_seed;
  _gen_rand(state);
}

void seed_rand(uint64_t init_seed, uint64_t seq_num) {
  _seed_rand(&__state, init_seed, seq_num);
}

static uint32_t _gen_rand(struct rand_state* state) {
  uint64_t prev = state->state;
  state->state = prev * 6364136223846793005ULL + state->seq_num;

  // do some xor stuff
  uint32_t xor = ((prev >> 18U) ^ prev) >> 27U;
  uint32_t rot = prev >> 59U;

  // rotate result by "rot"
  return (xor >> rot) | (xor << ((-rot) & 0x1f));
}

static uint64_t _gen_rand64(struct rand_state* state) {
  uint32_t r1 = _gen_rand(state);
  uint32_t r2 = _gen_rand(state);
  return (((uint64_t) r1) << 32) | ((uint64_t) r2);
}

/*
 * generate random number (32 bit)
 */
uint32_t gen_rand() {
  return _gen_rand(&__state);
}

uint64_t gen_rand64() {
  return _gen_rand64(&__state);
}

static uint32_t _gen_rand_r(struct rand_state* state, uint32_t max) {
  // equivalent to 0x100000000lu % max, but is done with 32-bit numbers so
  // it's faster
  uint32_t thresh = -max % max;

  // range is limited to thresh and above, to eliminate any bias (i.e. if
  // max is 3, then 0 is not allowed to be chosen, as 0xffffffffffffffff
  // would also give 0 as a result, meaning 0 is slightly more likely to
  // be chosen)
  uint32_t res;
  do {
    res = _gen_rand(state);
  } while (__builtin_expect(res < thresh, 0));

  return res % max;
}

uint32_t gen_rand_r(uint32_t max) {
  return _gen_rand_r(&__state, max);
}

static uint64_t _gen_rand_r64(struct rand_state* state, uint64_t max) {
  // mathematically equivalent to 0x10000000000000000lu % max
  uint64_t thresh = -max % max;

  // range is limited to thresh and above, to eliminate any bias (i.e. if
  // max is 3, then 0 is not allowed to be chosen, as 0xffffffffffffffff
  // would also give 0 as a result, meaning 0 is slightly more likely to
  // be chosen)
  uint64_t res;
  do {
    res = _gen_rand64(state);
  } while (__builtin_expect(res < thresh, 0));

  return res % max;
}

uint64_t gen_rand_r64(uint64_t max) {
  return _gen_rand_r64(&__state, max);
}

`PcgRandom`
-----------

A 32-bit pseudorandom number generator.
Uses PCG32, an algorithm of the permuted congruential generator family,
offering very strong randomness.

* constructor `PcgRandom(seed, [seq])`
  * `seed`: 64-bit unsigned seed
  * `seq`: 64-bit unsigned sequence, optional

### Methods

* `next()`: return next integer random number [`-2147483648`...`2147483647`]
* `next(min, max)`: return next integer random number [`min`...`max`]
* `rand_normal_dist(min, max, num_trials=6)`: return normally distributed
  random number [`min`...`max`].
    * This is only a rough approximation of a normal distribution with:
    * `mean = (max - min) / 2`, and
    * `variance = (((max - min + 1) ^ 2) - 1) / (12 * num_trials)`
    * Increasing `num_trials` improves accuracy of the approximation
* `get_state()`: return generator state encoded in string
* `set_state(state_string)`: restore generator state from encoded string

`PseudoRandom`
--------------

A 16-bit pseudorandom number generator.
Uses a well-known LCG algorithm introduced by K&R.

**Note**:
`PseudoRandom` is slower and has worse random distribution than `PcgRandom`.
Use `PseudoRandom` only if you need output to match the well-known LCG algorithm introduced by K&R.
Otherwise, use `PcgRandom`.

* constructor `PseudoRandom(seed)`
  * `seed`: 32-bit signed number

### Methods

* `next()`: return next integer random number [`0`...`32767`]
* `next(min, max)`: return next integer random number [`min`...`max`]
    * Either `max - min == 32767` or `max - min <= 6553` must be true
      due to the simple implementation making a bad distribution otherwise.
* `get_state()`: return state of pseudorandom generator as number
    * use returned number as seed in PseudoRandom constructor to restore

`SecureRandom`
--------------

Interface for the operating system's crypto-secure PRNG.

It can be created via `SecureRandom()`.  The constructor throws an error if a
secure random device cannot be found on the system.

### Methods

* `next_bytes([count])`: return next `count` (default 1, capped at 2048) many
  random bytes, as a string.

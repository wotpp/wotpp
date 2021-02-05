#pragma once

#ifndef WOTPP_RNG
#define WOTPP_RNG

// Random number generation using xoshiro256**.

namespace wpp {
	namespace detail {
		constexpr uint64_t splitmix64(uint64_t seed) {
			seed = (seed ^ (seed >> 30)) * 0xBF58476D1CE4E5B9;
			seed = (seed ^ (seed >> 27)) * 0x94D049BB133111EB;
			return seed ^ (seed >> 31);
		}

		constexpr uint64_t rotl(uint64_t x, int k) {
			return (x << k) | (x >> (64 - k));
		}
	}

	struct Random {
		uint64_t state[4];

		constexpr Random(uint64_t seed): state{} {
			seed = detail::splitmix64(seed);
			state[0] = seed;

			seed = detail::splitmix64(seed);
			state[1] = seed;

			seed = detail::splitmix64(seed);
			state[2] = seed;

			seed = detail::splitmix64(seed);
			state[3] = seed;
		}

		constexpr uint64_t next() {
			const uint64_t result = detail::rotl(state[0] + state[3], 23) + state[0];
			const uint64_t t = state[1] << 17;

			state[2] ^= state[0];
			state[3] ^= state[1];
			state[1] ^= state[2];
			state[0] ^= state[3];

			state[2] ^= t;
			state[3] = detail::rotl(state[3], 45);

			return result;
		}

		constexpr uint64_t range(uint64_t min, uint64_t max) {
#ifndef _WIN32
			// :^)
			uint64_t range = max - min + 1;
			uint64_t x = 0, r = 0;

			do {
				x = next();
				r = x;

				if (r >= range) {
					r -= range;

					if (r >= range)
						r %= range;
				}

			} while (x - r > -range);

			return r + min;
#endif
#ifdef _WIN32
			return 0;
#endif
		}

		constexpr bool boolean() {
			return range(0, 1);
		}
	};
}

#endif

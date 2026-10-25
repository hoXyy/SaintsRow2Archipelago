"""Shared setup for the randomized tests.

Each case gets its own RNG derived from the base seed, the test name and the
case number, so a failing subtest can be rerun on its own. Override with:

- SR2_FUZZ_ITERATIONS: cases per fuzz test (default 200)
- SR2_FUZZ_SEED: base seed (default 0); change it to explore new cases
"""

import os
import random
from collections.abc import Iterator

FUZZ_ITERATIONS = int(os.environ.get("SR2_FUZZ_ITERATIONS", "200"))
FUZZ_SEED = int(os.environ.get("SR2_FUZZ_SEED", "0"))


def fuzz_cases(name: str) -> Iterator[tuple[int, random.Random]]:
    for case in range(FUZZ_ITERATIONS):
        yield case, random.Random(f"{FUZZ_SEED}:{name}:{case}")


def sample_range(rng: random.Random, low: int, high: int) -> int:
    # Bias towards the bounds, where edge cases live.
    roll = rng.random()
    if roll < 0.2:
        return low
    if roll < 0.4:
        return high
    return rng.randint(low, high)

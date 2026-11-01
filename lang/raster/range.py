"""Inclusive integer ranges.

MIN, NOM, and MAX are integers with MIN <= NOM <= MAX.
NOM is the value used when the caller does not supply one.
Any other integer v folds into the closed interval:

    span = MAX - MIN + 1
    quot, rem = divmod(v - MIN, span)
    folded = MIN + rem

rem is always in 0 .. span-1. quot is how many spans were removed.
"""

from __future__ import annotations


def fold(value: int | None, low: int, nominal: int, high: int) -> tuple[int, int]:
    if high < low:
        raise ValueError("MAX is below MIN")
    if not (low <= nominal <= high):
        raise ValueError("NOM is outside MIN..MAX")
    if value is None:
        return nominal, 0
    span = high - low + 1
    quot, rem = divmod(value - low, span)
    return low + rem, quot

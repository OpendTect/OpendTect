import pytest
from numbers import Number

def approx(expected, rel=None, abs=None, nan_ok=False):
    if isinstance(expected, dict):
        return {k: approx(v, rel, abs, nan_ok) for k, v in expected.items()}
    if isinstance(expected, (tuple, list)):
        return type(expected)(approx(v, rel, abs, nan_ok) for v in expected)
    if isinstance(expected, Number):
        return pytest.approx(expected, rel, abs, nan_ok)
    return expected

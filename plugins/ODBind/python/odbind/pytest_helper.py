import pytest
from _pytest.python_api import ApproxBase, ApproxMapping, ApproxSequenceLike

class ApproxBaseReprMixin(ApproxBase):
    def __repr__(self) -> str:

        def recur_repr_helper(obj):
            if isinstance(obj, dict):
                return dict((k, recur_repr_helper(v)) for k, v in obj.items())
            elif isinstance(obj, tuple):
                return tuple(recur_repr_helper(o) for o in obj)
            elif isinstance(obj, list):
                return list(recur_repr_helper(o) for o in obj)
            else:
                return self._approx_scalar(obj)

        return "approx({!r})".format(recur_repr_helper(self.expected))


class ApproxNestedSequenceLike(ApproxSequenceLike, ApproxBaseReprMixin):

    def _yield_comparisons(self, actual):
        for k in range(len(self.expected)):
            if isinstance(self.expected[k], dict):
                mapping = ApproxNestedMapping(self.expected[k], rel=self.rel, abs=self.abs, nan_ok=self.nan_ok)
                for el in mapping._yield_comparisons(actual[k]):
                    yield el
            elif isinstance(self.expected[k], (tuple, list)):
                mapping = ApproxNestedSequenceLike(self.expected[k], rel=self.rel, abs=self.abs, nan_ok=self.nan_ok)
                for el in mapping._yield_comparisons(actual[k]):
                    yield el
            else:
                yield actual[k], self.expected[k]

    def _check_type(self):
        pass


class ApproxNestedMapping(ApproxMapping, ApproxBaseReprMixin):

    def _yield_comparisons(self, actual):
        for k in self.expected.keys():
            if isinstance(self.expected[k], dict):
                mapping = ApproxNestedMapping(self.expected[k], rel=self.rel, abs=self.abs, nan_ok=self.nan_ok)
                for el in mapping._yield_comparisons(actual[k]):
                    yield el
            elif isinstance(self.expected[k], (tuple, list)):
                mapping = ApproxNestedSequenceLike(self.expected[k], rel=self.rel, abs=self.abs, nan_ok=self.nan_ok)
                for el in mapping._yield_comparisons(actual[k]):
                    yield el
            else:
                yield actual[k], self.expected[k]

    def _check_type(self):
        pass


def approx(expected, rel=None, abs=None, nan_ok=False):
    if isinstance(expected, dict):
        return ApproxNestedMapping(expected, rel, abs, nan_ok)
    if isinstance(expected, (tuple, list)):
        return ApproxNestedSequenceLike(expected, rel, abs, nan_ok)
    return pytest.approx(expected, rel, abs, nan_ok)

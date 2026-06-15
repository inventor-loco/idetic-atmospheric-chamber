"""Tests for the Cn² estimator."""

import numpy as np

from orchestrator.analysis import cn2_from_array, structure_function

# 5 sensors spaced 10 cm apart on the side wall (PROJECT_SEED §4.4).
POSITIONS = np.array([0.0, 0.1, 0.2, 0.3, 0.4])


def test_structure_function_pair_count():
    seps, dvals = structure_function(np.zeros(5), POSITIONS)
    # 5 choose 2 = 10 unordered pairs.
    assert len(seps) == 10
    assert len(dvals) == 10


def test_uniform_field_gives_zero_cn2():
    # No temperature gradient -> no turbulence -> Cn² == 0.
    temps = np.full(5, 25.0)
    assert cn2_from_array(temps, POSITIONS) == 0.0


def test_cn2_positive_and_increases_with_gradient():
    mild = np.array([22.0, 23.0, 24.0, 25.0, 26.0])
    steep = np.array([22.0, 26.0, 30.0, 34.0, 38.0])
    cn2_mild = cn2_from_array(mild, POSITIONS)
    cn2_steep = cn2_from_array(steep, POSITIONS)
    assert cn2_mild > 0
    assert cn2_steep > cn2_mild

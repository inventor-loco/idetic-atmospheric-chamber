"""Estimate the refractive-index structure constant Cn² from the vertical
temperature array (PROJECT_SEED §6, §12).

Method
------
1. Temperature structure function over the array of sensors::

       D_T(r) = < ( T(x + r) - T(x) )^2 >

2. In the inertial subrange ``D_T(r) = C_T² · r^(2/3)``, so the temperature
   structure constant ``C_T²`` is the best fit of ``D_T`` against ``r^(2/3)``.

3. Convert to the optical index structure constant (Andrews & Phillips)::

       Cn² = ( 79e-6 · P / T² )² · C_T²

   with pressure ``P`` in hPa (mbar) and absolute temperature ``T`` in kelvin.

This is a coarse estimate: 5 sensors give few separations and we assume local
isotropy. Treat the output as a relative turbulence indicator pending the
proper Rytov-based calibration in Phase 3.
"""

from __future__ import annotations

import numpy as np


def structure_function(
    temps_c: np.ndarray, positions_m: np.ndarray
) -> tuple[np.ndarray, np.ndarray]:
    """Return ``(separations, D_T)`` for every sensor pair.

    ``temps_c`` and ``positions_m`` are 1-D arrays of equal length, ordered
    along the vertical array.
    """
    temps_c = np.asarray(temps_c, dtype=float)
    positions_m = np.asarray(positions_m, dtype=float)
    if temps_c.shape != positions_m.shape or temps_c.ndim != 1:
        raise ValueError("temps_c and positions_m must be 1-D and equal length")

    seps: list[float] = []
    dvals: list[float] = []
    n = len(temps_c)
    for i in range(n):
        for j in range(i + 1, n):
            seps.append(abs(positions_m[j] - positions_m[i]))
            dvals.append((temps_c[j] - temps_c[i]) ** 2)
    return np.array(seps), np.array(dvals)


def cn2_from_array(
    temps_c: np.ndarray,
    positions_m: np.ndarray,
    pressure_hpa: float = 1013.25,
) -> float:
    """Estimate Cn² (m^-2/3) from one snapshot of the vertical array."""
    seps, dvals = structure_function(temps_c, positions_m)
    mask = seps > 0
    if not np.any(mask):
        return float("nan")

    # Fit D_T = C_T² · r^(2/3) through the origin (least squares on the slope).
    basis = seps[mask] ** (2.0 / 3.0)
    c_t2 = float(np.dot(basis, dvals[mask]) / np.dot(basis, basis))
    c_t2 = max(c_t2, 0.0)

    t_kelvin = float(np.mean(temps_c)) + 273.15
    factor = 79e-6 * pressure_hpa / (t_kelvin**2)
    return factor**2 * c_t2

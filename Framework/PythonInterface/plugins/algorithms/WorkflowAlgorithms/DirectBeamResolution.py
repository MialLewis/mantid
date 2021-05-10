# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import math
import numpy


class DirectBeamResolution:

    def __call__(self, q):
        if isinstance(q, numpy.ndarray):
            if q.ndim > 1:
                raise NotImplementedError('Resolution calculation not supported for arrays with more than one dimension.')
            else:
                return numpy.array([self._delta_q(qi) for qi in q])
        elif isinstance(q, list):
            return [self._delta_q(qi) for qi in q]
        else:
            return self._delta_q(q)

    def __init__(self, wavelength, delta_wavelength, beam_width):
        """
        Sets up the parametrized formula
        Args:
            wavelength: Constant wavelength [A]
            delta_wavelength: Relative wavelength resolution []
            beam_width: Fitted horizontal beam width resolution [rad]
            l2: Sample to detector distance [m]
        """
        self._wavelength = wavelength
        self._delta_wavelength = delta_wavelength**2
        self._delta_theta = beam_width**2

    def _delta_q(self, q):
        """
        Returns the sigma_Q at given q
        Args:
            q: Momentum transfer [inverse Angstrom]
        Returns: Absolute Q resolution [inverse Angstrom]
        """
        wavelength_coeff = (1/(2.0 * math.sqrt(2.0*math.log(2.0))))**2
        k = 4.0 * math.pi / self._wavelength
        return math.sqrt(q**2 * wavelength_coeff * self._delta_wavelength + self._delta_theta * (k**2 - q**2))

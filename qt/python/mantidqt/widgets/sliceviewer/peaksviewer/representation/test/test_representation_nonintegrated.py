# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports
import unittest
from unittest.mock import MagicMock

# local imports
from mantidqt.widgets.sliceviewer.model import SliceInfo
from mantidqt.widgets.sliceviewer.peaksviewer.representation \
    import NonIntegratedPeakRepresentation
from mantidqt.widgets.sliceviewer.peaksviewer.representation.test.representation_test_mixin \
    import PeakRepresentationMixin


class NonIntegratedPeakRepresentationTest(unittest.TestCase, PeakRepresentationMixin):
    REPR_CLS = NonIntegratedPeakRepresentation

    def create_test_object(self):
        self.x, self.y, self.z, self.alpha, shape, \
            self.fg_color, self.bg_color = 0.0, 1.0, -1.0, 0.5, None, 'b', 'w'
        sliceinfo = MagicMock(spec=SliceInfo)
        sliceinfo.transform.return_value = (self.x, self.y, self.z)
        sliceinfo.value = -1.05
        sliceinfo.width = 10.
        self.alpha = 0.53333

        return self.REPR_CLS.create((self.x, self.y, self.z), shape, sliceinfo, self.fg_color,
                                    self.bg_color)

    def test_noshape_representation_draw_creates_cross(self):
        x, y, z, alpha, fg_color = 0.0, 1.0, -1.0, 0.5, 'b'
        drawables = [MagicMock()]
        no_shape = NonIntegratedPeakRepresentation(x, y, z, alpha, fg_color, drawables)
        painter = MagicMock()

        no_shape.draw(painter)

        drawables[0].draw.assert_called_once_with(painter, no_shape)


if __name__ == "__main__":
    unittest.main()

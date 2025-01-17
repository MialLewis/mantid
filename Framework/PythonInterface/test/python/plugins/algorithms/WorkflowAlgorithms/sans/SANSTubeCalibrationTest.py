# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantid.api import AnalysisDataService
from testhelpers import create_algorithm
from plugins.algorithms.WorkflowAlgorithms.SANS.SANSTubeCalibration import Prop, DetectorInfo


class SANSTubeCalibrationTest(unittest.TestCase):
    def tearDown(self):
        AnalysisDataService.clear()

    def test_not_enough_data_files_for_strip_positions_throws_error(self):
        data_ws_name = "test_SANS_calibration_ws"
        args = {Prop.STRIP_POSITIONS: [920, 755, 590, 425, 260, 95, 5], Prop.DATA_FILES: [data_ws_name]}
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error("There must be a measurement for each strip position.", alg)

    def test_not_enough_strip_positions_for_data_files_throws_error(self):
        data_ws_name = "test_SANS_calibration_ws"
        args = {Prop.STRIP_POSITIONS: [920], Prop.DATA_FILES: [data_ws_name, data_ws_name]}
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error("There must be a strip position for each measurement.", alg)

    def test_duplicate_strip_positions_throws_error(self):
        data_ws_name = "test_SANS_calibration_ws"
        args = {Prop.STRIP_POSITIONS: [920, 920, 475], Prop.DATA_FILES: [data_ws_name, data_ws_name, data_ws_name]}
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error("Duplicate strip positions are not permitted.", alg)

    def test_end_pixel_not_greater_than_start_pixel_throws_error(self):
        data_ws_name = "test_SANS_calibration_ws"
        expected_error = "The ending pixel must have a greater index than the starting pixel."

        args = {
            Prop.STRIP_POSITIONS: [920],
            Prop.DATA_FILES: [data_ws_name],
            Prop.START_PIXEL: 10,
            Prop.END_PIXEL: 5,
        }
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error(expected_error, alg)

        args = {
            Prop.STRIP_POSITIONS: [920],
            Prop.DATA_FILES: [data_ws_name],
            Prop.START_PIXEL: 10,
            Prop.END_PIXEL: 10,
        }
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error(expected_error, alg)

    def test_end_pixel_larger_then_pixels_in_tube_throws_error(self):
        data_ws_name = "test_SANS_calibration_ws"
        args = {
            Prop.STRIP_POSITIONS: [920],
            Prop.DATA_FILES: [data_ws_name],
            Prop.START_PIXEL: 10,
            Prop.END_PIXEL: DetectorInfo.NUM_PIXELS_IN_TUBE + 100,
        }
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error("The ending pixel must be less than or equal to", alg)

    def _setup_front_detector_calibration(self, args):
        default_args = {Prop.ENCODER: 474.2, Prop.REAR_DET: False}
        alg = create_algorithm("SANSTubeCalibration", **default_args, **args)
        alg.setRethrows(True)
        return alg

    def _assert_raises_error(self, error_msg_regex, alg):
        self.assertRaisesRegex(RuntimeError, error_msg_regex, alg.execute)


if __name__ == "__main__":
    unittest.main()

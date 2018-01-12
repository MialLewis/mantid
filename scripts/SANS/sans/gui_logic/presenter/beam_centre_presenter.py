from __future__ import (absolute_import, division, print_function)

import copy

from mantid.kernel import Logger
from ui.sans_isis.beam_centre import BeamCentre
from ui.sans_isis.work_handler import WorkHandler
from sans.sans_batch import SANSCentreFinder
from sans.common.enums import FindDirectionEnum


class BeamCentrePresenter(object):
    class ConcreteBeamCentreListener(BeamCentre.BeamCentreListener):
        def __init__(self, presenter):
            self._presenter = presenter

        def on_run_clicked(self):
            self._presenter.on_run_clicked()

    class CentreFinderListener(WorkHandler.WorkListener):
        def __init__(self, presenter):
            super(BeamCentrePresenter.CentreFinderListener, self).__init__()
            self._presenter = presenter

        def on_processing_finished(self, result):
            self._presenter.on_processing_finished_centre_finder(result)

        def on_processing_error(self, error):
            self._presenter.on_processing_error_centre_finder(error)

    def __init__(self, parent_presenter, WorkHandler, BeamCentreModel):
        super(BeamCentrePresenter, self).__init__()
        self._view = None
        self._parent_presenter = parent_presenter
        self._work_handler = WorkHandler()
        self._logger = Logger("SANS")
        self._beam_centre_model = BeamCentreModel()

    def set_view(self, view):
        if view:
            self._view = view

            # Set up run listener
            listener = BeamCentrePresenter.ConcreteBeamCentreListener(self)
            self._view.add_listener(listener)

            # Set the default gui
            self._view.set_options(self._beam_centre_model)
    
    def on_update_instrument(self, instrument):
        self._beam_centre_model.set_scaling(instrument)
    
    def on_update_rows(self):
        self._view.set_options(self._beam_centre_model)

    def on_processing_finished_centre_finder(self, result):
        # Enable button
        self._view.set_run_button_to_normal()
        # Update Centre Positions in model and GUI
        self._beam_centre_model.lab_pos_1 = result['pos1']
        self._beam_centre_model.lab_pos_2 = result['pos2']
        self._beam_centre_model.hab_pos_1 = result['pos1']
        self._beam_centre_model.hab_pos_2 = result['pos2']

        self._view.lab_pos_1 = self._beam_centre_model.lab_pos_1 * self._beam_centre_model.scale_1
        self._view.lab_pos_2 = self._beam_centre_model.lab_pos_2 * self._beam_centre_model.scale_2
        self._view.hab_pos_1 = self._beam_centre_model.hab_pos_1 * self._beam_centre_model.scale_1
        self._view.hab_pos_2 = self._beam_centre_model.hab_pos_2 * self._beam_centre_model.scale_2

    def on_processing_error_centre_finder(self, error):
        self._logger.warning("There has been an error. See more: {}".format(error))

    def on_processing_error(self, error):
        pass

    def on_run_clicked(self):
        # Get the state information for the first row.
        state = self._parent_presenter.get_state_for_row(0)

        if not state:
            self._logger.information("You can only calculate the beam centre if a user file has been loaded and there"
                                     "valid sample scatter entry has been provided in the selected row.")
            return

        # Disable the button
        self._view.set_run_button_to_processing()

        #Update model
        self._update_beam_model_from_view()

        # Run the task
        listener = BeamCentrePresenter.CentreFinderListener(self)
        state_copy = copy.copy(state)
        self._work_handler.process(listener, find_beam_centre, state_copy, self._beam_centre_model)

    def _update_beam_model_from_view(self):
        self._beam_centre_model.r_min = self._view.r_min
        self._beam_centre_model.r_max = self._view.r_max
        self._beam_centre_model.max_iterations = self._view.max_iterations
        self._beam_centre_model.tolerance = self._view.tolerance
        self._beam_centre_model.left_right = self._view.left_right
        self._beam_centre_model.up_down = self._view.up_down
        self._beam_centre_model.lab_pos_1 = self._view.lab_pos_1 / self._beam_centre_model.scale_1
        self._beam_centre_model.lab_pos_2 = self._view.lab_pos_2 / self._beam_centre_model.scale_2
        self._beam_centre_model.hab_pos_1 = self._view.hab_pos_1 / self._beam_centre_model.scale_1
        self._beam_centre_model.hab_pos_2 = self._view.hab_pos_2 / self._beam_centre_model.scale_2

    def set_on_state_model(self, attribute_name, state_model):
        attribute = getattr(self._view, attribute_name)
        if attribute or isinstance(attribute, bool):
            setattr(state_model, attribute_name, attribute)

    def set_on_view(self, attribute_name, state_model):
        attribute = getattr(state_model, attribute_name)
        if attribute or isinstance(attribute, bool):  # We need to be careful here. We don't want to set empty strings, or None, but we want to set boolean values. # noqa
            setattr(self._view, attribute_name, attribute)


def find_beam_centre(state, beam_centre_model):
    """
    This is called from the GUI and runs the find beam centre algorithm given a state model and a beam_centre_model object.

    :param state: A SANS state object
    :param beam_centre_model: An instance of the BeamCentreModel class.
    :returns: The centre position found.
    """
    centre_finder = SANSCentreFinder()
    find_direction = None
    if beam_centre_model.up_down and beam_centre_model.left_right:
        find_direction = FindDirectionEnum.All
    elif beam_centre_model.up_down:
        find_direction = FindDirectionEnum.Left_Right
    elif beam_centre_model.left_right:
        find_direction = FindDirectionEnum.Up_Down

    centre = centre_finder(state, r_min=beam_centre_model.r_min, r_max=beam_centre_model.r_max, max_iter=beam_centre_model.max_iterations,
                           x_start=beam_centre_model.lab_pos_1, y_start=beam_centre_model.lab_pos_2, tolerance=beam_centre_model.tolerance,
                           find_direction=find_direction, reduction_method=True)
    return centre

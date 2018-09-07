from mantid.kernel import ErrorReporter, UsageService, ConfigService
from mantid.kernel import Logger
from ErrorReporter.retrieve_recovery_files import RetrieveRecoveryFiles
import requests


class ErrorReporterPresenter(object):
    def __init__(self, view, exit_code):
        self.error_log = Logger("error")
        self._view = view
        self._exit_code = exit_code
        self._view.set_report_callback(self.error_handler)

    def do_not_share(self, continue_working=True):
        self.error_log.notice("No information shared")
        self._handle_exit(continue_working)

    def share_non_identifiable_information(self, continue_working):
        uptime = UsageService.getUpTime()
        self._send_report_to_server(share_identifiable=False, uptime=uptime)
        self.error_log.notice("Sent non-identifiable information")
        self._handle_exit(continue_working)

    def share_all_information(self, continue_working, name, email):
        uptime = UsageService.getUpTime()
        zip_recovery_file, file_hash = RetrieveRecoveryFiles.zip_recovery_directory()
        self._send_report_to_server(share_identifiable=False, uptime=uptime, name=name, email=email, file_hash=file_hash)
        self.error_log.notice("Sent complete information")
        self._upload_recovery_file(zip_recovery_file=zip_recovery_file)
        self._handle_exit(continue_working)

    def error_handler(self, continue_working, share, name, email):
        if share == 0:
            self.share_all_information(continue_working, name, email)
        elif share == 1:
            self.share_non_identifiable_information(continue_working)
        elif share == 2:
            self.do_not_share(continue_working)
        else:
            self.error_log.error("Unrecognised signal in errorreporter exiting")
            self._handle_exit(continue_working)

    def _handle_exit(self, continue_working):
        if not continue_working:
            self.error_log.error("Terminated by user.")
            self._view.quit()
        else:
            self.error_log.error("Continue working.")

    def _upload_recovery_file(self, zip_recovery_file):
        url = ConfigService['errorreports.rooturl']
        url = '{}/api/recovery'.format(url)
        files = {'file': open('{}.zip'.format(zip_recovery_file), 'rb')}
        response = requests.post(url, files=files)
        if response.status_code == 201:
            self.error_log.notice("Uploaded recovery file to server HTTP response {}".format(response.status_code))
        else:
            self.error_log.error("Failed to send recovery data HTTP response {}".format(response.status_code))

    def _send_report_to_server(self, share_identifiable=False, name='', email='', file_hash='', uptime=''):
        errorReporter = ErrorReporter(
            "mantidplot", uptime, self._exit_code, share_identifiable, str(name), str(email),
            str(file_hash))
        status = errorReporter.sendErrorReport()

        if status != 201:
            self._view.display_message_box('Error contacting server', 'There was an error when sending the report.'
                                                                      'Please contact mantid-help@mantidproject.org directly',
                                           'http request returned with status {}'.format(status))
            self.error_log.error("Failed to send error report http request returned status {}".format(status))

        return status

    def show_view(self):
        self._view.show()

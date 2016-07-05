#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisFitDataPresenter.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"
#include "MantidQtCustomInterfaces/Muon/MuonSequentialFitDialog.h"
#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"
#include <boost/lexical_cast.hpp>

using MantidQt::MantidWidgets::IMuonFitDataSelector;
using MantidQt::MantidWidgets::IWorkspaceFitControl;
using Mantid::API::AnalysisDataService;
using Mantid::API::MatrixWorkspace;
using Mantid::API::TableRow;
typedef MantidQt::CustomInterfaces::Muon::MuonAnalysisOptionTab::RebinType
    RebinType;

namespace {
/// static logger
Mantid::Kernel::Logger g_log("MuonAnalysisFitDataPresenter");
}

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Constructor
 * @param fitBrowser :: [input] Pointer to fit browser to update
 * @param dataSelector :: [input] Pointer to data selector to get input from
 * @param dataLoader :: [input] Data loader (shared with MuonAnalysis)
 * @param timeZero :: [input] Time zero from MuonAnalysis interface (optional)
 * @param rebinArgs :: [input] Rebin args from MuonAnalysis interface (optional)
 */
MuonAnalysisFitDataPresenter::MuonAnalysisFitDataPresenter(
    IWorkspaceFitControl *fitBrowser, IMuonFitDataSelector *dataSelector,
    MuonAnalysisDataLoader &dataLoader, double timeZero,
    RebinOptions &rebinArgs)
    : m_fitBrowser(fitBrowser), m_dataSelector(dataSelector),
      m_dataLoader(dataLoader), m_timeZero(timeZero), m_rebinArgs(rebinArgs) {
  // Ensure this is set correctly at the start
  handleSimultaneousFitLabelChanged();
  doConnect();
}

/**
 * Connect up signals and slots
 * Abstract base class is not a QObject, so attempt a cast.
 * (Its derived classes are QObjects).
 */
void MuonAnalysisFitDataPresenter::doConnect() {
  if (const QObject *fitBrowser = dynamic_cast<QObject *>(m_fitBrowser)) {
    connect(fitBrowser, SIGNAL(fittingDone(const QString &)), this,
            SLOT(handleFitFinished(const QString &)));
    connect(fitBrowser, SIGNAL(xRangeChanged(double, double)), this,
            SLOT(handleXRangeChangedGraphically(double, double)));
    connect(fitBrowser, SIGNAL(sequentialFitRequested()), this,
            SLOT(openSequentialFitDialog()));
  }
  if (const QObject *dataSelector = dynamic_cast<QObject *>(m_dataSelector)) {
    connect(dataSelector, SIGNAL(dataPropertiesChanged()), this,
            SLOT(handleDataPropertiesChanged()));
    connect(dataSelector, SIGNAL(simulLabelChanged()), this,
            SLOT(handleSimultaneousFitLabelChanged()));
    connect(dataSelector, SIGNAL(datasetIndexChanged(int)), this,
            SLOT(handleDatasetIndexChanged(int)));
  }
}

/**
 * Called when data selector reports "data properties changed"
 * Updates WS index, startX, endX
 */
void MuonAnalysisFitDataPresenter::handleDataPropertiesChanged() {
  // update workspace index
  const unsigned int wsIndex = m_dataSelector->getWorkspaceIndex();
  m_fitBrowser->setWorkspaceIndex(static_cast<int>(wsIndex));

  // update start and end times
  const double start = m_dataSelector->getStartTime();
  const double end = m_dataSelector->getEndTime();
  m_fitBrowser->setStartX(start);
  m_fitBrowser->setEndX(end);
}

/**
 * Called when data selector reports "selected data changed"
 * @param grouping :: [input] Grouping table from interface
 * @param plotType :: [input] Type of plot currently selected in interface
 * @param overwrite :: [input] Whether overwrite is on or off in interface
 */
void MuonAnalysisFitDataPresenter::handleSelectedDataChanged(
    const Mantid::API::Grouping &grouping, const Muon::PlotType &plotType,
    bool overwrite) {
  const auto names = generateWorkspaceNames(grouping, plotType, overwrite);
  createWorkspacesToFit(names, grouping);
}

/**
 * Called when user drags lines to set fit range
 * Update the text boxes silently (no event)
 * @param start :: [input] start of fit range
 * @param end :: [input] end of fit range
 */
void MuonAnalysisFitDataPresenter::handleXRangeChangedGraphically(double start,
                                                                  double end) {
  m_dataSelector->setStartTimeQuietly(start);
  m_dataSelector->setEndTimeQuietly(end);
}

/**
 * Called by selectMultiPeak: fit browser has been reassigned to a new
 * workspace.
 * Sets run number, instrument and workspace index in the data selector.
 * If multiple runs selected, disable sequential fit option.
 * @param wsName :: [input] Name of new workspace
 */
void MuonAnalysisFitDataPresenter::setAssignedFirstRun(const QString &wsName) {
  if (wsName == m_PPAssignedFirstRun)
    return;

  m_PPAssignedFirstRun = wsName;
  // Parse workspace name here for run number and instrument name
  const auto wsParams =
      MuonAnalysisHelper::parseWorkspaceName(wsName.toStdString());
  const QString instRun = QString::fromStdString(wsParams.label);
  const int firstZero = instRun.indexOf("0");
  const QString numberString = instRun.right(instRun.size() - firstZero);
  m_dataSelector->setWorkspaceDetails(
      numberString, QString::fromStdString(wsParams.instrument));
  m_dataSelector->setWorkspaceIndex(0u); // always has only one spectrum
  // Check for multiple runs
  if (wsParams.runs.size() > 1) {
    m_fitBrowser->allowSequentialFits(false);
  } else {
    m_fitBrowser->allowSequentialFits(
        true); // will still be forbidden if no function
  }

  // Set selected groups/pairs and periods here too
  m_dataSelector->setChosenGroup(QString::fromStdString(wsParams.itemName));
  m_dataSelector->setChosenPeriod(QString::fromStdString(wsParams.periods));
}

/**
 * Creates all workspaces that don't
 * yet exist in the ADS and adds them. Sets the workspace name, which
 * sends a signal to update the peak picker.
 * @param names :: [input] Names of workspaces to create
 * @param grouping :: [input] Grouping table from interface
 */
void MuonAnalysisFitDataPresenter::createWorkspacesToFit(
    const std::vector<std::string> &names,
    const Mantid::API::Grouping &grouping) const {
  if (names.empty()) {
    m_fitBrowser->setWorkspaceNames(QStringList());
    return;
  }

  // For each name, if not in the ADS, create it
  for (const auto &name : names) {
    if (AnalysisDataService::Instance().doesExist(name)) {
      // We already have it! Leave it there
    } else {
      // Create here and add to the ADS
      std::string groupLabel;
      const auto ws = createWorkspace(name, grouping, groupLabel);
      AnalysisDataService::Instance().add(name, ws);
      if (!groupLabel.empty()) {
        MuonAnalysisHelper::groupWorkspaces(groupLabel, {name});
      }
    }
  }

  // Update model with these
  QStringList qNames;
  std::transform(
      names.begin(), names.end(), std::back_inserter(qNames),
      [](const std::string &s) { return QString::fromStdString(s); });
  m_fitBrowser->setWorkspaceNames(qNames);
  m_dataSelector->setDatasetNames(qNames);

  // Quietly update the workspace name set in the fit property browser
  // (Don't want the signal to change what's selected in the view)
  auto *fitBrowser = dynamic_cast<QObject *>(m_fitBrowser);
  if (fitBrowser) {
    fitBrowser->blockSignals(true);
  }
  m_fitBrowser->setWorkspaceName(qNames.first());
  if (fitBrowser) {
    fitBrowser->blockSignals(false);
  }
}

/**
 * Gets names of all workspaces required by asking the view
 * @param grouping :: [input] Grouping table from interface
 * @param plotType :: [input] Type of plot currently selected in interface
 * @param overwrite :: [input] Whether overwrite is on or off in interface
 * @returns :: list of workspace names
 */
std::vector<std::string> MuonAnalysisFitDataPresenter::generateWorkspaceNames(
    const Mantid::API::Grouping &grouping, const Muon::PlotType &plotType,
    bool overwrite) const {
  // From view, get names of all workspaces needed
  std::vector<std::string> workspaceNames;
  const auto groups = m_dataSelector->getChosenGroups();
  const auto periods = m_dataSelector->getPeriodSelections();

  Muon::DatasetParams params;
  QString instRuns = m_dataSelector->getInstrumentName();
  instRuns.append(m_dataSelector->getRuns());
  std::vector<int> selectedRuns;
  MuonAnalysisHelper::parseRunLabel(instRuns.toStdString(), params.instrument,
                                    selectedRuns);
  params.version = 1;
  params.plotType = plotType;

  // Find if given name is a group or a pair - defaults to group.
  // If it is not in the groups or pairs list, we will produce a workspace name
  // with "Group" in it rather than throwing.
  const auto getItemType = [&grouping](const std::string &name) {
    if (std::find(grouping.pairNames.begin(), grouping.pairNames.end(), name) !=
        grouping.pairNames.end()) {
      return Muon::ItemType::Pair;
    } else { // If it's not a pair, assume it's a group
      return Muon::ItemType::Group;
    }
  };

  // Generate a unique name from the given parameters
  const auto getUniqueName = [](Muon::DatasetParams &params) {
    std::string workspaceName =
        MuonAnalysisHelper::generateWorkspaceName(params);
    while (AnalysisDataService::Instance().doesExist(workspaceName)) {
      params.version++;
      workspaceName = MuonAnalysisHelper::generateWorkspaceName(params);
    }
    return workspaceName;
  };

  // generate workspace names
  std::vector<std::vector<int>> runNumberVectors;
  if (m_dataSelector->getFitType() == IMuonFitDataSelector::FitType::CoAdd) {
    // Analyse all the runs in one go
    runNumberVectors.push_back(selectedRuns);
  } else { // Analyse the runs one by one
    for (const int run : selectedRuns) {
      runNumberVectors.push_back({run});
    }
  }

  for (const auto runsVector : runNumberVectors) {
    params.runs = runsVector;
    for (const auto &group : groups) {
      params.itemType = getItemType(group.toStdString());
      params.itemName = group.toStdString();
      for (const auto &period : periods) {
        params.periods = period.toStdString();
        const std::string wsName =
            overwrite ? MuonAnalysisHelper::generateWorkspaceName(params)
                      : getUniqueName(params);
        workspaceNames.push_back(wsName);
      }
    }
  }
  return workspaceNames;
}

/**
 * Create an analysis workspace given the required name.
 * @param name :: [input] Name of workspace to create (in format INST0001234;
 * Pair; long; Asym; 1; #1)
 * @param grouping :: [input] Grouping table from interface
 * @param groupLabel :: [output] Label to group workspace under
 * @returns :: workspace
 */
Mantid::API::Workspace_sptr MuonAnalysisFitDataPresenter::createWorkspace(
    const std::string &name, const Mantid::API::Grouping &grouping,
    std::string &groupLabel) const {
  Mantid::API::Workspace_sptr outputWS;

  // parse name to get runs, periods, groups etc
  auto params = MuonAnalysisHelper::parseWorkspaceName(name);

  // load original data - need to get filename(s) of individual run(s)
  QStringList filenames;
  for (const int run : params.runs) {
    filenames.append(QString::fromStdString(MuonAnalysisHelper::getRunLabel(
                                                params.instrument, {run}))
                         .append(".nxs"));
  }
  try {
    // This will sum multiple runs together
    const auto loadedData = m_dataLoader.loadFiles(filenames);
    groupLabel = loadedData.label;

    // correct and group the data
    const auto correctedData =
        m_dataLoader.correctAndGroup(loadedData, grouping);

    // run analysis to generate workspace
    Muon::AnalysisOptions analysisOptions(grouping);
    // Periods
    if (params.periods.empty()) {
      analysisOptions.summedPeriods = "1";
    } else {
      std::replace(params.periods.begin(), params.periods.end(), ',', '+');
      const size_t minus = params.periods.find('-');
      analysisOptions.summedPeriods = params.periods.substr(0, minus);
      if (minus != std::string::npos && minus != params.periods.size()) {
        analysisOptions.subtractedPeriods =
            params.periods.substr(minus + 1, std::string::npos);
      }
    }

    // Rebin params: use the same as MuonAnalysis uses
    analysisOptions.rebinArgs = getRebinParams(correctedData);
    analysisOptions.loadedTimeZero = loadedData.timeZero;
    analysisOptions.timeZero = m_timeZero;
    analysisOptions.timeLimits.first = m_dataSelector->getStartTime();
    analysisOptions.timeLimits.second = m_dataSelector->getEndTime();
    analysisOptions.groupPairName = params.itemName;
    analysisOptions.plotType = params.plotType;
    outputWS =
        m_dataLoader.createAnalysisWorkspace(correctedData, analysisOptions);
  } catch (const std::exception &ex) {
    std::ostringstream err;
    err << "Failed to create analysis workspace " << name << ": " << ex.what();
    g_log.error(err.str());
  }

  return outputWS;
}

/**
 * Generates rebin parameter string from options passed in by MuonAnalysis
 * @param ws :: [input] Workspace to get bin size from
 * @returns :: parameter string for rebinning
 */
std::string
MuonAnalysisFitDataPresenter::getRebinParams(const Workspace_sptr ws) const {
  std::string params = "";
  if (m_rebinArgs.first == RebinType::FixedRebin) {
    try {
      const double step = std::stod(m_rebinArgs.second);
      auto mws = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
      if (mws) {
        const double binSize = mws->dataX(0)[1] - mws->dataX(0)[0];
        params = boost::lexical_cast<std::string>(step * binSize);
      }
    } catch (const std::exception &) {
      params = "";
    }
  } else if (m_rebinArgs.first == RebinType::VariableRebin) {
    params = m_rebinArgs.second;
  }
  return params;
}

/**
 * Set the label for simultaneous fit results based on input
 */
void MuonAnalysisFitDataPresenter::handleSimultaneousFitLabelChanged() const {
  const QString label = m_dataSelector->getSimultaneousFitLabel();
  m_fitBrowser->setSimultaneousLabel(label.toStdString());
}

/**
 * When a simultaneous fit finishes, transform the results so the results table
 * can be easily generated:
 * - rename fitted workspaces
 * - extract from group to one level up
 * - split parameter table
 * @param status :: [input] Fit status (unused)
 */
void MuonAnalysisFitDataPresenter::handleFitFinished(const QString &status) const {
  Q_UNUSED(status);
  // Test if this was a simultaneous fit, or a co-add fit with multiple
  // groups/periods
  auto &dataSelector = m_dataSelector; // local ref
  const bool isSimultaneousFit = [dataSelector]() {
    if (dataSelector->getFitType() ==
        IMuonFitDataSelector::FitType::Simultaneous) {
      return true;
    } else {
      return dataSelector->getChosenGroups().size() > 1 ||
             dataSelector->getPeriodSelections().size() > 1;
    }
  }();
  // If fitting was simultaneous, transform the results.
  if (isSimultaneousFit) {
    AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
    const auto label = m_dataSelector->getSimultaneousFitLabel();
    const auto groupName =
        MantidWidgets::MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX +
        label.toStdString();
    handleFittedWorkspaces(groupName);
    extractFittedWorkspaces(groupName);
  }
}

/**
 * Rename fitted workspaces so they can be linked to the input and found by the
 * results table generation code. Add special logs and generate params table.
 * @param groupName :: [input] Name of group that workspaces belong to
 */
void MuonAnalysisFitDataPresenter::handleFittedWorkspaces(
    const std::string &groupName) const {
  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
  const auto resultsGroup =
      ads.retrieveWS<WorkspaceGroup>(groupName + "_Workspaces");
  const auto paramsTable =
      ads.retrieveWS<ITableWorkspace>(groupName + "_Parameters");
  if (resultsGroup && paramsTable) {
    const size_t offset = paramsTable->rowCount() - resultsGroup->size();
    for (size_t i = 0; i < resultsGroup->size(); i++) {
      const std::string oldName = resultsGroup->getItem(i)->name();
      auto wsName = paramsTable->cell<std::string>(offset + i, 0);
      wsName = wsName.substr(wsName.find_first_of('=') + 1); // strip the "f0="
      const auto wsDetails = MuonAnalysisHelper::parseWorkspaceName(wsName);
      // Add group and period as log values so they appear in the table
      addSpecialLogs(oldName, wsDetails);
      // Generate new name and rename workspace
      std::ostringstream newName;
      newName << groupName << "_" << wsDetails.label << "_"
              << wsDetails.itemName << "_" << wsDetails.periods;
      ads.rename(oldName, newName.str() + "_Workspace");
      // Generate new parameters table for this dataset
      const auto fitTable = generateParametersTable(wsName, paramsTable);
      if (fitTable) {
        const std::string fitTableName = newName.str() + "_Parameters";
        ads.add(fitTableName, fitTable);
        ads.addToGroup(groupName, fitTableName);
      }
    }
    // Now that we have split parameters table, can delete it
    ads.remove(groupName + "_Parameters");
  }
}

/**
 * Moves all workspaces in group "groupName_Workspaces" up a level into
 * "groupName"
 * @param groupName :: [input] Name of upper group e.g. "MuonSimulFit_Label"
 */
void MuonAnalysisFitDataPresenter::extractFittedWorkspaces(
    const std::string &groupName) const {
  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
  const std::string resultsGroupName = groupName + "_Workspaces";
  const auto resultsGroup = ads.retrieveWS<WorkspaceGroup>(resultsGroupName);
  if (ads.doesExist(groupName) && resultsGroup) {
    for (const auto &name : resultsGroup->getNames()) {
      ads.removeFromGroup(resultsGroupName, name);
      ads.addToGroup(groupName, name);
    }
    ads.remove(resultsGroupName); // should be empty now
  }
}

/**
 * Add extra logs to the named workspace, using supplied parameters.
 * This is so they can be selected to add to the results table.
 * At present these are:
 * - "group": group name
 * - "period": period string
 * @param wsName :: [input] Name of workspace to add logs to
 * @param wsParams :: [input] Parameters to get log values from
 */
void MuonAnalysisFitDataPresenter::addSpecialLogs(
    const std::string &wsName, const Muon::DatasetParams &wsParams) const {
  auto matrixWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
  if (matrixWs) {
    matrixWs->mutableRun().addProperty<std::string>("group", wsParams.itemName);
    matrixWs->mutableRun().addProperty<std::string>("period", wsParams.periods);
  }
}

/**
 * Extract fit parameters into an individual parameters table for this dataset.
 * This enables the Results Table tab of MuonAnalysis to do its work on them.
 * @param wsName :: [input] Workspace name to extract parameters for
 * @param inputTable :: [input] Fit parameters table for all datasets
 * @returns :: individual table for the given workspace
 */
ITableWorkspace_sptr MuonAnalysisFitDataPresenter::generateParametersTable(
    const std::string &wsName, ITableWorkspace_sptr inputTable) const {
  ITableWorkspace_sptr fitTable =
      Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  auto nameCol = fitTable->addColumn("str", "Name");
  nameCol->setPlotType(6); // label
  auto valCol = fitTable->addColumn("double", "Value");
  valCol->setPlotType(2); // Y
  auto errCol = fitTable->addColumn("double", "Error");
  errCol->setPlotType(5); // YErr

  // Get "f0"/"f1" index for this workspace name
  const std::string fIndex = [&inputTable, &wsName]() {
    for (size_t i = 0; i < inputTable->rowCount(); i++) {
      auto title = inputTable->cell<std::string>(i, 0);
      const size_t eqPos = title.find_first_of('=');
      if (eqPos == std::string::npos)
        continue;
      if (title.substr(eqPos + 1) == wsName) {
        return title.substr(0, eqPos) + '.';
      }
    }
    return std::string();
  }();

  static const std::string costFuncVal = "Cost function value";
  TableRow row = inputTable->getFirstRow();
  do {
    std::string key;
    double value;
    double error;
    row >> key >> value >> error;
    const size_t fPos = key.find(fIndex);
    if (fPos != std::string::npos) {
      TableRow outputRow = fitTable->appendRow();
      outputRow << key.substr(fPos + fIndex.size()) << value << error;
    } else if (key == costFuncVal) {
      TableRow outputRow = fitTable->appendRow();
      outputRow << key << value << error;
    }
  } while (row.next());

  return fitTable;
}

/**
 * Called when user changes the selected dataset index
 * Notify model of this change, which will update function browser
 * @param index :: [input] Selected dataset index
 */
void MuonAnalysisFitDataPresenter::handleDatasetIndexChanged(int index) {
  m_fitBrowser->userChangedDataset(index);
}

/**
 * Called when user requests a sequential fit.
 * Open the dialog and set up the correct options.
 */
void MuonAnalysisFitDataPresenter::openSequentialFitDialog() {
  // Make sure we have a real fit browser, not a testing mock
  auto *fitBrowser =
      dynamic_cast<MantidWidgets::MuonFitPropertyBrowser *>(m_fitBrowser);
  if (!fitBrowser) {
    return;
  }

  // Lambda to convert QStringList to std::vector<std::string>
  const auto toVector = [](const QStringList &qsl) {
    std::vector<std::string> vec;
    std::transform(qsl.begin(), qsl.end(), std::back_inserter(vec),
                   [](const QString &qs) { return qs.toStdString(); });
    return vec;
  };

  // Open the dialog
  fitBrowser->blockSignals(true);
  MuonSequentialFitDialog dialog(
      fitBrowser, this, toVector(m_dataSelector->getChosenGroups()),
      toVector(m_dataSelector->getPeriodSelections()));
  dialog.exec();
  fitBrowser->blockSignals(false);
}

} // namespace CustomInterfaces
} // namespace MantidQt

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveDiffCal.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataHandling/H5Util.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"

#include <H5Cpp.h>
#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid::DataHandling {

using Mantid::API::FileProperty;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_const_sptr;
using Mantid::API::PropertyMode;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::GroupingWorkspace;
using Mantid::DataObjects::GroupingWorkspace_const_sptr;
using Mantid::DataObjects::GroupingWorkspace_sptr;
using Mantid::DataObjects::MaskWorkspace;
using Mantid::DataObjects::MaskWorkspace_const_sptr;
using Mantid::Kernel::Direction;

using namespace H5;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveDiffCal)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SaveDiffCal::name() const { return "SaveDiffCal"; }

/// Algorithm's version for identification. @see Algorithm::version
int SaveDiffCal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SaveDiffCal::category() const { return "DataHandling\\Instrument;Diffraction\\DataHandling"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SaveDiffCal::summary() const { return "Saves a calibration file for powder diffraction"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveDiffCal::init() {
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("CalibrationWorkspace", "", Direction::Input),
                  "An output workspace.");

  declareProperty(std::make_unique<WorkspaceProperty<GroupingWorkspace>>("GroupingWorkspace", "", Direction::Input,
                                                                         PropertyMode::Optional),
                  "Optional: A GroupingWorkspace giving the grouping info.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MaskWorkspace>>("MaskWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: A MaskWorkspace giving which detectors are masked.");

  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Save, ".h5"),
                  "Path to the .h5 file that will be created.");
}

std::map<std::string, std::string> SaveDiffCal::validateInputs() {
  std::map<std::string, std::string> result;

  ITableWorkspace_const_sptr calibrationWS = getProperty("CalibrationWorkspace");
  if (!bool(calibrationWS)) {
    result["CalibrationWorkspace"] = "Cannot save empty table";
  } else {
    size_t numRows = calibrationWS->rowCount();
    if (numRows == 0) {
      result["CalibrationWorkspace"] = "Cannot save empty table";
    } else {
      GroupingWorkspace_const_sptr groupingWS = getProperty("GroupingWorkspace");
      if (bool(groupingWS) && numRows < groupingWS->getNumberHistograms()) {
        result["GroupingWorkspace"] = "Must have equal or less number of spectra as the table has rows";
      }
      MaskWorkspace_const_sptr maskWS = getProperty("MaskWorkspace");
      if (bool(maskWS) && numRows < maskWS->getNumberHistograms()) {
        result["MaskWorkspace"] = "Must have equal or less number of spectra as the table has rows";
      }
    }
  }

  return result;
}

/**
 * Create a dataset under a given group with a given name
 * Use CalibrationWorkspace to retrieve the data
 *
 * @param group :: group parent to the dataset
 * @param name :: column name of CalibrationWorkspace, and name of the dataset
 */
void SaveDiffCal::writeDoubleFieldFromTable(H5::Group &group, const std::string &name) {
  auto column = m_calibrationWS->getColumn(name);
  // Retrieve only the first m_numValues, not necessarily the whole column
  auto data = column->numeric_fill<>(m_numValues);
  H5Util::writeArray1D(group, name, data);
}

/**
 * Create a dataset under a given group with a given name
 * Use CalibrationWorkspace to retrieve the data.
 *
 * @param group :: group parent to the dataset
 * @param name :: column name of CalibrationWorkspace, and name of the dataset
 */
void SaveDiffCal::writeIntFieldFromTable(H5::Group &group, const std::string &name) {
  auto column = m_calibrationWS->getColumn(name);
  // Retrieve only the first m_numValues, not necessarily the whole column
  auto data = column->numeric_fill<int32_t>(m_numValues);
  H5Util::writeArray1D(group, name, data);
}

/**
 * Create a dataset under a given group with a given name
 * Use GroupingWorkspace or MaskWorkspace to retrieve the data.
 *
 * @param group :: group parent to the dataset
 * @param name :: column name of the workspace, and name of the dataset
 * @param ws :: pointer to GroupingWorkspace or MaskWorkspace
 */
void SaveDiffCal::writeIntFieldFromSVWS(H5::Group &group, const std::string &name,
                                        const DataObjects::SpecialWorkspace2D_const_sptr &ws) {
  const bool isMask = (name == "use");

  // output array defaults to all one (one group, use the pixel)
  std::vector<int32_t> values(m_numValues, 1);

  if (bool(ws)) {
    for (size_t i = 0; i < m_numValues; ++i) {
      auto &ids = ws->getSpectrum(i).getDetectorIDs(); // set of detector ID's
      // check if the first detector ID in the set is in the calibration table
      auto found = m_detidToIndex.find(*(ids.begin())); // (detID, row_index)
      if (found != m_detidToIndex.end()) {
        auto value = static_cast<int32_t>(ws->getValue(found->first));
        // in maskworkspace 0=use, 1=dontuse - backwards from the file
        if (isMask) {
          if (value == 0)
            value = 1; // thus "use" means a calibrated detector, good for use
          else
            value = 0;
        }
        values[found->second] = value;
      }
    }
  }

  H5Util::writeArray1D(group, name, values);
}

void SaveDiffCal::generateDetidToIndex() {
  m_detidToIndex.clear();

  auto detidCol = m_calibrationWS->getColumn("detid");
  auto detids = detidCol->numeric_fill<detid_t>();

  const size_t numDets = detids.size();
  for (size_t i = 0; i < numDets; ++i) {
    m_detidToIndex[static_cast<detid_t>(detids[i])] = i;
  }
}

bool SaveDiffCal::tableHasColumn(const std::string &ColumnName) const {
  const std::vector<std::string> names = m_calibrationWS->getColumnNames();
  return std::any_of(names.cbegin(), names.cend(), [&ColumnName](const auto &name) { return name == ColumnName; });
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveDiffCal::exec() {
  m_calibrationWS = getProperty("CalibrationWorkspace");
  this->generateDetidToIndex();

  GroupingWorkspace_sptr groupingWS = getProperty("GroupingWorkspace");
  if (bool(groupingWS) && groupingWS->isDetectorIDMappingEmpty())
    groupingWS->buildDetectorIDMapping();

  MaskWorkspace_const_sptr maskWS = getProperty("MaskWorkspace");

  // initialize `m_numValues` as the minimum of (CalibrationWorkspace_row_count,
  // GroupingWorkspace_histogram_count, MaskWorkspace_histogram_count)
  m_numValues = m_calibrationWS->rowCount();
  if (bool(groupingWS) && groupingWS->getNumberHistograms() < m_numValues) {
    m_numValues = groupingWS->getNumberHistograms();
  }
  if (bool(maskWS) && maskWS->getNumberHistograms() < m_numValues) {
    m_numValues = maskWS->getNumberHistograms();
  }

  // delete the file if it already exists
  std::string filename = getProperty("Filename");
  if (Poco::File(filename).exists()) {
    Poco::File(filename).remove();
  }

  H5File file(filename, H5F_ACC_EXCL);

  auto calibrationGroup = H5Util::createGroupNXS(file, "calibration", "NXentry");

  // write the d-spacing to TOF conversion parameters for the selected pixels
  // as datasets under the NXentry group
  this->writeDoubleFieldFromTable(calibrationGroup, "difc");
  this->writeDoubleFieldFromTable(calibrationGroup, "difa");
  this->writeDoubleFieldFromTable(calibrationGroup, "tzero");

  this->writeIntFieldFromTable(calibrationGroup, "detid");
  if (this->tableHasColumn("dasid")) // optional field
    this->writeIntFieldFromTable(calibrationGroup, "dasid");
  else
    g_log.information("Not writing out values for \"dasid\"");

  this->writeIntFieldFromSVWS(calibrationGroup, "group", groupingWS);
  this->writeIntFieldFromSVWS(calibrationGroup, "use", maskWS);

  // check if the input calibration table has an "offset" field
  if (this->tableHasColumn("offset")) // optional field
    this->writeDoubleFieldFromTable(calibrationGroup, "offset");
  else
    g_log.information("Not writing out values for \"offset\"");

  // get the instrument information only if a GroupingWorkspace or
  // MaskWorkspace is supplied by the user
  std::string instrumentName;
  std::string instrumentSource;
  if (bool(groupingWS)) {
    instrumentName = groupingWS->getInstrument()->getName();
    instrumentSource = groupingWS->getInstrument()->getFilename();
  }
  if (bool(maskWS)) {
    if (instrumentName.empty()) {
      instrumentName = maskWS->getInstrument()->getName();
    }
    if (instrumentSource.empty()) {
      instrumentSource = maskWS->getInstrument()->getFilename();
    }
  }
  if (!instrumentSource.empty()) {
    instrumentSource = Poco::Path(instrumentSource).getFileName();
  }

  // add the instrument information
  auto instrumentGroup = calibrationGroup.createGroup("instrument");
  H5Util::writeStrAttribute(instrumentGroup, "NX_class", "NXinstrument");
  if (!instrumentName.empty()) {
    H5Util::write(instrumentGroup, "name", instrumentName);
  }
  if (!instrumentSource.empty()) {
    H5Util::write(instrumentGroup, "instrument_source", instrumentSource);
  }

  file.close();
}

} // namespace Mantid::DataHandling

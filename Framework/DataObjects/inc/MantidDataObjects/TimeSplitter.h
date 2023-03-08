// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/DllConfig.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/DateAndTime.h"

namespace Mantid {

using Types::Core::DateAndTime;

namespace Kernel {
class TimeROI;
}
using Kernel::TimeROI;

namespace DataObjects {

class MANTID_DATAOBJECTS_DLL TimeSplitter {

public:
  TimeSplitter() = default;
  TimeSplitter(const DateAndTime &start, const DateAndTime &stop);
  TimeSplitter(const Mantid::API::MatrixWorkspace_sptr &ws, const DateAndTime &offset = DateAndTime(0, 0));
  TimeSplitter(const TableWorkspace_sptr &tws, const DateAndTime &offset = DateAndTime(0, 0));
  TimeSplitter(const SplittersWorkspace_sptr &sws);

public:
  static constexpr int NO_TARGET{-1}; // no target (a.k.a. destination) workspace for filtered out events
  int valueAtTime(const DateAndTime &time) const;
  void addROI(const DateAndTime &start, const DateAndTime &stop, const int value);
  std::vector<int> outputWorkspaceIndices() const;
  TimeROI getTimeROI(const int workspaceIndex);

  /// this is to aid in testing and not intended for use elsewhere
  std::size_t numRawValues() const;

private:
  void clearAndReplace(const DateAndTime &start, const DateAndTime &stop, const int value);
  std::string debugPrint() const;
  std::map<DateAndTime, int> m_roi_map;
};
} // namespace DataObjects
} // namespace Mantid
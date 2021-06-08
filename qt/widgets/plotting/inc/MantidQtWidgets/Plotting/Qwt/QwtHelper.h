// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <qwt_data.h>

namespace MantidQt {
namespace API {
namespace QwtHelper {
/// Create Qwt curve data from a workspace
EXPORT_OPT_MANTIDQT_PLOTTING std::shared_ptr<QwtData> curveDataFromWs(const Mantid::API::MatrixWorkspace_const_sptr &ws,
                                                                      size_t wsIndex);

/// Create vector of Qwt curve data from a workspace, used for EnggDiffraction
/// GUI
EXPORT_OPT_MANTIDQT_PLOTTING std::vector<std::shared_ptr<QwtData>>
curveDataFromWs(const Mantid::API::MatrixWorkspace_const_sptr &ws);

/// Create error vector from a workspace
EXPORT_OPT_MANTIDQT_PLOTTING std::vector<double> curveErrorsFromWs(const Mantid::API::MatrixWorkspace_const_sptr &ws,
                                                                   size_t wsIndex);

/// Create Qwt curve data from a function
EXPORT_OPT_MANTIDQT_PLOTTING std::shared_ptr<QwtData>
curveDataFromFunction(const Mantid::API::IFunction_const_sptr &func, const std::vector<double> &xValues);

/// Create workspace filled with function values
EXPORT_OPT_MANTIDQT_PLOTTING Mantid::API::MatrixWorkspace_sptr
createWsFromFunction(const Mantid::API::IFunction_const_sptr &func, const std::vector<double> &xValues);

/// Creates empty Qwt curve data
EXPORT_OPT_MANTIDQT_PLOTTING std::shared_ptr<QwtData> emptyCurveData();

} // namespace QwtHelper
} // namespace API
} // namespace MantidQt

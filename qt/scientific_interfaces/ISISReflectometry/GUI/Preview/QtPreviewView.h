// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "ui_PreviewWidget.h"
#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** QtInstrumentView : Provides an interface for the "Preview" tab in the
ISIS Reflectometry interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL QtPreviewView : public QWidget {
  Q_OBJECT
public:
  QtPreviewView(QWidget *parent = nullptr);

private:
  Ui::PreviewWidget m_ui;
  void connectSignals() const;

private slots:
  void onLoadWorkspaceRequested() const;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry

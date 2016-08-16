#include "MantidHistogramData/HistogramRebin.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Histogram.h"
#include <algorithm>

using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::BinEdges;

namespace Mantid {
namespace HistogramData {

Histogram rebinCounts(const Histogram &input, const BinEdges &binEdges) {
  if (input.yMode() != Histogram::YMode::Counts ||
      input.xMode() != Histogram::XMode::BinEdges) {
    throw std::runtime_error(
        "Histogram  XMode should be BinEdges and YMode should be Counts.");
  }

  auto &xold = input.x();
  auto &yold = input.y();
  auto &eold = input.e();

  auto &xnew = binEdges.rawData();
  Counts newCounts(xnew.size() - 1);
  CountVariances newCountVariances(xnew.size() - 1);
  auto &ynew = newCounts.mutableData();
  auto &enew = newCountVariances.mutableData();

  auto size_yold = yold.size();
  auto size_ynew = ynew.size();
  size_t iold = 0, inew = 0;
  double width;

  while ((inew < size_ynew) && (iold < size_yold)) {
    auto xo_low = xold[iold];
    auto xo_high = xold[iold + 1];
    auto xn_low = xnew[inew];
    auto xn_high = xnew[inew + 1];

    if (xn_high <= xo_low)
      inew++; /* old and new bins do not overlap */
    else if (xo_high <= xn_low)
      iold++; /* old and new bins do not overlap */
    else {
      // delta is the overlap of the bins on the x axis
      auto delta = xo_high < xn_high ? xo_high : xn_high;
      delta -= xo_low > xn_low ? xo_low : xn_low;
      width = xo_high - xo_low;

      if (delta <= 0.0 || width <= 0.0) {
        throw std::runtime_error("Negative or zero bin widths not allowed.");
      }
      auto factor = 1 / width;
      ynew[inew] += yold[iold] * delta * factor;
      enew[inew] += eold[iold] * eold[iold] * delta * factor;

      if (xn_high > xo_high) {
        iold++;
      } else {
        inew++;
      }
    }
  }

  return Histogram(binEdges, newCounts,
                   CountStandardDeviations(std::move(newCountVariances)));
}

Histogram rebinFrequencies(const Histogram &input, const BinEdges &binEdges) {
  if (input.yMode() != Histogram::YMode::Frequencies ||
      input.xMode() != Histogram::XMode::BinEdges) {
    throw std::runtime_error(
        "Histogram  XMode should be BinEdges and YMode should be Frequencies.");
  }

  auto &xold = input.x();
  auto &yold = input.y();
  auto &eold = input.e();

  auto &xnew = binEdges.rawData();
  Frequencies newFrequencies(xnew.size() - 1);
  FrequencyStandardDeviations newFrequencyStdDev(xnew.size() - 1);
  auto &ynew = newFrequencies.mutableData();
  auto &enew = newFrequencyStdDev.mutableData();

  auto size_yold = yold.size();
  auto size_ynew = ynew.size();
  size_t iold = 0, inew = 0;
  double width;

  while ((inew < size_ynew) && (iold < size_yold)) {
    auto xo_low = xold[iold];
    auto xo_high = xold[iold + 1];
    auto xn_low = xnew[inew];
    auto xn_high = xnew[inew + 1];

    if (xn_high <= xo_low)
      inew++; /* old and new bins do not overlap */
    else if (xo_high <= xn_low)
      iold++; /* old and new bins do not overlap */
    else {
      //        delta is the overlap of the bins on the x axis
      auto delta = std::min(xo_high, xn_high) - std::max(xo_low, xn_low);
      width = xo_high - xo_low;
      if (delta <= 0.0 || width <= 0.0) {
        throw std::runtime_error("Negative or zero bin widths not allowed.");
      }

      ynew[inew] += yold[iold] * delta;
      enew[inew] += eold[iold] * eold[iold] * delta * width;

      if (xn_high > xo_high) {
        iold++;
      } else {
        inew++;
      }
    }
  }

  /*
  * convert back to counts/unit time
  */
  for (size_t i = 0; i < size_ynew; ++i) {
    width = xnew[i + 1] - xnew[i];
    ynew[i] /= width;
    enew[i] = sqrt(enew[i]) / width;
  }

  return Histogram(binEdges, newFrequencies, newFrequencyStdDev);
}

} // namespace HistogramData
} // namespace Mantid

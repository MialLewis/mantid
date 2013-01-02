#ifndef MANTID_ALGORITHMS_REBINRAGGEDTEST_H_
#define MANTID_ALGORITHMS_REBINRAGGEDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/RebinRagged.h"

using namespace Mantid::API;
using Mantid::Algorithms::RebinRagged;
using Mantid::MantidVec;

class RebinRaggedTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RebinRaggedTest *createSuite() { return new RebinRaggedTest(); }
  static void destroySuite( RebinRaggedTest *suite ) { delete suite; }


  void test_Init()
  {
    RebinRagged alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );
  }
  
  void test_linear_binning()
  {

    RebinRagged alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    //
    int numBins(3000);
    MantidVec xValues;
    double delta;

    alg.setOptions(numBins, false, false);
    delta = alg.determineBinning(xValues, 0, 300);
//    std::cout << "**** " << numBins << " **** " << xValues.front() << ", " << delta << ", " << xValues.back() << " ****" << std::endl;
//    std::cout << "000> " << xValues.size() << std::endl;
//    std::cout << "001> " << xValues[0] << ", " << xValues[1] << ", ..., " << xValues.back() << std::endl;
    TS_ASSERT_EQUALS(numBins, xValues.size()-1);
    TS_ASSERT_EQUALS(.1, delta);
    TS_ASSERT_EQUALS(0., xValues[0]);
    TS_ASSERT_EQUALS(0.1, xValues[1]);
    TS_ASSERT_EQUALS(300., xValues[3000]);


//    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "value") );
//    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
//    "XMin"
//    "XMax"
//    "NumberBins"
//    "LogBinning"
//    "PreserveEvents"
//    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
//    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
//    Workspace_sptr ws;
//    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
//    TS_ASSERT(ws);
//    if (!ws) return;
    
    // TODO: Check the results
    
    // Remove workspace from the data service.
//    AnalysisDataService::Instance().remove(outWSName);
  }


};


#endif /* MANTID_ALGORITHMS_REBINRAGGEDTEST_H_ */

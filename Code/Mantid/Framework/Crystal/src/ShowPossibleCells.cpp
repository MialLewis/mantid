/*WIKI* 


Given a PeaksWorkspace with a UB matrix corresponding to a Niggli reduced cell,
this algorithm will display a list of possible conventional cells.  The 
max scalar error property sets a limit on the maximum allowed error in the 
cell scalars, to restrict the list to possible cells that are a good match 
for the current reduced cell.  The list can also be forced to contain only 
the best fitting conventional cell for each Bravais lattice type, by setting 
the best only property to true.

This algorithm is based on the paper: "Lattice Symmetry and Identification 
-- The Fundamental Role of Reduced Cells in Materials Characterization", 
Alan D. Mighell, Vol. 106, Number 6, Nov-Dec 2001, Journal of Research of 
the National Institute of Standards and Technology, available from: 
nvlpubs.nist.gov/nistpubs/jres/106/6/j66mig.pdf.


*WIKI*/
#include "MantidCrystal/ShowPossibleCells.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/ScalarUtils.h"
#include "MantidGeometry/Crystal/ConventionalCell.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include <cstdio>

namespace Mantid
{
namespace Crystal
{
  Kernel::Logger& ShowPossibleCells::g_log = 
                                      Kernel::Logger::get("ShowPossibleCells");

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ShowPossibleCells)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;

  //--------------------------------------------------------------------------
  /** Constructor
   */
  ShowPossibleCells::ShowPossibleCells()
  {
  }
    
  //--------------------------------------------------------------------------
  /** Destructor
   */
  ShowPossibleCells::~ShowPossibleCells()
  {
  }

  //--------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ShowPossibleCells::initDocs()
  {
    std::string summary("Show conventional cells corresponding to the UB ");
    summary += "stored with the sample for this peaks works space.";
    this->setWikiSummary( summary );

    std::string message("NOTE: The current UB must correspond to a ");
    message += "Niggli reduced cell.";
    this->setOptionalMessage(message);
  }

  //--------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ShowPossibleCells::init()
  {
    this->declareProperty(new WorkspaceProperty<PeaksWorkspace>(
          "PeaksWorkspace","",Direction::InOut), "Input Peaks Workspace");

    BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
    mustBePositive->setLower(0.0);

    this->declareProperty(new PropertyWithValue<double>( "MaxScalarError",0.2,
          mustBePositive,Direction::Input),"Max Scalar Error (0.2)");

    this->declareProperty( "BestOnly", true, 
                           "Show at most one for each Bravais Lattice" );

    this->declareProperty(
          new PropertyWithValue<int>( "NumberOfCells", 0,
          Direction::Output), "Gets set with the number of possible cells.");
  }

  //--------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ShowPossibleCells::exec()
  {
    PeaksWorkspace_sptr ws;
    ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
         AnalysisDataService::Instance().retrieve(this->getProperty("PeaksWorkspace")) );

    if (!ws) 
    { 
      throw std::runtime_error("Could not read the peaks workspace");
    }

    OrientedLattice o_lattice = ws->mutableSample().getOrientedLattice();
    Matrix<double> UB = o_lattice.getUB();

    if ( ! IndexingUtils::CheckUB( UB ) )
    {
       throw std::runtime_error(
             "ERROR: The stored UB is not a valid orientation matrix");
    }

    double max_scalar_error = this->getProperty("MaxScalarError");
    bool   best_only        = this->getProperty("BestOnly");

    std::vector<ConventionalCell> list = ScalarUtils::GetCells( UB, best_only );

    ScalarUtils::RemoveHighErrorForms( list, max_scalar_error );

    size_t num_cells = list.size();

    // now tell the user the number of possible conventional cells:
    g_log.notice() << "Num Cells : " << num_cells << std::endl;


    std::vector<std::string> result_string_list;
    char buffer[200];

    for ( size_t i = 0; i < num_cells; i++ )
    {
      DblMatrix newUB = list[i].GetNewUB();
      std::vector<double> lat_par;
      IndexingUtils::GetLatticeParameters( newUB, lat_par );

      sprintf( buffer, std::string("Form #%2d").c_str(), list[i].GetFormNum());
      std::string message( buffer );

      sprintf( buffer, std::string(" Error: %8.4f").c_str(), list[i].GetError());
      message += std::string( buffer );

      sprintf( buffer, std::string(" %12s").c_str(), list[i].GetCellType().c_str() );
      message += std::string( buffer );

      sprintf( buffer, std::string(" %2s  ").c_str(), list[i].GetCentering().c_str() );
      message += std::string( buffer );

      sprintf( buffer,
               std::string("Lattice Parameters: %8.4f %8.4f %8.4f  %8.3f %8.3f %8.3f  %9.2f").c_str(),
               lat_par[0], lat_par[1], lat_par[2], lat_par[3], lat_par[4], lat_par[5], lat_par[6]);
      message += std::string( buffer );

      g_log.notice( std::string(message) );

      result_string_list.push_back(message);
    }

    this->setProperty("NumberOfCells", (int)num_cells );

  }


} // namespace Mantid
} // namespace Crystal


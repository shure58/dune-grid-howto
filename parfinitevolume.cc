// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#include "config.h"               // know what grids are present
#include <iostream>               // for input/output to shell
#include <fstream>                // for input/output to files
#include <vector>                 // STL vector class
#include <dune/grid/common/mcmgmapper.hh> // mapper class
#include <dune/common/mpihelper.hh> // include mpi helper class


// checks for defined gridtype and inlcudes appropriate dgfparser implementation
#include "vtkout.hh"
#include "unitcube.hh"
#include "transportproblem2.hh"
#include "initialize.hh"
#include "parfvdatahandle.hh"
#include "parevolve.hh"


//===============================================================
// the time loop function working for all types of grids
//===============================================================

//! Parameter for mapper class
template<int dim>
struct P0Layout
{
  bool contains (Dune::GeometryType gt)
  {
    if (gt.dim()==dim) return true;
    return false;
  }
};

template<class G>
void partimeloop (const G& grid, double tend)
{
  // make a mapper for codim 0 entities in the leaf grid
  Dune::LeafMultipleCodimMultipleGeomTypeMapper<G,P0Layout>
  mapper(grid);

  // allocate a vector for the concentration
  std::vector<double> c(mapper.size());

  // initialize concentration with initial values
  initialize(grid,mapper,c);
  vtkout(grid,c,"pconc",0,0.0,grid.comm().rank());

  // now do the time steps
  double t=0,dt;
  int k=0;
  const double saveInterval = 0.1;
  double saveStep = 0.1;
  int counter = 1;
  while (t<tend)
  {
    // augment time step counter
    k++;

    // apply finite volume scheme
    parevolve(grid,mapper,c,t,dt);

    // augment time
    t += dt;

    // check if data should be written
    if (t >= saveStep)
    {
      // write data
      vtkout(grid,c,"pconc",counter,t,grid.comm().rank());

      //increase counter and saveStep for next interval
      saveStep += saveInterval;
      ++counter;
    }

    // print info about time, timestep size and counter
    if (grid.comm().rank()==0)                         /*@\label{pfc:rank0}@*/
      std::cout << "k=" << k << " t=" << t << " dt=" << dt << std::endl;
  }
  vtkout(grid,c,"pconc",counter,tend,grid.comm().rank());
}

//===============================================================
// The main function creates objects and does the time loop
//===============================================================

int main (int argc , char ** argv)
{
  // initialize MPI, finalize is done automatically on exit
  Dune::MPIHelper::instance(argc,argv);

  // start try/catch block to get error messages from dune
  try {
    using namespace Dune;

    UnitCube<YaspGrid<2>,64> uc;
    uc.grid().globalRefine(2);
    partimeloop(uc.grid(),0.5);

    /* To use an alternative grid implementations for parallel computations,
       uncomment exactly one definition of uc2 and the line below. */
    //    #define LOAD_BALANCING

    //  UGGrid supports parallelization in 2 or 3 dimensions
#if HAVE_UG
    //    typedef UGGrid< 2 > GridType;
    //    UnitCube< GridType, 2 > uc2;
#endif

    //  ALUGRID supports parallelization in 3 dimensions only
#if HAVE_ALUGRID
    //    typedef ALUCubeGrid< 3, 3 > GridType;
    //    typedef ALUSimplexGrid< 3, 3 > GridType;
    //    UnitCube< GridType , 1 > uc2;
#endif

#ifdef LOAD_BALANCING

    // refine grid until upper limit of level
    uc2.grid().globalRefine( 6 );

    // re-partition grid for better load balancing
    uc2.grid().loadBalance();                               /*@\label{pfv:lb}@*/

    // do time loop until end time 0.5
    partimeloop(uc2.grid(), 0.5);
#endif

  }
  catch (std::exception & e) {
    std::cout << "STL ERROR: " << e.what() << std::endl;
    return 1;
  }
  catch (Dune::Exception & e) {
    std::cout << "DUNE ERROR: " << e.what() << std::endl;
    return 1;
  }
  catch (...) {
    std::cout << "Unknown ERROR" << std::endl;
    return 1;
  }

  // done
  return 0;
}

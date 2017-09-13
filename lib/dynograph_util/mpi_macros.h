//
// Created by ehein6 on 2/16/17.
//

#ifndef DYNOGRAPH_MPI_MACROS_H
#define DYNOGRAPH_MPI_MACROS_H

#ifdef USE_MPI
#include <boost/mpi.hpp>
#include <boost/mpi/collectives.hpp>

#define MPI_RANK_0_ONLY \
if (::boost::mpi::communicator().rank() == 0)

#define MPI_BROADCAST_RESULT(X) \
::boost::mpi::broadcast(boost::mpi::communicator(), X, 0)

#define MPI_BARRIER() \
::boost::mpi::communicator().barrier()

#else

#define MPI_RANK_0_ONLY \
if (true)
#define MPI_BROADCAST_RESULT(X)
#define MPI_BARRIER()

#endif

#endif //DYNOGRAPH_MPI_MACROS_H_H

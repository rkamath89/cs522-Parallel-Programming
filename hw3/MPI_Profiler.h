#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

int MPI_Init(int* argc,char*** argv);
int MPI_Finalize();
int MPI_Barrier(MPI_Comm comm);
int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,MPI_Comm comm);
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,MPI_Comm comm, MPI_Status *status);
int MPI_Pcontrol(int killNode,...);
int MPI_Comm_rank( MPI_Comm comm, int *rank );
int MPI_Comm_size( MPI_Comm comm, int *size );
int isNodeAlive(int rank);
int fetchDestinationInOppositeGroup(int myActualRank,int destination);
int fetchDestinationInMyGroup(int myActualRank,int destination);

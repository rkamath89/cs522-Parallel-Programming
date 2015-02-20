#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

#define _MPI_Init 0
#define _MPI_Barrier 1
#define _MPI_Send 2
#define _MPI_Recv 3
#define _MPI_Isend 4
#define _MPI_Irecv 5
#define _MPI_Scatter 6
#define _MPI_Gather 7
#define _MPI_Reduce 8
#define _MPI_Allreduce 9
#define _MPI_Wait 10
#define _MPI_Waitall 11
#define _MPI_Alltoall 12
#define _MPI_Finalize 13
#define _NUMBER_OF_MPI_OPERTAION 14
#define _STATS_DAT "stats.dat"
#define _CRIT_PATH_OUT_ "critPath.out"
#define _DOT_GRAPH_TXT_ "dotGraph.dot"
#define _DOT_GRAPH_PNG_ "dotGraph.png"
#define _DOT_GRAPH_DOT_ "dotGraph.dot"

int countOfOperation[14];//To Keep track of all the Operations
struct criticalPathData *pathToDestination;

/*
 * Arrays that will maintain the wall Clock time Between Execution
 */
int *sendWallClock = NULL,sendWallClockPos =0,totalSendWallClock =0;
int *iSendWallClock = NULL, iSendWallClockPos =0,totalISendWallClock=0;
int *recvWallClock =NULL,recvWallClockpos =0,totalRecvWallClock=0;
int *iRecvWallClock = NULL,iRecvWallClockPos=0,totalIRecvWallClock=0;
int *barrierWallClock = NULL,barrierWallClockPos=0,totalBarrierWallClock=0;
int *initWallClock = NULL,initWallClockPos=0,totalInitWallClock=0;
int *scatterWallClock = NULL,scatterWallClockPos=0,totalScatterWallClock=0;
int *gatherWallClock = NULL,gatherWallClockPos=0,totalGatherWallClock=0;
int *reduceWallClock = NULL,reduceWallClockPos,totalReduceWallClock=0;
int *allReduceWallClock = NULL,allReduceWallClockPos,totalAllReduceWallClock=0;
int *waitWallClock = NULL,waitWallClockPos,totalWaitWallClock=0;
int *waitAllWallClock = NULL,waitAllWallClockPos,totalWaitAllWallClock=0;
int *allToAllWallClock = NULL,allToAllWallClockPos,totalAllToAllWallClock=0;
int **criticalPathLinkExists = NULL;

struct vertexNode
{
	char operationName[30];
	char *conctanatedName;
	int source;
	int	destination;
	int count;
	long msgSize;
	int tag;
	int latency;// Time from Regression
	int globalTimeElapsed;// Time between point to point
	int positionInAdjacencymatrix;
	int totalTime;
	//struct vertexNode *nextNode;
};
struct criticalPathData
{
	char operationName[30];
	int myRank;
	int msgSize;
	int totalTime;
};

int MPI_Init(int* argc,char*** argv);
int MPI_Finalize();
int MPI_Barrier(MPI_Comm comm);
int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,MPI_Comm comm);
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,MPI_Comm comm, MPI_Status *status);
int ** initializeValues(int numNodes,int defaultValue);
void initializeVariousArray(int numnodes);
void setTimeElapsedBetweenCalls();
void getNumberOfNodesToCreate(int numNodes);
void createNodePerRank(int numNodes,char * fileName,int myRank);
void printTheNodes(int numNodes);
void printContentsOfArrayOfNodes(int numNodes);
void createAdjacencyMatrix(int numnodes);
int findPositionBasedOnRank(char *operationName,int tag,int count,int source,int destination,int rank);
int findPositionOfCorrespondingAction(char *operationName,int tag,int count,int source,int destination,int rank);
int MPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,MPI_Op op, int root, MPI_Comm comm);
int createDotGraphNodes(int myRank,int j,int numNodes,int structureCreated);
int isCollectiveOperation(char * opName);
void  createDotGraphNodesForCollectives();
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source,int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,MPI_Comm comm, MPI_Request *request);
int MPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,int root, MPI_Comm comm);
int MPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,MPI_Comm comm);
int MPI_Wait(MPI_Request *request, MPI_Status *status);
int MPI_Waitall(int count, MPI_Request array_of_requests[],MPI_Status *array_of_statuses);
void countNumberOfInvocation(char *opName);
void fillUpCountArrayWithTime(int numNodes);
void createTheCountArrayForEachOperation();
void printNumberOfOperation();
void generateReport();
void quicksort(int *unsortedArray,int first,int last);
void sortTimeArray();
void printContentsOfTimeArray();
int* findCriticalPath(int workingCol, int **pathFound, int *maxLenghtFound);
int getCountOfElementsInFath(int *path);
void storeDataForCriticalPath(int myRank,int j);
void generateDotGraph(int numnodes);



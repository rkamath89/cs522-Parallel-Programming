#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include "MPI_Profiler.h"

double globalStartTime =0,globalEndTime=0,globalTimeElapsed=0,startTime=0,endTime=0,totalTime=0;
int numberOfOperations=0,recvCount=1,isRecvInitialized=0,waitCount=1;barrierCount=1,scatterCount=1,gatherCount=1,reduceCount=1;sendCount=1,isSendInitialized=0,positionInAdjacencyMatrix=0;
int allToAllCount=0,allReduceCount=0;
int isIsendInititalized=0,isIrecvInitialied=0;
int **adjacencyMatrix,**sendArray,**recvArray,**ISendArray,**IRecvArray,**skipValues;
int * nodesPerRank ; // Maintains the # operations per rank , array of values
struct vertexNode **arrayOfNodes ;// To keep track of node and its information
double isStartWallSet=0,isEndWallSet=0;


int ** initializeValues(int numNodes,int defaultValue)
{
	int **index;
	int *matrixBlock;
	int i=0;

	matrixBlock = (int*)malloc( (numNodes)*(numNodes)*sizeof(int));
	index = (int**)malloc( (numNodes)*sizeof(int*));
	for(i = 0 ;	 i < numNodes;i++)
	{
		index[i]=&(matrixBlock[i*(numNodes)]);
	}
	int j;
	for(i = 0 ;	 i < numNodes;i++)
	{
		for(j = 0 ;	 j < numNodes;j++)
		{
			index[i][j] = defaultValue;
		}
	}
	return index;

}
int MPI_Init(int* argc,char*** argv)
{
	int numnodes =0;
	FILE * fp;
	// Code to delete all files initially
	int i =0;
	char del_file_name[30];
	char del_file_name_2[30];
	for(i=0;i<16;i++)
	{		
		sprintf(del_file_name,"ResultForRank%d.txt",i);
		sprintf(del_file_name_2,"countRank%d.txt",i);


		remove(del_file_name_2);
		remove(del_file_name);
	}
	remove(_CRIT_PATH_OUT_);
	remove(_STATS_DAT);
	remove(_DOT_GRAPH_DOT_);
	remove(_DOT_GRAPH_PNG_);
	remove(_DOT_GRAPH_TXT_);
	remove("result.txt"); // Delete the Result file [ Just for Debugging ]
	//End
	startTime = MPI_Wtime();
	PMPI_Init(argc,argv);
	endTime = MPI_Wtime();
	totalTime = (endTime-startTime);
	int myrank;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	MPI_Comm_size(MPI_COMM_WORLD,&numnodes);

	char* file_name = "ResultForRank0.txt";
	if(myrank == 0)
	{
		numberOfOperations++;
		file_name = "ResultForRank0.txt";
	}
	else
	{
		numberOfOperations++;
		char file[30];
		sprintf(file,"ResultForRank%d.txt",myrank);
		file_name = file;
	}
	if(myrank == 0)
	{
		fp = fopen(file_name,"a");
		//fprintf(fp,"%s\n","     ");
		fprintf(fp,"\r%s %d %d %d %d %d %d %d %d","MPI_Init",0,0,0,0,0,0,0,(int)round(totalTime));
		fclose(fp);
	}
	else
	{
		fp = fopen(file_name,"a");
		fprintf(fp,"\r%s %d %d %d %d %d %d %d %d","MPI_Init",0,0,0,0,0,0,0,(int)round(totalTime));
		fclose(fp);
	}

	initializeVariousArray(numnodes); // Initialize sequence counter
	if(isStartWallSet == 0)// Wall clock was not started , start it now
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}
}
int MPI_Finalize()
{
	if(isStartWallSet == 1)
	{
		globalEndTime = MPI_Wtime();
		isStartWallSet = 0;
	}
	FILE * fp;
	FILE *opCount;
	int myrank,numnodes;
	char* file_name = "ResultForRank0.txt";
	char* file_operation_count = "countRank0.txt";
	char *dotfileName = _DOT_GRAPH_TXT_;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	MPI_Comm_size(MPI_COMM_WORLD,&numnodes);
	if(myrank == 0)
	{
		numberOfOperations++;
		file_name = "ResultForRank0.txt";
	}
	else
	{
		//file_name="test.txt";
		numberOfOperations++;
		char file[30],file_op_count[30];
		sprintf(file,"ResultForRank%d.txt",myrank);
		sprintf(file_op_count,"countRank%d.txt",myrank);
		file_name = file;
		file_operation_count=file_op_count;

	}
	PMPI_Barrier(MPI_COMM_WORLD);

	globalTimeElapsed = (globalEndTime-globalStartTime) ;
	if(myrank == 0) // Write Finalize only for Rank 0
	{


		FILE *dotFp;
		if((dotFp = fopen(dotfileName,"a")) != NULL)
		{
			fprintf(dotFp,"\r\ndigraph dotGraph {\n");
		}
		fclose(dotFp);

		fp = fopen(file_name,"a");
		fprintf(fp,"\r\n%s %d %d %d %d %d %d %d %d","MPI_Finalize",0,0,0,0,0,0,(int)round(globalTimeElapsed),0);
		fclose(fp);
		// Save the Number of operations based Per Rank
		opCount = fopen(file_operation_count,"a");

		fprintf(opCount,"%d",numberOfOperations);
		fclose(opCount);

	}
	else
	{
		fp = fopen(file_name,"a");

		fprintf(fp,"\r\n%s %d %d %d %d %d %d %d %d","MPI_Finalize",0,0,0,0,0,0,(int)round(globalTimeElapsed),0);
		fclose(fp);

		opCount = fopen(file_operation_count,"a");

		fprintf(opCount,"%d",numberOfOperations);
		fclose(opCount);

	}
	PMPI_Barrier(MPI_COMM_WORLD);

	PMPI_Finalize();


	if(myrank == 0)
	{
		char fileName[30];
		int i;
		//printf("\nFetch Nodes\n");
		nodesPerRank = (int*)malloc(sizeof(int)*numnodes);
		getNumberOfNodesToCreate(numnodes);
		//printContentsOfArrayOfNodes(numnodes);
		for(i=0;i<numnodes;i++)
		{
			sprintf(fileName,"ResultForRank%d.txt",i);
			createNodePerRank(numnodes,fileName,i);
			//printTheNodes(numnodes);
		}
		if(myrank == 0)
			createDotGraphNodesForCollectives(); // Create only the Collective nodes ( BARRIER , Scatter,Gatther etc )


		printContentsOfArrayOfNodes(numnodes); // Used For Debugging No relevance to Assignment .
		createAdjacencyMatrix(numnodes); // Creates Edged For Dot also
		// Create the Structure to create critPath.out
		pathToDestination = (struct criticalPathData*)malloc(sizeof(struct criticalPathData) * positionInAdjacencyMatrix);
		//END

		// COde to Create statistic file

		createTheCountArrayForEachOperation();

		fillUpCountArrayWithTime(numnodes);
		sortTimeArray();
		//printContentsOfTimeArray();
		generateReport();
		// END

		// Critical Path
		int ** criticalPath = initializeValues(positionInAdjacencyMatrix,-1);
		int *maxLengthFound = (int*)malloc(positionInAdjacencyMatrix * sizeof(int));
		int loop=0;
		for(loop=0;loop < positionInAdjacencyMatrix ;loop++)
		{
			maxLengthFound[loop] = -1;
		}
		int finalizeCol = nodesPerRank[0]-1;
		//printf("\n Col for Finalize %d",finalizeCol);
		maxLengthFound = findCriticalPath(finalizeCol,criticalPath,maxLengthFound);
		// Criticcal Path was Found


		// Code to handle Coloring of Path
		criticalPathLinkExists = initializeValues(positionInAdjacencyMatrix,-1);
		// END coloring initialization
		printf("\n");
		// Variables For DotGraph
		/*FILE *dotFp;
		char *dotfileName = _DOT_GRAPH_TXT_;
		dotFp = fopen(dotfileName,"a");*/
		// End Of Dot Graph Variables

		int iter=0;
		FILE *crit;
		char *crit_file =_CRIT_PATH_OUT_;
		crit = fopen(crit_file,"a");
		int prev =0,skip=0,duplicate=0,finalizePosition=0;
		while(maxLengthFound[iter] != -1)
		{
			duplicate = 0;
			struct criticalPathData tempData ;
			struct criticalPathData removeData ;
			//printf("%d->", maxLengthFound[iter]);
			int pos = maxLengthFound[iter];
			// dot file Logic
			criticalPathLinkExists[prev][pos] = 1;// Indcate that path exists
			prev = pos;
			// End Dot file Logic
			int myRankCritPath =-1;
			// END OF DOT FILE*/
			tempData = pathToDestination[pos];
			removeData.myRank = -999;
			if(tempData.myRank == -999)
			{
				duplicate =1;
			}

			if(isCollectiveOperation(tempData.operationName) || isInitOrFinalize(tempData.operationName))
			{
				//printf("\n Pos :: %d , opNmae :: %s, Duplicate :: %d",pos,tempData.operationName,duplicate);
				if((duplicate != 1) && ((strcmp("MPI_Init",tempData.operationName) == 0) || (strcmp("Init",tempData.operationName) == 0)))
				{
					strcpy(tempData.operationName,"MPI_Init"); // Some issue with the Name stores it as Init , mostly while parsing file
				}
				else if((duplicate != 1) && (strcmp("MPI_Finalize",tempData.operationName) == 0))
				{
					skip = 1;
					finalizePosition = pos;
				}
				if((skip != 1) && (duplicate != 1))// Skip Finalie Write it int the end
				{
					pathToDestination[pos] = removeData;
					fprintf(crit,"\r\n%s %d",tempData.operationName,myRankCritPath);
					fprintf(crit,"\r\n%d",tempData.totalTime);
				}
			}
			else
			{
				if((duplicate != 1) && ((strcmp("MPI_Send",tempData.operationName)==0) || (strcmp("MPI_Isend",tempData.operationName)==0)))
				{
					pathToDestination[pos] = removeData;
					fprintf(crit,"\r\n%s %d",tempData.operationName,tempData.myRank);
					fprintf(crit,"\r\n%d",tempData.msgSize);
				}
				else
				{
					if(duplicate !=1 )
					{
						pathToDestination[pos] = removeData;
						fprintf(crit,"\r\n%s %d",tempData.operationName,tempData.myRank);
						fprintf(crit,"\r\n%d",tempData.totalTime);
					}
				}
			}

			iter++;
		}
		if(skip ==1 && finalizePosition != 0)// we skipped finalize to call it here
		{
			struct criticalPathData finalizeData = pathToDestination[finalizePosition];
			fprintf(crit,"\r\n%s %d",finalizeData.operationName,-1);
		}
		fclose(crit);
		//New Logic for Critical path
		generateDotGraph(numnodes);

		//END
		// Closing braces and color for Dot File

		//Dot File closed

		printf("\n");

		// End
		system("dot -Tpng -o dotGraph.png dotGraph.dot");
		printf("\n Completed\n");

	}
	if(isStartWallSet == 0)
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}

}
int MPI_Allreduce(void *sendbuf, void *recvbuf, int count,MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{

	//sleep(1);
	// Check if wall was started if yes set end time and get the elapsed time , reset both start and end
	if(isStartWallSet == 1)
	{
		globalEndTime = MPI_Wtime();
		isStartWallSet = 0;
	}
	numberOfOperations++;
	//setTimeElapsedBetweenCalls();
	char* file_name;
	int myrank;
	//printf("MPI_Barrier");

	double startTime,endTime;
	FILE * fp;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	if(myrank == 0)
	{

		file_name = "ResultForRank0.txt";
	}
	else
	{
		//file_name="test.txt";
		char file[30];
		sprintf(file,"ResultForRank%d.txt",myrank);
		file_name = file;
	}
	//sleep(1);
	startTime = MPI_Wtime();
	PMPI_Allreduce(sendbuf,recvbuf,count,datatype,op,comm);
	endTime = MPI_Wtime();
	totalTime = (endTime-startTime);

	globalTimeElapsed = (globalEndTime-globalStartTime) ;
	fp = fopen(file_name,"a");
	fprintf(fp,"\r\n%s %d %d %d %d %d %d %d %d","MPI_Allreduce",0,myrank,0,allReduceCount++,0,0,(int)round(globalTimeElapsed),(int)round(totalTime));
	fclose(fp);

	if(isStartWallSet == 0)
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}

}
int MPI_Alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,MPI_Comm comm)
{
	//sleep(1);
	// Check if wall was started if yes set end time and get the elapsed time , reset both start and end
	if(isStartWallSet == 1)
	{
		globalEndTime = MPI_Wtime();
		isStartWallSet = 0;
	}
	numberOfOperations++;
	//setTimeElapsedBetweenCalls();
	char* file_name;
	int myrank;
	//printf("MPI_Barrier");

	double startTime,endTime;
	FILE * fp;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	if(myrank == 0)
	{

		file_name = "ResultForRank0.txt";
	}
	else
	{
		//file_name="test.txt";
		char file[30];
		sprintf(file,"ResultForRank%d.txt",myrank);
		file_name = file;
	}
	//sleep(1);
	startTime = MPI_Wtime();
	PMPI_Alltoall(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,comm);
	endTime = MPI_Wtime();
	totalTime = (endTime-startTime);

	globalTimeElapsed = (globalEndTime-globalStartTime) ;
	fp = fopen(file_name,"a");
	fprintf(fp,"\r\n%s %d %d %d %d %d %d %d %d","MPI_Alltoall",0,myrank,0,allToAllCount++,0,0,(int)round(globalTimeElapsed),(int)round(totalTime));
	fclose(fp);

	if(isStartWallSet == 0)
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}
}
int MPI_Barrier(MPI_Comm comm)
{
	//sleep(1);
	// Check if wall was started if yes set end time and get the elapsed time , reset both start and end
	if(isStartWallSet == 1)
	{
		globalEndTime = MPI_Wtime();
		isStartWallSet = 0;
	}
	numberOfOperations++;
	//setTimeElapsedBetweenCalls();
	char* file_name;
	int myrank;
	//printf("MPI_Barrier");

	double startTime,endTime;
	FILE * fp;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	if(myrank == 0)
	{

		file_name = "ResultForRank0.txt";
	}
	else
	{
		//file_name="test.txt";
		char file[30];
		sprintf(file,"ResultForRank%d.txt",myrank);
		file_name = file;
	}
	//sleep(1);
	startTime = MPI_Wtime();
	PMPI_Barrier(comm);
	endTime = MPI_Wtime();
	totalTime = (endTime-startTime);

	globalTimeElapsed = (globalEndTime-globalStartTime) ;
	fp = fopen(file_name,"a");
	fprintf(fp,"\r\n%s %d %d %d %d %d %d %d %d","MPI_Barrier",0,myrank,0,barrierCount++,0,0,(int)round(globalTimeElapsed),(int)round(totalTime));
	fclose(fp);

	if(isStartWallSet == 0)
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}
}
int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
	if(isStartWallSet == 1)
	{
		globalEndTime = MPI_Wtime();
		isStartWallSet = 0;
	}
	numberOfOperations++;
	int myrank;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	char *file_name;
	FILE * fp;

	startTime = MPI_Wtime();
	PMPI_Wait(request,status);
	endTime = MPI_Wtime();
	totalTime = (endTime-startTime);

	if(myrank == 0)
	{
		file_name = "ResultForRank0.txt";
	}
	else
	{
		//file_name="test.txt";
		char file[30];
		sprintf(file,"ResultForRank%d.txt",myrank);
		file_name = file;
	}
	globalTimeElapsed = (globalEndTime-globalStartTime) ;
	fp = fopen(file_name,"a");
	fprintf(fp,"\r\n%s %d %d %d %d %d %d %d %d","MPI_Wait",0,myrank,0,waitCount++,0,0,(int)round(globalTimeElapsed),(int)round(totalTime));
	fclose(fp);
	if(isStartWallSet == 0)
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}
}
int MPI_Waitall(int count, MPI_Request array_of_requests[],MPI_Status *array_of_statuses)
{
	if(isStartWallSet == 1)
	{
		globalEndTime = MPI_Wtime();
		isStartWallSet = 0;
	}
	int i=0;
	for (i=0; i<count; i++)
	{
		MPI_Wait(&array_of_requests[i],array_of_statuses);
	}
	globalTimeElapsed = (globalEndTime-globalStartTime) ;
	//PMPI_Waitall(count,array_of_requests,array_of_statuses);
	if(isStartWallSet == 0)
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}
}
int MPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,MPI_Comm comm)
{
	if(isStartWallSet == 1)
	{
		globalEndTime = MPI_Wtime();
		isStartWallSet = 0;
	}
	numberOfOperations++;
	int myrank;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	char *file_name;
	FILE * fp;
	long messageSize = sendcount * sizeof(sendtype);

	startTime = MPI_Wtime();
	PMPI_Scatter(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,root,comm);
	endTime = MPI_Wtime();
	totalTime = (endTime-startTime);

	if(myrank == 0)
	{
		file_name = "ResultForRank0.txt";
	}
	else
	{
		//file_name="test.txt";
		char file[30];
		sprintf(file,"ResultForRank%d.txt",myrank);
		file_name = file;
	}
	globalTimeElapsed = (globalEndTime-globalStartTime) ;
	fp = fopen(file_name,"a");
	fprintf(fp,"\r\n%s %d %d %d %d %ld %d %d %d","MPI_Scatter",0,myrank,0,scatterCount++,messageSize,0,(int)round(globalTimeElapsed),(int)round(totalTime));
	fclose(fp);
	if(isStartWallSet == 0)
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}
}
int MPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,int root, MPI_Comm comm)
{
	if(isStartWallSet == 1)
	{
		globalEndTime = MPI_Wtime();
		isStartWallSet = 0;
	}
	numberOfOperations++;
	int myrank;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	char *file_name;
	FILE * fp;
	long messageSize = sendcount * sizeof(sendtype);
	startTime = MPI_Wtime();
	PMPI_Gather(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,root,comm);
	endTime = MPI_Wtime();
	totalTime = (endTime-startTime);
	if(myrank == 0)
	{
		file_name = "ResultForRank0.txt";
	}
	else
	{
		//file_name="test.txt";
		char file[30];
		sprintf(file,"ResultForRank%d.txt",myrank);
		file_name = file;
	}
	globalTimeElapsed = (globalEndTime-globalStartTime) ;
	fp = fopen(file_name,"a");
	fprintf(fp,"\r\n%s %d %d %d %d %ld %d %d %d","MPI_Gather",0,myrank,0,gatherCount++,messageSize,0,(int)round(globalTimeElapsed),(int)round(totalTime));
	fclose(fp);
	if(isStartWallSet == 0)
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}
}
int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,MPI_Comm comm, MPI_Request *request)
{
	if(isStartWallSet == 1)
	{
		globalEndTime = MPI_Wtime();
		isStartWallSet = 0;
	}
	numberOfOperations++;
	int myrank;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	char *file_name;
	FILE * fp;
	long messageSize = count * sizeof(datatype);

	startTime = MPI_Wtime();
	PMPI_Isend(buf,count,datatype,dest,tag,comm,request);
	endTime = MPI_Wtime();
	totalTime = (endTime-startTime);

	if(myrank == 0)
	{
		file_name = "ResultForRank0.txt";
	}
	else
	{
		//file_name="test.txt";
		char file[30];
		sprintf(file,"ResultForRank%d.txt",myrank);
		file_name = file;
	}
	fp = fopen(file_name,"a");
	//FunctionName	TotalTime	MessageSize	Tag
	//Eqn for Latency
	globalTimeElapsed = (globalEndTime-globalStartTime) ;
	int latency = ( 0.000000302856 * messageSize ) + 0.000588765;
	fprintf(fp,"\r\n%s %d %d %d %d %ld %d %d %d","MPI_Isend",tag,myrank,dest,ISendArray[myrank][dest],messageSize,latency,(int)round(globalTimeElapsed),(int)round(totalTime));
	ISendArray[myrank][dest] = ISendArray[myrank][dest]+1;
	fclose(fp);
	if(isStartWallSet == 0)
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}
}
int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,MPI_Comm comm)
{
	if(isStartWallSet == 1)
	{
		globalEndTime = MPI_Wtime();
		isStartWallSet = 0;
	}
	//sleep(1);
	numberOfOperations++;

	char *file_name;
	double startTime,endTime;
	FILE * fp;
	long messageSize = count * sizeof(datatype);

	int myrank;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	startTime = MPI_Wtime();
	PMPI_Send(buf,count,datatype,dest,tag,comm);
	endTime = MPI_Wtime();
	totalTime = (endTime-startTime);
	if(myrank == 0)
	{
		file_name = "ResultForRank0.txt";
	}
	else
	{
		//file_name="test.txt";
		char file[30];
		sprintf(file,"ResultForRank%d.txt",myrank);
		file_name = file;
	}
	fp = fopen(file_name,"a");
	//FunctionName	TotalTime	MessageSize	Tag
	//Eqn for Latency
	globalTimeElapsed = (globalEndTime-globalStartTime) ;
	int latency = ( 0.000000302856 * messageSize ) + 0.000588765;
	fprintf(fp,"\r\n%s %d %d %d %d %ld %d %d %d","MPI_Send",tag,myrank,dest,sendArray[myrank][dest],messageSize,latency,(int)round(globalTimeElapsed),(int)round(totalTime));
	sendArray[myrank][dest] = sendArray[myrank][dest]+1;
	fclose(fp);
	if(isStartWallSet == 0)
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}
}
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status)
{
	if(isStartWallSet == 1)
	{
		globalEndTime = MPI_Wtime();
		isStartWallSet = 0;
	}
	numberOfOperations++;
	//printf("\nMPI_Recv");
	//setTimeElapsedBetweenCalls();
	char *file_name;
	FILE * fp;
	long messageSize = count * sizeof(datatype);

	int myrank;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	startTime = MPI_Wtime();
	PMPI_Recv(buf,count,datatype, source, tag, comm, status);
	endTime = MPI_Wtime();
	totalTime = (endTime-startTime);
	if(myrank == 0)
	{
		file_name = "ResultForRank0.txt";
	}
	else
	{
		char file[30];
		sprintf(file,"ResultForRank%d.txt",myrank);
		file_name = file;
		//sprintf(file_name,"ResultForRank%d.txt",source);
	}
	fp = fopen(file_name,"a");
	//FunctionName	TotalTime	MessageSize	Tag
	globalTimeElapsed = (globalEndTime-globalStartTime) ;
	int latency = ( 0.000000302856 * messageSize ) + 0.000588765;
	fprintf(fp,"\r\n%s %d %d %d %d %ld %d %d %d","MPI_Recv",tag,source,myrank,recvArray[source][myrank],messageSize,latency,(int)round(globalTimeElapsed),(int)round(totalTime));
	recvArray[source][myrank] = recvArray[source][myrank] +1;
	fclose(fp);
	if(isStartWallSet == 0)
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}
}
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source,int tag, MPI_Comm comm, MPI_Request *request)
{
	if(isStartWallSet == 1)
	{
		globalEndTime = MPI_Wtime();
		isStartWallSet = 0;
	}
	numberOfOperations++;
	FILE * fp;
	char *file_name;
	int myrank;
	long messageSize = count * sizeof(datatype);
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	startTime = MPI_Wtime();
	PMPI_Irecv(buf,count,datatype,source,tag,comm,request);
	endTime = MPI_Wtime();
	totalTime = (endTime-startTime);
	if(myrank == 0)
	{
		file_name = "ResultForRank0.txt";
	}
	else
	{
		char file[30];
		sprintf(file,"ResultForRank%d.txt",myrank);
		file_name = file;
		//sprintf(file_name,"ResultForRank%d.txt",source);
	}
	fp = fopen(file_name,"a");
	//FunctionName	TotalTime	MessageSize	Tag
	globalTimeElapsed = (globalEndTime-globalStartTime) ;
	int latency = ( 0.000000302856 * messageSize ) + 0.000588765;
	fprintf(fp,"\r\n%s %d %d %d %d %ld %d %d %d","MPI_Irecv",tag,source,myrank,IRecvArray[source][myrank],messageSize,latency,(int)round(globalTimeElapsed),(int)round(totalTime));
	IRecvArray[source][myrank] = IRecvArray[source][myrank] +1;
	fclose(fp);
	if(isStartWallSet == 0)
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}
}
int MPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,MPI_Op op, int root, MPI_Comm comm)
{
	if(isStartWallSet == 1)
	{
		globalEndTime = MPI_Wtime();
		isStartWallSet = 0;
	}
	numberOfOperations++;
	//setTimeElapsedBetweenCalls();

	char *file_name;
	FILE * fp;

	int myrank;
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	startTime = MPI_Wtime();
	PMPI_Reduce(sendbuf,recvbuf,count,datatype,op,root,comm);
	endTime = MPI_Wtime();
	totalTime = (endTime-startTime);
	if(myrank == 0)
	{
		file_name = "ResultForRank0.txt";
	}
	else
	{
		char file[30];
		sprintf(file,"ResultForRank%d.txt",myrank);
		file_name = file;
	}
	globalTimeElapsed = (globalEndTime-globalStartTime) ;
	fp = fopen(file_name,"a");
	fprintf(fp,"\r\n%s %d %d %d %d %d %d %d %d","MPI_Reduce",0,myrank,0,reduceCount++,0,0,(int)round(globalTimeElapsed),(int)round(totalTime));
	fclose(fp);
	if(isStartWallSet == 0)
	{
		globalStartTime = MPI_Wtime();
		isStartWallSet = 1;
	}
}
void initializeVariousArray(int numnodes)
{
	if(isSendInitialized != 1 )
	{
		isSendInitialized = 1;
		sendArray =  initializeValues(numnodes,1);
	}
	if(isRecvInitialized != 1 )
	{
		isRecvInitialized = 1;
		recvArray =  initializeValues(numnodes,1);
	}
	if(isIsendInititalized != 1)
	{
		isIsendInititalized = 1;
		ISendArray = initializeValues(numnodes,1);
	}
	if(isIrecvInitialied != 1)
	{
		isIrecvInitialied =1 ;
		IRecvArray = initializeValues(numnodes,1);
	}
}
void setTimeElapsedBetweenCalls()
{
	if(globalStartTime == 0)
	{
		globalStartTime = MPI_Wtime();
	}
	else if(globalEndTime == 0)
	{
		globalEndTime = MPI_Wtime();
		globalTimeElapsed = (globalEndTime-globalStartTime) ;
		globalEndTime  = 0;
		globalStartTime = MPI_Wtime();
	}
}
void getNumberOfNodesToCreate(int numNodes)
{
	FILE * fp;
	int nodes = 0;
	int i,sum=0;
	char fileName[30];
	char *my_string =(char *)malloc (100);
	int numberOfVertex ;
	struct vertexNode *vals;
	arrayOfNodes = (struct vertexNode**)malloc(numNodes * sizeof(struct vertexNode*));
	//indexVertex = (struct vertexNode**)malloc( (numNodes)*sizeof(double*));
	for(i=0;i<numNodes;i++)
	{

			sprintf(fileName,"countRank%d.txt",i);
			fp = fopen(fileName,"r");
			fgets(my_string,10,fp);
			fclose(fp);
			numberOfVertex = atoi(my_string);
			nodesPerRank[i] = numberOfVertex;
			sum = sum + numberOfVertex;
			arrayOfNodes[i] = (struct vertexNode*)malloc( sizeof(struct vertexNode) * numberOfVertex);


	}
	numberOfVertex = 0;
	vals =(struct vertexNode*)malloc(sum * sizeof(struct vertexNode));
	sum = 0;
	for(i=0;i<numNodes;i++)
	{
		numberOfVertex = nodesPerRank[i];
		sum = sum + numberOfVertex;
		arrayOfNodes[i]= &vals[sum - numberOfVertex];
	}
}

void printAdjacencyMatrix(int numNodes)
{
	printf("\n Total Number Of Entries :: %d\n",numNodes);
	int i=0,j=0;
	for(i=0;i<numNodes;i++)
	{
		for(j=0;j<numNodes;j++)
		{
			printf("%d ",adjacencyMatrix[i][j]);
		}
		printf("\n");
	}
}
void printContentsOfArrayOfNodes(int numNodes)
{
	FILE *fp;
	fp = fopen("result.txt","a");
	printf("\n");
	int i =0,j=0;
	for(i=0;i<numNodes;i++)
	{
		fprintf(fp,"\r%s %s %s %s %s %s %s %s %s\n","operationName","positionInAdjacencymatrix","tag","source",
										"destination","count","msgSize","latency","globalTimeElapsed");
		for(j=0;j<nodesPerRank[i];j++)
		{
			//printf("\n%s %d %d %d %d %ld %d %d",arrayOfNodes[i][j].operationName,arrayOfNodes[i][j].tag,arrayOfNodes[i][j].source,
				//				arrayOfNodes[i][j].destination,arrayOfNodes[i][j].count,arrayOfNodes[i][j].msgSize,arrayOfNodes[i][j].latency,arrayOfNodes[i][j].globalTimeElapsed);
			//fprintf(fp,"\r\n%s %d %d %d ",arrayOfNodes[i][j].operationName,arrayOfNodes[i][j].source,arrayOfNodes[i][j].destination,arrayOfNodes[i][j].tag);

			fprintf(fp,"\n\r%s %d %d %d %d %d %ld %d %d\n",arrayOfNodes[i][j].operationName,arrayOfNodes[i][j].positionInAdjacencymatrix,arrayOfNodes[i][j].tag,arrayOfNodes[i][j].source,
					arrayOfNodes[i][j].destination,arrayOfNodes[i][j].count,arrayOfNodes[i][j].msgSize,arrayOfNodes[i][j].latency,arrayOfNodes[i][j].globalTimeElapsed);
		}
		printf("\n");
	}
	fclose(fp);
}
void createAdjacencyMatrix(int numnodes)
{

	adjacencyMatrix = initializeValues(positionInAdjacencyMatrix,-1);
	fillAdjacencyMatrix(numnodes);
	//printAdjacencyMatrix(positionInAdjacencyMatrix);
}
int findPositionBasedOnRank(char *operationName,int tag,int count,int source,int destination,int rank)
{
	int numOfVertex = nodesPerRank[rank];
	int j=0;
	for(j=0;j<numOfVertex;j++)
	{
		if((strcmp(arrayOfNodes[rank][j].operationName,operationName) == 0) && arrayOfNodes[rank][j].tag == tag && arrayOfNodes[rank][j].count == count)
		{
			return arrayOfNodes[rank][j].positionInAdjacencymatrix;
		}
	}
}
int findPositionOfCorrespondingAction(char *operationName,int tag,int count,int source,int destination,int rank)
{
	int numOfVertex = nodesPerRank[rank];
	int j=0;
	for(j=0;j<numOfVertex;j++)
	{
		if((strcmp(arrayOfNodes[rank][j].operationName,operationName) == 0) && arrayOfNodes[rank][j].tag == tag
				&& arrayOfNodes[rank][j].count == count && arrayOfNodes[rank][j].source == source && arrayOfNodes[rank][j].destination == destination)
		{
			return arrayOfNodes[rank][j].positionInAdjacencymatrix;
		}
	}
}
void fillAdjacencyMatrix(int numnodes)
{
	// Get From Operation and To Operation and corresponding positions and fill in the weight in the adjacencymatrix
	/*FILE *dotFp;
	char *dotfileName = _DOT_GRAPH_TXT_;
	if((dotFp=fopen(dotfileName,"a")) == NULL )
	{
		printf("Could Not open file to write to DotGraph.txt");
	}*/
	char correspondingAction[30];
	int i=0,j=0;
	int srcPosition=0,myPosition=0,correspondingActionPosition=0;
	for(i=0;i<numnodes;i++)
	{
		int numOfVertex = nodesPerRank[i];
		for(j=1; j<numOfVertex;j++)
		{
			srcPosition = arrayOfNodes[i][j-1].positionInAdjacencymatrix;
			myPosition = arrayOfNodes[i][j].positionInAdjacencymatrix;
			if((strcmp(arrayOfNodes[i][j].operationName,"MPI_Send")==0) ||
					(strcmp(arrayOfNodes[i][j].operationName,"MPI_Isend") == 0) /*||
							(strcmp(arrayOfNodes[i][j].operationName,"MPI_Scatter") == 0 )*/)// Check for Correspondinng send , isend , scatter operation for its pair recv , irecv , gather .
			{
				if((strcmp(arrayOfNodes[i][j].operationName,"MPI_Send") == 0))
				{
					strcpy(correspondingAction,"MPI_Recv");
				}
				else if((strcmp(arrayOfNodes[i][j].operationName,"MPI_Isend") == 0))
				{
					strcpy(correspondingAction,"MPI_Irecv");
				}
				/*else if((strcmp(arrayOfNodes[i][j].operationName,"MPI_Scatter") == 0))
				{
					strcpy(correspondingAction,"MPI_Gather");
				}*/
				correspondingActionPosition = findPositionOfCorrespondingAction(correspondingAction,arrayOfNodes[i][j].tag,
						arrayOfNodes[i][j].count,arrayOfNodes[i][j].source,arrayOfNodes[i][j].destination,arrayOfNodes[i][j].destination);
				//printf("\ncorrespondingActionPosition for %s from %d is %d",arrayOfNodes[i][j].operationName,myPosition,correspondingActionPosition);
				adjacencyMatrix[myPosition][correspondingActionPosition] = arrayOfNodes[i][j].latency;// My current Position to my corresponding partner location
				// Code to add an Entry into Dot Graph for Links
				//fprintf(dotFp,"\r\n%d->%d[label=\"%d(%ld)\"]",myPosition,correspondingActionPosition,arrayOfNodes[i][j].latency,arrayOfNodes[i][j].msgSize);
				// End of code
			}
			myPosition = arrayOfNodes[i][j].positionInAdjacencymatrix;
			/*
			 * Change Below Part , always take wall clockTime for sequential parts on same rank send_>recv->barrier , latency is only for cross operations
			 */
			if(isCollectiveOperation(arrayOfNodes[i][j].operationName) == 1 && adjacencyMatrix[srcPosition][myPosition] < arrayOfNodes[i][j].globalTimeElapsed)
			{
				adjacencyMatrix[srcPosition][myPosition] = arrayOfNodes[i][j].globalTimeElapsed;
			}
			else
			{
				adjacencyMatrix[srcPosition][myPosition] = arrayOfNodes[i][j].globalTimeElapsed;// From previous Node to Current Node what is the time taken ie : send->recv [On same rank]
			}
			//fprintf(dotFp,"\r\n%d->%d[label=\"%d\"]",srcPosition,myPosition,arrayOfNodes[i][j].globalTimeElapsed);
		}
	}
	//fclose(dotFp);
}
void createNodePerRank(int numNodes,char * fileName,int myRank)
{
	//printf("\n Creating Node For My Rank %d",myRank);
	int i,value =1;
	FILE * fp;
	FILE * result;
	//struct vertexNode *newAdjacencyNode;
	char *token;
	char * delimiter = " ";
	char *lineRead = (char*)malloc(100);
	//	arrayOfNodes[myRank] = newAdjacencyNode;

	int j=0,structureCreated =0;

	if((fp = fopen(fileName,"r")) != NULL)
	{
		struct vertexNode *tailNode = NULL;
		struct vertexNode *headNode = NULL;
		fgets(lineRead,6,fp);// Skip the 1st Line it has no useful info for the matrix
		while(!feof(fp))
		{
			fgets(lineRead,50,fp);
			struct vertexNode newAdjacencyNode;

			token = strtok(lineRead," ");
			value =1;
			while(token != NULL)
			{
				if(token != NULL)
				{
					switch(value)
					{
					case 1:
						//printf("\n Op:: %s",token);
						strcpy(arrayOfNodes[myRank][j].operationName,token);
						value++;
						break;
					case 2 :
						//printf("%s\n",token);
						arrayOfNodes[myRank][j].tag = atoi(token);
						//printf("%d\n", newAdjacencyNode->source);
						value++;
						break;
					case 3 :
						//printf("%s\n",token);
						arrayOfNodes[myRank][j].source = atoi(token);
						//printf("%d\n", newAdjacencyNode->source);
						value++;
						break;
					case 4 :
						//printf("%s\n",token);
						arrayOfNodes[myRank][j].destination = atoi(token);
						//printf("%d\n", newAdjacencyNode->destination);
						value++;
						break;
					case 5 :
						//printf("%s\n",token);
						arrayOfNodes[myRank][j].count = atoi(token);
						//printf("%d\n", newAdjacencyNode->count);
						value++;
						break;
					case 6 :
						//printf("%s\n",token);
						arrayOfNodes[myRank][j].msgSize = atol(token);
						//printf("%ld\n", newAdjacencyNode->msgSize);
						value++;
						break;
					case 7 :
						//printf("%s\n",token);
						arrayOfNodes[myRank][j].latency = atoi(token);
						//printf("%d\n", newAdjacencyNode->latency);
						value++;
						break;
					case 8 :
						//printf("%s\n",token);
						arrayOfNodes[myRank][j].globalTimeElapsed = atoi(token);
						//printf("%d\n", newAdjacencyNode->globalTimeElapsed);
						value++;
						break;
					case 9 :
						arrayOfNodes[myRank][j].totalTime = atoi(token);
						break;
					default: break;
					}
					token = strtok(NULL,delimiter);
				}

			}
			countNumberOfInvocation(arrayOfNodes[myRank][j].operationName);
			if(myRank != 0  && ( ((strcmp(arrayOfNodes[myRank][j].operationName,"MPI_Barrier") == 0)) ||
					(strcmp(arrayOfNodes[myRank][j].operationName,"MPI_Reduce") == 0) ||
					(strcmp(arrayOfNodes[myRank][j].operationName,"MPI_Scatter") == 0) ||
					(strcmp(arrayOfNodes[myRank][j].operationName,"MPI_Gather") == 0) ||
					(strcmp(arrayOfNodes[myRank][j].operationName,"MPI_Allreduce") == 0) ||
					(strcmp(arrayOfNodes[myRank][j].operationName,"MPI_Alltoall") == 0)) )
			{
				int pos = findPositionBasedOnRank(arrayOfNodes[myRank][j].operationName,arrayOfNodes[myRank][j].tag,arrayOfNodes[myRank][j].count,arrayOfNodes[myRank][j].source,arrayOfNodes[myRank][j].destination,0);// Find from Rank 0
				arrayOfNodes[myRank][j].positionInAdjacencymatrix = pos;
			}
			else if(myRank != 0 && ((strcmp(arrayOfNodes[myRank][j].operationName,"Init") == 0) ||
					(strcmp(arrayOfNodes[myRank][j].operationName,"MPI_Init") == 0)))// Init will be the 1st node of Rank 0
			{
				arrayOfNodes[myRank][j].positionInAdjacencymatrix = 0;
			}
			else if(myRank !=0 &&(strcmp(arrayOfNodes[myRank][j].operationName,"MPI_Finalize") == 0)) // Finalize will be the last node of Rank 0
			{
				arrayOfNodes[myRank][j].positionInAdjacencymatrix = nodesPerRank[0] -1;
			}
			else
			{
				//printf("Position for %s is %d",arrayOfNodes[myRank][j].operationName,positionInAdjacencyMatrix);
				arrayOfNodes[myRank][j].positionInAdjacencymatrix = positionInAdjacencyMatrix;
				positionInAdjacencyMatrix++;
			}
			// Skip Collective , Init , Finalize these are common to all ranks
			if(isInitOrFinalize(arrayOfNodes[myRank][j].operationName) == 0 && isCollectiveOperation(arrayOfNodes[myRank][j].operationName) == 0 ) // Dont create for collecctive operations here , create it later at end of Rank 0 file
			{
				//printf("\n Structure Created :: %d ",structureCreated);
				structureCreated = createDotGraphNodes(myRank,j,numNodes,structureCreated);
			}
			j++;
		}
		fclose(fp);
		FILE *dotFp;
		char dotfileName[20] = _DOT_GRAPH_TXT_;
		if((dotFp = fopen(dotfileName,"a")) != NULL)
		{
			fprintf(dotFp,"\r\n}");

		}
		fclose(dotFp);
	}


}
int  createDotGraphNodes(int myRank,int j,int numNodes,int structureCreated)
{
	FILE *dotFp;
	char *dotfileName = _DOT_GRAPH_TXT_;
	if((dotFp = fopen(dotfileName,"a")) != NULL)
	{
		if(structureCreated == 0)
		{
			fprintf(dotFp,"\r\nsubgraph cluster_%d{",myRank);
			fprintf(dotFp,"\r label =\"Rank%d\"",myRank);
			fprintf(dotFp,"\r\n%d [label=%s];",arrayOfNodes[myRank][j].positionInAdjacencymatrix,arrayOfNodes[myRank][j].operationName);
			structureCreated = 1;
		}
		else
		{
			//fprintf(dotFp,"\r\n%s_%d_%d_%d_%d [label=%s];",arrayOfNodes[myRank][j].operationName,arrayOfNodes[myRank][j].tag,arrayOfNodes[myRank][j].source,arrayOfNodes[myRank][j].destination,arrayOfNodes[myRank][j].count,arrayOfNodes[myRank][j].operationName);
			fprintf(dotFp,"\r\n%d [label=%s];",arrayOfNodes[myRank][j].positionInAdjacencymatrix,arrayOfNodes[myRank][j].operationName);
		}
	}
	fclose(dotFp);
	return structureCreated;

}
void  createDotGraphNodesForCollectives()
{
	FILE *dotFp;
	char *dotfileName = _DOT_GRAPH_TXT_;
	int i=0,j=0;
	dotFp = fopen(dotfileName,"a");

		for(j=0;j<nodesPerRank[0];j++)
		{
			if((isCollectiveOperation(arrayOfNodes[0][j].operationName) == 1)
					|| (isInitOrFinalize(arrayOfNodes[0][j].operationName) == 1))
			{
				fprintf(dotFp,"\r\n%d [label=%s];",arrayOfNodes[0][j].positionInAdjacencymatrix,arrayOfNodes[0][j].operationName);
			}
		}

	fclose(dotFp);
}

int isCollectiveOperation(char * opName)
{
	//printf("\n Op Name is :: %s ",opName);
	if((strcmp(opName,"MPI_Barrier") == 0) || (strcmp(opName,"MPI_Reduce") == 0)
			|| (strcmp(opName,"MPI_Scatter") == 0) || (strcmp(opName,"MPI_Gather") == 0)
			|| (strcmp(opName,"MPI_Allreduce") == 0) || (strcmp(opName,"MPI_Alltoall") == 0))
	{
		//printf("Return 1");
		return 1;
	}
	else
		return 0;
}
int isInitOrFinalize(char * opName)
{
	if((strcmp(opName,"Init") == 0) || (strcmp(opName,"MPI_Init") == 0) || (strcmp(opName,"MPI_Finalize") == 0))
	{
		return 1;
	}
	else
		return 0;
}
int* findCriticalPath(int workingCol, int **pathFound, int *maxLenghtFound)
{
	int pathLength = 0;

	if(workingCol == 0)
	{
		maxLenghtFound[workingCol] = 0;
		pathFound[workingCol][0] = 0;
		return pathFound[workingCol];
	}
	else
	{
		if(pathFound[workingCol][0] != -1)// If path already Exists Just return it , no need to recompute saving processing time
		{
			return pathFound[workingCol];
		}
		else
		{
			int i=0,j=0;
			for(i = 0; i < positionInAdjacencyMatrix; i++)
			{
				if(adjacencyMatrix[i][workingCol] != -1)
				{
					pathFound[i] = findCriticalPath(i, pathFound, maxLenghtFound);
					if(maxLenghtFound[workingCol] <= maxLenghtFound[i] + adjacencyMatrix[i][workingCol])
					{

						maxLenghtFound[workingCol] = maxLenghtFound[i] + adjacencyMatrix[i][workingCol];

						pathLength = getCountOfElementsInFath(pathFound[i]);
						for(j = 0; j < positionInAdjacencyMatrix; j++)
						{
							pathFound[workingCol][j] = -1;
						}
						for(j = 0; j < pathLength; j++){
							pathFound[workingCol][j] = pathFound[i][j];
						}
						pathFound[workingCol][pathLength] = workingCol;

					}
				}
			}
			return pathFound[workingCol];
		}
	}
}
int getCountOfElementsInFath(int *path)
{
	int i=0;
	while(path[i] != -1)
	{
		i++;
	}
	return i;
}
void countNumberOfInvocation(char *opName)
{
	if((strcmp("MPI_Init",opName) == 0) || (strcmp("Init",opName) == 0))
	{
		countOfOperation[_MPI_Init] = countOfOperation[_MPI_Init]+1;
	}
	else if(strcmp("MPI_Barrier",opName) == 0)
	{
		countOfOperation[_MPI_Barrier] = countOfOperation[_MPI_Barrier]+1;
	}
	else if(strcmp("MPI_Wait",opName) == 0)
	{
		countOfOperation[_MPI_Wait] = countOfOperation[_MPI_Wait]+1;
	}
	else if(strcmp("MPI_Scatter",opName) == 0)
	{
		countOfOperation[_MPI_Scatter] = countOfOperation[_MPI_Scatter]+1;
	}
	else if(strcmp("MPI_Gather",opName) == 0)
	{
		countOfOperation[_MPI_Gather] = countOfOperation[_MPI_Gather]+1;
	}
	else if(strcmp("MPI_Isend",opName) == 0)
	{
		countOfOperation[_MPI_Isend] = countOfOperation[_MPI_Isend]+1;
	}
	else if(strcmp("MPI_Send",opName) == 0)
	{
		countOfOperation[_MPI_Send] = countOfOperation[_MPI_Send]+1;
	}
	else if(strcmp("MPI_Recv",opName) == 0)
	{
		countOfOperation[_MPI_Recv] = countOfOperation[_MPI_Recv]+1;
	}
	else if(strcmp("MPI_Irecv",opName) == 0)
	{
		countOfOperation[_MPI_Irecv] = countOfOperation[_MPI_Irecv]+1;
	}
	else if(strcmp("MPI_Reduce",opName) == 0)
	{
		countOfOperation[_MPI_Reduce] = countOfOperation[_MPI_Reduce]+1;
	}
	else if(strcmp("MPI_Alltoall",opName) == 0)
	{
		countOfOperation[_MPI_Alltoall] = countOfOperation[_MPI_Alltoall]+1;
	}
	else if(strcmp("MPI_Allreduce",opName) == 0)
	{
		countOfOperation[_MPI_Allreduce] = countOfOperation[_MPI_Allreduce]+1;
	}
	else if(strcmp("MPI_Waitall",opName) == 0)
	{
		countOfOperation[_MPI_Waitall] = countOfOperation[_MPI_Waitall]+1;
	}
	else
	{
		// Finalize Do Nothing
	}
}
void printNumberOfOperation()
{
	int i=0;
	for(i=0;i<_NUMBER_OF_MPI_OPERTAION;i++)
	{
		printf("\n %d :: %d ",i,countOfOperation[i]);
	}
}
void createTheCountArrayForEachOperation()
{
	int i=0;
	for(i=0;i<_NUMBER_OF_MPI_OPERTAION;i++)
	{
		int sizeOfArray = countOfOperation[i];// Get Count For Each Array and Create memory for it
		if(sizeOfArray > 0)
		{
			if(i == _MPI_Init)
			{
				initWallClock = (int*)malloc(sizeOfArray*sizeof(int));
			}
			else if(i ==_MPI_Barrier)
			{
				barrierWallClock = (int*)malloc(sizeOfArray*sizeof(int));
			}
			else if(i ==_MPI_Send)
			{
				sendWallClock = (int*)malloc(sizeOfArray*sizeof(int));
			}
			else if(i ==_MPI_Recv)
			{
				recvWallClock = (int*)malloc(sizeOfArray*sizeof(int));
			}
			else if(i ==_MPI_Isend)
			{
				iSendWallClock = (int*)malloc(sizeOfArray*sizeof(int));
			}
			else if(i ==_MPI_Irecv)
			{
				iRecvWallClock = (int*)malloc(sizeOfArray*sizeof(int));
			}
			else if(i ==_MPI_Scatter)
			{
				scatterWallClock = (int*)malloc(sizeOfArray*sizeof(int));
			}
			else if(i ==_MPI_Gather)
			{
				gatherWallClock = (int*)malloc(sizeOfArray*sizeof(int));
			}
			else if(i ==_MPI_Reduce)
			{
				reduceWallClock = (int*)malloc(sizeOfArray*sizeof(int));
			}
			else if(i ==_MPI_Allreduce)
			{
				allReduceWallClock = (int*)malloc(sizeOfArray*sizeof(int));
			}
			else if(i ==_MPI_Wait)
			{
				waitWallClock = (int*)malloc(sizeOfArray*sizeof(int));
			}
			else if(i ==_MPI_Waitall)
			{
				waitAllWallClock = (int*)malloc(sizeOfArray*sizeof(int));
			}
			else if(i ==_MPI_Alltoall)
			{
				allToAllWallClock = (int*)malloc(sizeOfArray*sizeof(int));
			}
			else{
				// Finalize Block
			}

		}
	}
}
void fillUpCountArrayWithTime(int numNodes)
{
	int i =0,j=0;
	for(i=0;i<numNodes;i++)
	{
		for(j=0;j<nodesPerRank[i];j++)
		{
			storeDataForCriticalPath(i,j);// THIS IS FOR CRITICAL PATH REPORT GENERATION
			if((strcmp("MPI_Init",arrayOfNodes[i][j].operationName) == 0) || (strcmp("Init",arrayOfNodes[i][j].operationName) == 0))
			{
				totalInitWallClock = totalInitWallClock +  arrayOfNodes[i][j].totalTime;
				initWallClock[initWallClockPos++] = arrayOfNodes[i][j].totalTime;
			}
			else if(strcmp("MPI_Barrier",arrayOfNodes[i][j].operationName) == 0)
			{
				totalBarrierWallClock = totalBarrierWallClock + arrayOfNodes[i][j].totalTime;
				barrierWallClock[barrierWallClockPos++] = arrayOfNodes[i][j].totalTime;
			}
			else if(strcmp("MPI_Wait",arrayOfNodes[i][j].operationName) == 0)
			{
				totalWaitAllWallClock = totalWaitAllWallClock + arrayOfNodes[i][j].totalTime;
				waitWallClock[waitWallClockPos++] = arrayOfNodes[i][j].totalTime;
			}
			else if(strcmp("MPI_Scatter",arrayOfNodes[i][j].operationName) == 0)
			{
				totalScatterWallClock = totalScatterWallClock + arrayOfNodes[i][j].totalTime;
				scatterWallClock[scatterWallClockPos++] = arrayOfNodes[i][j].totalTime;
			}
			else if(strcmp("MPI_Gather",arrayOfNodes[i][j].operationName) == 0)
			{
				totalGatherWallClock = totalGatherWallClock + arrayOfNodes[i][j].totalTime;
				gatherWallClock[gatherWallClockPos++] = arrayOfNodes[i][j].totalTime;
			}
			else if(strcmp("MPI_Isend",arrayOfNodes[i][j].operationName) == 0)
			{
				totalISendWallClock = totalISendWallClock + arrayOfNodes[i][j].totalTime;
				iSendWallClock[iSendWallClockPos++] = arrayOfNodes[i][j].totalTime;
			}
			else if(strcmp("MPI_Send",arrayOfNodes[i][j].operationName) == 0)
			{
				totalSendWallClock = totalSendWallClock + arrayOfNodes[i][j].totalTime;
				sendWallClock[sendWallClockPos++] = arrayOfNodes[i][j].totalTime;
			}
			else if(strcmp("MPI_Recv",arrayOfNodes[i][j].operationName) == 0)
			{
				totalRecvWallClock = totalRecvWallClock + arrayOfNodes[i][j].totalTime;
				recvWallClock[recvWallClockpos++]= arrayOfNodes[i][j].totalTime;
			}
			else if(strcmp("MPI_Irecv",arrayOfNodes[i][j].operationName) == 0)
			{
				totalIRecvWallClock = totalIRecvWallClock + arrayOfNodes[i][j].totalTime;
				iRecvWallClock[iRecvWallClockPos++] = arrayOfNodes[i][j].totalTime;
			}
			else if(strcmp("MPI_Reduce",arrayOfNodes[i][j].operationName) == 0)
			{
				totalReduceWallClock = totalReduceWallClock + arrayOfNodes[i][j].totalTime;
				reduceWallClock[reduceWallClockPos++] = arrayOfNodes[i][j].totalTime;
			}
			else if(strcmp("MPI_Alltoall",arrayOfNodes[i][j].operationName) == 0)
			{
				totalAllToAllWallClock = totalAllToAllWallClock + arrayOfNodes[i][j].totalTime;
				allToAllWallClock[allToAllWallClockPos++] = arrayOfNodes[i][j].totalTime;
			}
			else if(strcmp("MPI_Allreduce",arrayOfNodes[i][j].operationName) == 0)
			{
				totalAllReduceWallClock = totalAllReduceWallClock + arrayOfNodes[i][j].totalTime;
				allReduceWallClock[allReduceWallClockPos++] = arrayOfNodes[i][j].totalTime;
			}
			else if(strcmp("MPI_Waitall",arrayOfNodes[i][j].operationName) == 0)
			{
				totalWaitAllWallClock = totalWaitAllWallClock + arrayOfNodes[i][j].totalTime;
				waitAllWallClock[waitAllWallClockPos++] = arrayOfNodes[i][j].totalTime;
			}
			else
			{
				// Finalize Do Nothing
			}
		}
	}
}

void printContentsOfTimeArray()
{
	int i=0,j=0;
	printf("\n");
	printf("Contents Of Time Array \n");
	for(i=0;i<_NUMBER_OF_MPI_OPERTAION;i++)
	{
		printf("\n Op :: %d",i);
		if(i == _MPI_Init && initWallClock != NULL)
		{
			for(j=0;j<countOfOperation[i];j++)
			{
				printf("%d,",initWallClock[j]);
			}
			printf("\n");
		}
		else if(i ==_MPI_Barrier && barrierWallClock != NULL)
		{
			for(j=0;j<countOfOperation[i];j++)
			{
				printf("%d,",barrierWallClock[j]);
			}
			printf("\n");
		}
		else if(i ==_MPI_Send && sendWallClock != NULL)
		{
			for(j=0;j<countOfOperation[i];j++)
			{
				printf("%d,",sendWallClock[j]);
			}
			printf("\n");
		}
		else if(i ==_MPI_Recv && recvWallClock != NULL)
		{
			for(j=0;j<countOfOperation[i];j++)
			{
				printf("%d,",recvWallClock[j]);
			}
			printf("\n");
		}
		else if(i ==_MPI_Isend && iSendWallClock != NULL)
		{
			for(j=0;j<countOfOperation[i];j++)
			{
				printf("%d,",iSendWallClock[j]);
			}
			printf("\n");
		}
		else if(i ==_MPI_Irecv && iRecvWallClock != NULL)
		{
			for(j=0;j<countOfOperation[i];j++)
			{
				printf("%d,",iRecvWallClock[j]);
			}
			printf("\n");
		}
		else if(i ==_MPI_Scatter && scatterWallClock != NULL)
		{
			for(j=0;j<countOfOperation[i];j++)
			{
				printf("%d,",scatterWallClock[j]);
			}
			printf("\n");
		}
		else if(i ==_MPI_Gather && gatherWallClock != NULL)
		{
			for(j=0;j<countOfOperation[i];j++)
			{
				printf("%d,",gatherWallClock[j]);
			}
			printf("\n");
		}
		else if(i ==_MPI_Reduce && reduceWallClock != NULL)
		{
			for(j=0;j<countOfOperation[i];j++)
			{
				printf("%d,",reduceWallClock[j]);
			}
			printf("\n");
		}
		else if(i ==_MPI_Allreduce && allReduceWallClock !=NULL)
		{
			for(j=0;j<countOfOperation[i];j++)
			{
				printf("%d,",allReduceWallClock[j]);
			}
			printf("\n");
		}
		else if(i ==_MPI_Wait && waitWallClock != NULL)
		{
			for(j=0;j<countOfOperation[i];j++)
			{
				printf("%d,",waitWallClock[j]);
			}
			printf("\n");
		}
		else if(i ==_MPI_Waitall && waitAllWallClock != NULL)
		{
			for(j=0;j<countOfOperation[i];j++)
			{
				printf("%d,",waitAllWallClock[j]);
			}
			printf("\n");
		}
		else if(i ==_MPI_Alltoall && allToAllWallClock != NULL)
		{
			for(j=0;j<countOfOperation[i];j++)
			{
				printf("%d,",allToAllWallClock[j]);
			}
			printf("\n");
		}
		else{
			// Finalize Block
		}

	}
}
void sortTimeArray()
{

	int i=0,j=0;
	for(i=0;i<_NUMBER_OF_MPI_OPERTAION;i++)
	{
		if(i == _MPI_Init && initWallClock != NULL)
		{
			quicksort(initWallClock,0,countOfOperation[i]-1);
		}
		else if(i ==_MPI_Barrier && barrierWallClock != NULL)
		{
			quicksort(barrierWallClock,0,countOfOperation[i]-1);
		}
		else if(i ==_MPI_Send && sendWallClock != NULL)
		{
			quicksort(sendWallClock,0,countOfOperation[i]-1);
		}
		else if(i ==_MPI_Recv && recvWallClock != NULL)
		{
			quicksort(recvWallClock,0,countOfOperation[i]-1);
		}
		else if(i ==_MPI_Isend && iSendWallClock != NULL)
		{
			quicksort(iSendWallClock,0,countOfOperation[i]-1);
		}
		else if(i ==_MPI_Irecv && iRecvWallClock != NULL)
		{
			quicksort(iRecvWallClock,0,countOfOperation[i]-1);
		}
		else if(i ==_MPI_Scatter && scatterWallClock != NULL)
		{
			quicksort(scatterWallClock,0,countOfOperation[i]-1);
		}
		else if(i ==_MPI_Gather && gatherWallClock != NULL)
		{
			quicksort(gatherWallClock,0,countOfOperation[i]-1);
		}
		else if(i ==_MPI_Reduce && reduceWallClock != NULL)
		{
			quicksort(reduceWallClock,0,countOfOperation[i]-1);
		}
		else if(i ==_MPI_Allreduce && allReduceWallClock !=NULL)
		{
			quicksort(allReduceWallClock,0,countOfOperation[i]-1);
		}
		else if(i ==_MPI_Wait && waitWallClock != NULL)
		{
			quicksort(waitWallClock,0,countOfOperation[i]-1);
		}
		else if(i ==_MPI_Waitall && waitAllWallClock != NULL)
		{
			quicksort(waitAllWallClock,0,countOfOperation[i]-1);
		}
		else if(i ==_MPI_Alltoall && allToAllWallClock != NULL)
		{
			quicksort(allToAllWallClock,0,countOfOperation[i]-1);
		}
		else{
			// Finalize Block
		}

	}
}
void quicksort(int *unsortedArray,int first,int last)
{
	int pivot,j,temp,i;

	if(first<last){
		pivot=first;
		i=first;
		j=last;

		while(i<j){
			while(unsortedArray[i]<=unsortedArray[pivot]&&i<last)
				i++;
			while(unsortedArray[j]>unsortedArray[pivot])
				j--;
			if(i<j){
				temp=unsortedArray[i];
				unsortedArray[i]=unsortedArray[j];
				unsortedArray[j]=temp;
			}
		}

		temp=unsortedArray[pivot];
		unsortedArray[pivot]=unsortedArray[j];
		unsortedArray[j]=temp;
		quicksort(unsortedArray,first,j-1);
		quicksort(unsortedArray,j+1,last);

	}
}
void generateReport()
{
	// Remove the Old File 1st
	remove(_STATS_DAT);
	//Done Deleting File
	FILE *fp;
	char *functionName = (char*)malloc(30);
	char *file_name = (char*)malloc(30);
	strcpy(file_name,_STATS_DAT);
	int i=0;
	fp = fopen(file_name,"a");
	//Create the Column Names
	fprintf(fp,"\r\nFunction\tInvocations\tMean\tMin\tMedian\tMax");
	for(i=0;i<_NUMBER_OF_MPI_OPERTAION-1;i++)
	{
		int invocation=0,mean=0,median=0,max=0,min=0;

		switch(i)
		{
		case _MPI_Init:
			strcpy(functionName,"MPI_Init");
			if(initWallClock != NULL)
			{
				min = initWallClock[0];
				max = initWallClock[initWallClockPos-1];
				mean = totalInitWallClock/initWallClockPos;
				int val = initWallClockPos/2;
				median = initWallClock[val];
				invocation = countOfOperation[_MPI_Init];
			}
			break;
		case _MPI_Barrier:
			strcpy(functionName,"MPI_Barrier");
			if(barrierWallClock != NULL)
			{
				min = barrierWallClock[0];
				max = barrierWallClock[barrierWallClockPos-1];
				mean = totalBarrierWallClock/barrierWallClockPos;
				int val = barrierWallClockPos/2;
				median = barrierWallClock[val];
				invocation = countOfOperation[_MPI_Barrier];
			}
			break;
		case _MPI_Send:
			strcpy(functionName,"MPI_Send");
			if(sendWallClock != NULL)
			{
				min = sendWallClock[0];
				max = sendWallClock[sendWallClockPos-1];
				mean = totalSendWallClock/sendWallClockPos;
				int val = sendWallClockPos/2;
				median = sendWallClock[val];
				invocation = countOfOperation[_MPI_Send];
			}
			break;
		case _MPI_Recv:
			strcpy(functionName,"MPI_Recv");
			if(recvWallClock != NULL)
			{
				min = recvWallClock[0];
				max = recvWallClock[recvWallClockpos-1];
				mean = totalRecvWallClock/recvWallClockpos;
				int val = recvWallClockpos/2;
				median = recvWallClock[val];
				invocation = countOfOperation[_MPI_Recv];
			}
			break;
		case _MPI_Isend:
			strcpy(functionName,"MPI_Isend");
			if(iSendWallClock != NULL)
			{
				min = iSendWallClock[0];
				max = iSendWallClock[iSendWallClockPos-1];
				mean = totalISendWallClock/iSendWallClockPos;
				int val = iSendWallClockPos/2;
				median = iSendWallClock[val];
				invocation = countOfOperation[_MPI_Isend];
			}
			break;
		case _MPI_Irecv:
			strcpy(functionName,"MPI_Irecv");
			if(iRecvWallClock != NULL)
			{
				min = iRecvWallClock[0];
				max = iRecvWallClock[iRecvWallClockPos-1];
				mean = totalIRecvWallClock/iRecvWallClockPos;
				int val = iRecvWallClockPos/2;
				median = iRecvWallClock[val];
				invocation = countOfOperation[_MPI_Irecv];
			}
			break;
		case _MPI_Scatter:
			strcpy(functionName,"MPI_Scatter");
			if(scatterWallClock != NULL)
			{
				min = scatterWallClock[0];
				max = scatterWallClock[scatterWallClockPos-1];
				mean = totalScatterWallClock/scatterWallClockPos;
				int val = scatterWallClockPos/2;
				median = scatterWallClock[val];
				invocation = countOfOperation[_MPI_Scatter];
			}
			break;
		case _MPI_Gather:
			strcpy(functionName,"MPI_Gather");
			if(gatherWallClock != NULL)
			{
				min = gatherWallClock[0];
				max = gatherWallClock[gatherWallClockPos-1];
				mean = totalGatherWallClock/gatherWallClockPos;
				int val = gatherWallClockPos/2;
				median = gatherWallClock[val];
				invocation = countOfOperation[_MPI_Gather];
			}
			break;
		case _MPI_Reduce:
			strcpy(functionName,"MPI_Reduce");
			if(reduceWallClock != NULL)
			{
				min = reduceWallClock[0];
				max = reduceWallClock[reduceWallClockPos-1];
				mean = totalReduceWallClock/reduceWallClockPos;
				int val = reduceWallClockPos/2;
				median = reduceWallClock[val];
				invocation = countOfOperation[_MPI_Reduce];
			}
			break;
		case _MPI_Allreduce:
			strcpy(functionName,"MPI_Allreduce");
			if(allReduceWallClock != NULL)
			{
				min = allReduceWallClock[0];
				max = allReduceWallClock[allReduceWallClockPos-1];
				mean = totalAllReduceWallClock/allReduceWallClockPos;
				int val = allReduceWallClockPos/2;
				median = allReduceWallClock[val];
				invocation = countOfOperation[_MPI_Allreduce];
			}
			break;
		case _MPI_Wait:
			strcpy(functionName,"MPI_Wait");
			if(waitWallClock != NULL)
			{
				min = waitWallClock[0];
				max = waitWallClock[waitWallClockPos-1];
				mean = totalWaitWallClock/waitWallClockPos;
				int val = waitWallClockPos/2;
				median = waitWallClock[val];
				invocation = countOfOperation[_MPI_Wait];
			}
			break;
		case _MPI_Waitall:
			strcpy(functionName,"MPI_Waitall");
			if(waitAllWallClock != NULL)
			{
				min = waitAllWallClock[0];
				max = waitAllWallClock[waitAllWallClockPos-1];
				mean = totalWaitAllWallClock/waitAllWallClockPos;
				int val = waitAllWallClockPos/2;
				median = waitAllWallClock[val];
				invocation = countOfOperation[_MPI_Waitall];
			}
			break;
		case _MPI_Alltoall:
			strcpy(functionName,"MPI_Alltoall");
			if(allToAllWallClock != NULL)
			{
				min = allToAllWallClock[0];
				max = allToAllWallClock[allToAllWallClockPos-1];
				mean = totalAllToAllWallClock/allToAllWallClockPos;
				int val = allToAllWallClockPos/2;
				median = allToAllWallClock[val];
				invocation = countOfOperation[_MPI_Alltoall];
			}
			break;
		}
		fprintf(fp,"\r\n%s\t%d\t%d\t%d\t%d\t%d",functionName,invocation,mean,min,median,max);
	}

	fclose(fp);

}
void storeDataForCriticalPath(int myRank,int j)
{
	int isCollectiveOrFinalizeOp = ( (isInitOrFinalize(arrayOfNodes[myRank][j].operationName)) || (isCollectiveOperation(arrayOfNodes[myRank][j].operationName) )) ;
	if(myRank != 0 && (isCollectiveOrFinalizeOp == 1))// If It is collecive and not Rank 0 do not add it into the structure
	{

	}
	else
	{
		struct criticalPathData tempData ;
		strcpy(tempData.operationName,arrayOfNodes[myRank][j].operationName);
		tempData.msgSize = arrayOfNodes[myRank][j].msgSize;
		if(isCollectiveOrFinalizeOp == 1)
		{
			tempData.myRank = -1;
		}
		else if(((strcmp(arrayOfNodes[myRank][j].operationName,"MPI_Recv") == 0) || (strcmp(arrayOfNodes[myRank][j].operationName,"MPI_Irecv") == 0)))
		{
			//skipValues[j][myRank] = 1;
			tempData.myRank = arrayOfNodes[myRank][j].destination; // For Recv Destination is My Rank
		}
		else
		{
			tempData.myRank = arrayOfNodes[myRank][j].source;
		}

		if(j < nodesPerRank[myRank])
		tempData.totalTime = arrayOfNodes[myRank][j+1].globalTimeElapsed;

		tempData.msgSize = arrayOfNodes[myRank][j].msgSize;
		pathToDestination[arrayOfNodes[myRank][j].positionInAdjacencymatrix] = tempData;

	}
}
void generateDotGraph(int numnodes)
{

	// Get From Operation and To Operation and corresponding positions and fill in the weight in the adjacencymatrix
	FILE *dotFp;
	char *dotfileName = _DOT_GRAPH_TXT_;
	if((dotFp=fopen(dotfileName,"a")) == NULL )
	{
		printf("Could Not open file to write to DotGraph.txt");
	}
	char correspondingAction[30];
	int i=0,j=0;
	int srcPosition=0,myPosition=0,correspondingActionPosition=0;
	for(i=0;i<numnodes;i++)
	{
		int numOfVertex = nodesPerRank[i];
		for(j=1; j<numOfVertex;j++)
		{
			srcPosition = arrayOfNodes[i][j-1].positionInAdjacencymatrix;
			myPosition = arrayOfNodes[i][j].positionInAdjacencymatrix;
			if((strcmp(arrayOfNodes[i][j].operationName,"MPI_Send")==0) ||
					(strcmp(arrayOfNodes[i][j].operationName,"MPI_Isend") == 0))
			{
				if((strcmp(arrayOfNodes[i][j].operationName,"MPI_Send") == 0))
				{
					strcpy(correspondingAction,"MPI_Recv");
				}
				else if((strcmp(arrayOfNodes[i][j].operationName,"MPI_Isend") == 0))
				{
					strcpy(correspondingAction,"MPI_Irecv");
				}
				correspondingActionPosition = findPositionOfCorrespondingAction(correspondingAction,arrayOfNodes[i][j].tag,
						arrayOfNodes[i][j].count,arrayOfNodes[i][j].source,arrayOfNodes[i][j].destination,arrayOfNodes[i][j].destination);
				if(criticalPathLinkExists[myPosition][correspondingActionPosition] ==  1)
				{
					fprintf(dotFp,"\r\n%d->%d[label=\"%d(%ld)\"][color=\"RED\"]",myPosition,correspondingActionPosition,arrayOfNodes[i][j].latency,arrayOfNodes[i][j].msgSize);
				}
				else
				{
					fprintf(dotFp,"\r\n%d->%d[label=\"%d(%ld)\"]",myPosition,correspondingActionPosition,arrayOfNodes[i][j].latency,arrayOfNodes[i][j].msgSize);
				}
				// End of code
			}
			myPosition = arrayOfNodes[i][j].positionInAdjacencymatrix;
			if( (criticalPathLinkExists[srcPosition][myPosition] == 1) && (isCollectiveOperation(arrayOfNodes[i][j].operationName) == 1)
					&& (adjacencyMatrix[srcPosition][myPosition] == arrayOfNodes[i][j].globalTimeElapsed))// Collective Operation will have many edges select the one that matched the Adjacency Value
			{
				criticalPathLinkExists[srcPosition][myPosition] = -1;// So that next time another arrow with equal weight is not  marked
				fprintf(dotFp,"\r\n%d->%d[label=\"%d\"][color=\"RED\"]",srcPosition,myPosition,arrayOfNodes[i][j].globalTimeElapsed);
			}
			else if(criticalPathLinkExists[srcPosition][myPosition] == 1)
			{
				fprintf(dotFp,"\r\n%d->%d[label=\"%d\"][color=\"RED\"]",srcPosition,myPosition,arrayOfNodes[i][j].globalTimeElapsed);
			}
			else
			{
				fprintf(dotFp,"\r\n%d->%d[label=\"%d\"]",srcPosition,myPosition,arrayOfNodes[i][j].globalTimeElapsed);
			}

		}
	}

	fprintf(dotFp,"\r\n}"); // Close the Braces after critPath is Found
	fclose(dotFp);


}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>


#define ANY_SOURCE_TAG 666
#define SYNC_TAG 555
#define TAG_ADD 200
#define PROTOCOL_TYPE "PROTOCOL_TYPE"

int nodeKilled =-1;
int barrierReachedFlag = 0;
int isParallelProtocol=0;//1 is YES 0 is NO
int *isRankAlive,*rankReachedBarrier;
int myActualRank=0,perceivedMyRank=0,realNumberOfRanks=0,numberOfRanks=0;
int isPrimary =1;// 1 is Primary 0 is Replica
int MPI_Init(int* argc,char*** argv)
{
	char *protocolType = (char*)malloc(15);
	PMPI_Init(argc,argv);
	PMPI_Comm_size(MPI_COMM_WORLD,&realNumberOfRanks);
	numberOfRanks = realNumberOfRanks/2;
	// Create and initialize array to keep track of alive and dead nodes
	isRankAlive = (int)malloc(realNumberOfRanks * sizeof(int));
	rankReachedBarrier = (int)malloc(realNumberOfRanks * sizeof(int));
	int i=0;
	for(i=0;i<realNumberOfRanks;i++)
	{
		isRankAlive[i] = 1;
	}
	for(i=0;i<realNumberOfRanks;i++)
	{
		rankReachedBarrier[i] = 0;
	}
	// End

	PMPI_Comm_rank(MPI_COMM_WORLD,&myActualRank);
	if(myActualRank >= numberOfRanks)
	{
		perceivedMyRank = myActualRank -  numberOfRanks;
		isPrimary = 0;
	}
	else
	{
		perceivedMyRank = myActualRank;
		isPrimary = 1;
	}
	if(myActualRank == 0)
	{
		protocolType = getenv(PROTOCOL_TYPE);
		printf("\n Type : %s",protocolType);
		if(protocolType == NULL)
		{
			printf("\n Some issue with the Protocol Type , will be set to MIRRORED By Default");
		}
		if(protocolType != NULL && (strcmp(protocolType,"PARALLEL") == 0))
		{
			isParallelProtocol = 1;
		}
		else
		{
			isParallelProtocol = 0;
		}
	}
	MPI_Bcast(&isParallelProtocol,1,MPI_INT,0,MPI_COMM_WORLD);

	//printf("\n[REAL Values] NumberOfRanks :: %d IsPrimary :: %d  MyRank :: %d ParallelProtocol :: %d",realNumberOfRanks,isPrimary,myActualRank,isParallelProtocol);
	//printf("\n[Assigned Values ]NumberOfRanks :: %d IsPrimary :: %d  MyRank :: %d",numberOfRanks,isPrimary,perceivedMyRank);
}
int MPI_Pcontrol(int killNode,...)
{
	int returnValue =0;
	if(isRankAlive[killNode] == 0)
	{
		returnValue = PMPI_Pcontrol(killNode);
		return returnValue;
	}
	//printf("\n %d Killed Node :: %d",myActualRank,killNode);
	isRankAlive[killNode] = 0;
	returnValue = PMPI_Pcontrol(killNode);
	return returnValue;
}
int MPI_Finalize()
{
	int returnValue =0;
	barrierReachedFlag=0;
	//printf("\n Check is node is dead before finalizing :: %d",myActualRank);
	if((isNodeAlive(myActualRank) == 0) && (hasNodeReachedBarrier(myActualRank) == 1))
	{
		PMPI_Finalize();
		exit(0);
	}
	if((isNodeAlive(myActualRank)==0))
	{
		//printf("\n returning from finalize");
		return returnValue;
	}

	returnValue = PMPI_Finalize();
	return returnValue;
}
int MPI_Barrier(MPI_Comm comm)
{
	int i=0;
	int barrierReachedFlag = 1;
	if((isNodeAlive(myActualRank) == 0) && (hasNodeReachedBarrier(myActualRank) == 1))
	{
		//printf("\n Node %d called Finalize",myActualRank);
		PMPI_Finalize();
		exit(0);
	}
	//printf("\n Node %d entered Barrier",myActualRank);
	barrierReachedFlag=0;
	//printf("\n Node %d reached the barrier",myActualRank);
	int areAllPrimaryAlive=1,killedNode=0;
	int sendBuffer = 1, recvBuffer = 0;
	MPI_Status status;

	int partner = getNodesPartner(0);
	if(isNodeAlive(0) == 1)// Rank 0 is alive
	{
		if(myActualRank == 0)
		{
			for(i=1;i<numberOfRanks;i++)
			{
				if(isNodeAlive(i) == 1)// Node is Alive Send it , else send to his prtner
				{
					PMPI_Send(&sendBuffer, 1, MPI_INT,i, 1000, MPI_COMM_WORLD);
					//printf("\n Receiving from rank %d to rank %d\n",i,myActualRank);
					PMPI_Recv(&recvBuffer, 1, MPI_INT, i, 1000, MPI_COMM_WORLD, &status);
				}
				else
				{
					PMPI_Send(&sendBuffer, 1, MPI_INT,getNodesPartner(i), 1000, MPI_COMM_WORLD);
					//printf("\n Receiving from rank %d to rank %d\n",i,myActualRank);
					PMPI_Recv(&recvBuffer, 1, MPI_INT, getNodesPartner(i), 1000, MPI_COMM_WORLD, &status);
				}
			}
		}
		else if(myActualRank != getNodesPartner(0))
		{
			//Check if primary was alive 1st , if he is dead means msg wasa sent to replica and he should reeive
			int partner = getNodesPartner(myActualRank);
			if((isPrimary == 1) && (isNodeAlive(myActualRank) == 1))
			{
				PMPI_Recv(&recvBuffer, 1, MPI_INT, 0, 1000, MPI_COMM_WORLD, &status);
				PMPI_Send(&sendBuffer, 1, MPI_INT,0, 1000, MPI_COMM_WORLD);
			}
			else if((isPrimary == 0) && (isNodeAlive(partner)== 0))
			{
				PMPI_Recv(&recvBuffer, 1, MPI_INT, 0, 1000, MPI_COMM_WORLD, &status);
				PMPI_Send(&sendBuffer, 1, MPI_INT,0, 1000, MPI_COMM_WORLD);
			}
		}

	}
	else
	{
		//printf("\nzero was dead using replica");
		int rankZerosReplica = getNodesPartner(0);
		if(myActualRank == rankZerosReplica)
		{
			for(i=1;i<numberOfRanks;i++)
			{
				if(isNodeAlive(i) == 1)// Node is Alive Send it
				{
					PMPI_Send(&sendBuffer, 1, MPI_INT,i, 1000, MPI_COMM_WORLD);
					//printf("\n Receiving from rank %d to rank %d\n",i,myActualRank);
					PMPI_Recv(&recvBuffer, 1, MPI_INT, i, 1000, MPI_COMM_WORLD, &status);
				}
				else
				{
					PMPI_Send(&sendBuffer, 1, MPI_INT,getNodesPartner(i), 1000, MPI_COMM_WORLD);
					PMPI_Recv(&recvBuffer, 1, MPI_INT, getNodesPartner(i), 1000, MPI_COMM_WORLD, &status);
				}
			}
		}
		else if(myActualRank != getNodesPartner(rankZerosReplica))
		{
			//Check if primary was alive 1st , if he is dead means msg wasa sent to replica and he should reeive
			int partner = getNodesPartner(myActualRank);
			if((isPrimary == 1) && (isNodeAlive(myActualRank) == 1))
			{
				PMPI_Recv(&recvBuffer, 1, MPI_INT, rankZerosReplica, 1000, MPI_COMM_WORLD, &status);
				PMPI_Send(&sendBuffer, 1, MPI_INT,rankZerosReplica, 1000, MPI_COMM_WORLD);
			}
			else if((isPrimary == 0) && (isNodeAlive(partner)== 0))
			{
				PMPI_Recv(&recvBuffer, 1, MPI_INT, rankZerosReplica, 1000, MPI_COMM_WORLD, &status);
				PMPI_Send(&sendBuffer, 1, MPI_INT,rankZerosReplica, 1000, MPI_COMM_WORLD);
			}
		}


	}
	for(i=0;i<realNumberOfRanks;i++)
	{
		if((isNodeAlive(i) == 0) && (hasNodeReachedBarrier(i) == 0))
		{
			rankReachedBarrier[i] = 1;
		}
	}

	//printf("\n Node %d exited Barrier",myActualRank);
}
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status)
{

	if((isNodeAlive(myActualRank) == 0) && (hasNodeReachedBarrier(myActualRank) == 1))
	{
		//printf("\n Node %d called Finalize",myActualRank);
		if(status != NULL)
		{
			int rankValue = myActualRank;
			if(myActualRank >= numberOfRanks)
			{
				rankValue = myActualRank-numberOfRanks;
			}
			status->MPI_SOURCE = rankValue;
		}
		PMPI_Finalize();
		exit(0);
	}
	barrierReachedFlag =0;
	int returnValue = 0;
	if((isNodeAlive(myActualRank)==0) && (hasNodeReachedBarrier(myActualRank) == 1))// Node is dead and reached barrier
	{
		if(status != NULL)
		{
			int rankValue = myActualRank;
			if(myActualRank >= numberOfRanks)
			{
				rankValue = myActualRank-numberOfRanks;
			}
			status->MPI_SOURCE = rankValue;
		}
		return returnValue;
	}

	if(source == MPI_ANY_SOURCE)
	{
		//printf("\n Any Source message");
		if(isParallelProtocol == 0)// MIRROR
		{
			if(isPrimary == 1)
			{
				MPI_Status tempStatus;
				returnValue = PMPI_Recv(buf,count,datatype,MPI_ANY_SOURCE,MPI_ANY_TAG,comm,&tempStatus);// create status
				int receivedSrc = tempStatus.MPI_SOURCE;
				int receivedTag = tempStatus.MPI_TAG;
				//printf("\n Received from %d with tag %d",receivedSrc,receivedTag);
				/*if(receivedTag >= 200)
				{
					receivedSrc =receivedSrc;// Means i received a AnySource from a Replica
					printf("\n settig Receivedsrc %d ",receivedSrc);
				}*/

				int replicaNode = getNodesPartner(myActualRank);
				if( ((isNodeAlive(replicaNode)== 0) || (isNodeAlive(replicaNode) == 1))
						&& (hasNodeReachedBarrier(replicaNode) != 1) )// If node is alive and has not reached barrier proceede
				{
					//printf("\n %d should receive from %d",replicaNode,receivedSrc);
					PMPI_Send(&receivedSrc, 1, MPI_INT,replicaNode, ANY_SOURCE_TAG,MPI_COMM_WORLD);
				}
				// Add condition to send to the opposito node of received src
				int receivedSrcPartner = getNodesPartner(receivedSrc);
				if( ((isNodeAlive(receivedSrcPartner)== 0) || (isNodeAlive(receivedSrcPartner) == 1))// partner is Lead Node
						&& (hasNodeReachedBarrier(receivedSrcPartner) != 1) )// If node is alive and has not reached barrier proceede
				{
					int tagValue = receivedTag;
					if(isPrimaryNode(receivedSrcPartner))
					{
						if(receivedTag >= 200)
						{
							tagValue = receivedTag-200;
						}
					}
					else
					{
						tagValue = receivedTag + 200;
					}
				//	printf("\n Receiving on %d from %d with tag",myActualRank,receivedSrcPartner);
					returnValue = PMPI_Recv(buf,count,datatype,receivedSrcPartner,tagValue,comm,status);
				}


			}
			else// If he is Replica
			{
			//	printf("\n Replica %d",myActualRank);
				int receivedSrc = -1;
				int leadNode = getNodesPartner(myActualRank);// check if alive
				if( ((isNodeAlive(leadNode)== 0) || (isNodeAlive(leadNode) == 1))// partner is Lead Node
						&& (hasNodeReachedBarrier(leadNode) != 1) )// If node is alive and has not reached barrier proceede
				{
					PMPI_Recv(&receivedSrc,1,MPI_INT,leadNode,ANY_SOURCE_TAG,comm,MPI_STATUS_IGNORE);
					//printf("\n Replica %d will begin receiving from %d ",myActualRank,receivedSrc);
					if(receivedSrc > -1)
					{
						int recvTag ;
						if(isPrimaryNode(receivedSrc))
						{
							recvTag = tag;
						}
						else
						{
							recvTag = tag+200;
						}
					//	printf("\n Receiving on %d from %d with tag %d",myActualRank,receivedSrc,recvTag);
						returnValue = PMPI_Recv(buf,count,datatype,receivedSrc,recvTag,comm,status);// create status

						int replicaOfNodeReceived = getNodesPartner(receivedSrc);

						if( ((isNodeAlive(replicaOfNodeReceived)== 0) || (isNodeAlive(replicaOfNodeReceived) == 1))
								&& (hasNodeReachedBarrier(replicaOfNodeReceived) != 1) )// If node is alived and has not reached barrier proceede
						{
							int tagValue;
							if(isPrimaryNode(replicaOfNodeReceived))
							{
								tagValue = tag;
							}
							else
							{
								tagValue = tag + 200;
							}
						//	printf("\n Receiving on %d from %d with tag %d",myActualRank,replicaOfNodeReceived,tagValue);
							returnValue = PMPI_Recv(buf,count,datatype,replicaOfNodeReceived,tagValue,comm,status);
						}
					}
				}
				else//Lead node is dead i will handle both my receives
				{
				//	printf("\n Lead is Dead i(%d) will do all the work",myActualRank);
					MPI_Status tempStatus;
					returnValue = PMPI_Recv(buf,count,datatype,MPI_ANY_SOURCE,MPI_ANY_TAG,comm,&tempStatus);// create status
					int receivedSrc = tempStatus.MPI_SOURCE;
					int receivedTag = tempStatus.MPI_TAG;
					int replicaOfReceivedSrc = getNodesPartner(receivedSrc);
				//	printf("\n **Received on %d from %d with tag %d",myActualRank,receivedSrc,receivedTag);
					if( ((isNodeAlive(replicaOfReceivedSrc)== 0) || (isNodeAlive(replicaOfReceivedSrc) == 1))
							&& (hasNodeReachedBarrier(replicaOfReceivedSrc) != 1) )// If node is alived and has not reached barrier proceede
					{
						int tagValue;
						if(isPrimaryNode(replicaOfReceivedSrc))
						{
							if(receivedTag >= 200)
							{
								tagValue = receivedTag-200;
							}
							else
							{
								tagValue = receivedTag;
							}
						}
						else
						{
							tagValue = receivedTag + 200;
						}
				//		printf("\n Receiving on %d from %d with tag %d",myActualRank,replicaOfReceivedSrc,tagValue);
						returnValue = PMPI_Recv(buf,count,datatype,replicaOfReceivedSrc,tagValue,comm,status);
					}
				}


			}
		}
		else// Parallel
		{
		//	printf("\n Parallel Any Source");
			if(isPrimary == 1)
			{
				MPI_Status tempStatus;
				returnValue = PMPI_Recv(buf,count,datatype,MPI_ANY_SOURCE,MPI_ANY_TAG,comm,&tempStatus);// create status
				int receivedSrc = tempStatus.MPI_SOURCE;
				int receivedTag = tempStatus.MPI_TAG;
				int myReplica = getNodesPartner(myActualRank);
				if( ((isNodeAlive(myReplica)== 0) || (isNodeAlive(myReplica) == 1))
						&& (hasNodeReachedBarrier(myReplica) != 1) )// If node is alived and has not reached barrier proceede
				{
			//		printf("\n %d should receive from %d",myReplica,receivedSrc);
					PMPI_Send(&receivedSrc, 1, MPI_INT,myReplica, ANY_SOURCE_TAG,MPI_COMM_WORLD);
				}

			}
			else
			{
				int receivedSrc =-1;
				//Check  if Primary is alive , If he is he would have sent the source to recceive from
				//Check if sourcesPartner is alive if yes receive from him , else receive from receivedSrc
				//If primary is dead receive and check who we recceived from then receive from opp guy
				int primaryNode = getNodesPartner(myActualRank);
				if( ((isNodeAlive(primaryNode)== 0) || (isNodeAlive(primaryNode) == 1))
						&& (hasNodeReachedBarrier(primaryNode) != 1) )// If node is alived and has not reached barrier proceede
				{

					PMPI_Recv(&receivedSrc,1,MPI_INT,primaryNode,ANY_SOURCE_TAG,comm,MPI_STATUS_IGNORE);
					if(receivedSrc > -1)
					{
						//printf("\n My Lead recceived from %d",receivedSrc);
						if(isPrimaryNode(receivedSrc) == 1)
						{
							int partnerOfReceivedSrc = getNodesPartner(receivedSrc);
							if( ((isNodeAlive(partnerOfReceivedSrc)== 0) || (isNodeAlive(partnerOfReceivedSrc) == 1))
									&& (hasNodeReachedBarrier(partnerOfReceivedSrc) != 1) )// If node is alived and has not reached barrier proceede
							{
						//		printf("\n Receiving on %d from %d with tag %d",myActualRank,partnerOfReceivedSrc,tag);
								returnValue = PMPI_Recv(buf,count,datatype,partnerOfReceivedSrc,tag,comm,status);// create status
							}
							else
							{
						//		printf("\n Receiving on %d from %d with tag %d",myActualRank,receivedSrc,tag);
								returnValue = PMPI_Recv(buf,count,datatype,receivedSrc,tag,comm,status);// create status
							}
						}
						else
						{
						//	printf("\n Receiving on %d from %d with tag %d",myActualRank,receivedSrc,tag);
							returnValue = PMPI_Recv(buf,count,datatype,receivedSrc,tag,comm,status);// create status
						}

					}
				}
				else
				{
					returnValue = PMPI_Recv(buf,count,datatype,MPI_ANY_SOURCE,MPI_ANY_TAG,comm,status);// create status
				}
			}

		}
	}
	else
	{
		if(isParallelProtocol == 0)// MIRROR
		{
			int partnerInGroup = fetchDestinationInMyGroup(myActualRank,source);
			int crossPartner = fetchDestinationInOppositeGroup(myActualRank,source);
			if( ((isNodeAlive(partnerInGroup)== 0) || (isNodeAlive(partnerInGroup) == 1))
					&& (hasNodeReachedBarrier(partnerInGroup) != 1) )// If node is alived and has not reached barrier proceede
			{

				if(partnerInGroup >= numberOfRanks)// To distinguish between Primary and Replica add TAG_ADD value
				{
					//printf("\n Receiving from rank %d to rank %d Tag %d\n",partnerInGroup,myActualRank,tag+200);
					returnValue = PMPI_Recv(buf,count,datatype,partnerInGroup,tag+200,comm,status);
				}
				else
				{
					//printf("\n Receiving from rank %d to rank %d Tag %d\n",partnerInGroup,myActualRank,tag);
					returnValue = PMPI_Recv(buf,count,datatype,partnerInGroup,tag,comm,status);
				}
			}
			if( ((isNodeAlive(crossPartner)== 0) || (isNodeAlive(crossPartner) == 1))
					&& (hasNodeReachedBarrier(crossPartner) != 1) )// If node is alived and has not reached barrier proceede
			{

				if(crossPartner >= numberOfRanks)// To distinguish between Primary and Replica add TAG_ADD value
				{
					//printf("\n Receiving from rank %d to rank %d Tag %d\n",crossPartner,myActualRank,tag+200);
					returnValue = PMPI_Recv(buf,count,datatype,crossPartner,tag+200,comm,status);
				}
				else
				{
					//printf("\n Receiving from rank %d to rank %d Tag %d\n",crossPartner,myActualRank,tag);
					returnValue = PMPI_Recv(buf,count,datatype,crossPartner,tag,comm,status);
				}
			}

		}
		else
		{
			// Sending the message
			int destinationInSameGroup = fetchDestinationInMyGroup(myActualRank,source);
			int destinationInCrossGroupp = fetchDestinationInOppositeGroup(myActualRank,source);
			// If i am Primary and i am alive and my destination is alive send it to him
			//also if my replica is dead send it to cross destination if he is alive
			//if i am replica i am alive and my destination is alive send it to him
			//also if my primary is dead send it to my cross destination if he is alive
			if(isPrimary == 1 && ((isNodeAlive(myActualRank)== 0) || (isNodeAlive(myActualRank) == 1))
					&& (hasNodeReachedBarrier(myActualRank) != 1) )// If node is alived and has not reached barrier proceede
			{
				if( ((isNodeAlive(destinationInSameGroup)== 0) || (isNodeAlive(destinationInSameGroup) == 1))
						&& (hasNodeReachedBarrier(destinationInSameGroup) != 1) )// If node is alived and has not reached barrier proceede
				{
				//	printf("\n Receiving from rank %d on rank %d\n",destinationInSameGroup,myActualRank);
					returnValue = PMPI_Recv(buf,count,datatype,destinationInSameGroup,tag,comm,status);
				}
				else
				{

					if( ((isNodeAlive(destinationInCrossGroupp)== 0) || (isNodeAlive(destinationInCrossGroupp) == 1))
							&& (hasNodeReachedBarrier(destinationInCrossGroupp) != 1) )// If node is alived and has not reached barrier proceede
					{
				//		printf("\n Receiving from rank %d on rank %d\n",destinationInCrossGroupp,myActualRank);
						returnValue = PMPI_Recv(buf,count,datatype,destinationInCrossGroupp,tag,comm,status);
					}

				}
			}
			else if(isPrimary == 0 && ((isNodeAlive(myActualRank)== 0) || (isNodeAlive(myActualRank) == 1))
					&& (hasNodeReachedBarrier(myActualRank) != 1) )
			{
				if( ((isNodeAlive(destinationInSameGroup)== 0) || (isNodeAlive(destinationInSameGroup) == 1))
						&& (hasNodeReachedBarrier(destinationInSameGroup) != 1) )// If node is alived and has not reached barrier proceede
				{
				//	printf("\n Receiving from rank %d on rank %d\n",destinationInSameGroup,myActualRank);
					returnValue = PMPI_Recv(buf,count,datatype,destinationInSameGroup,tag,comm,status);
				}
				else
				{
					if( ((isNodeAlive(destinationInCrossGroupp)== 0) || (isNodeAlive(destinationInCrossGroupp) == 1))
							&& (hasNodeReachedBarrier(destinationInCrossGroupp) != 1) )// If node is alived and has not reached barrier proceede
					{
				//		printf("\n Receiving from rank %d on rank %d\n",destinationInCrossGroupp,myActualRank);
						returnValue = PMPI_Recv(buf,count,datatype,destinationInCrossGroupp,tag,comm,status);
					}
				}
			}
		}
	}
	if(status != NULL)
	{
		int rankValue = myActualRank;
		if(myActualRank >= numberOfRanks)
		{
			rankValue = myActualRank-numberOfRanks;
		}
		status->MPI_SOURCE = rankValue;
	}
}
int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,MPI_Comm comm)
{
	//printf("\n Here");
	if((isNodeAlive(myActualRank) == 0) && (hasNodeReachedBarrier(myActualRank) == 1) )
	{
		//printf("\n Node %d called Finalize",myActualRank);
		PMPI_Finalize();
		exit(0);
	}
	barrierReachedFlag =0;
	int returnValue = 0;
	if((isNodeAlive(myActualRank)==0) && (hasNodeReachedBarrier(myActualRank) == 1))
	{
		return returnValue;
	}
	//printf("\n Here1");
	if(isParallelProtocol == 0)// MIRROR
	{
		//printf("\n Here3");
		if(isPrimary == 0)// To distinguish between Primary and Replica add TAG_ADD value
		{
			tag = tag + 200;
		}
		int partnerInGroup = fetchDestinationInMyGroup(myActualRank,dest);
		int crossPartner = fetchDestinationInOppositeGroup(myActualRank,dest);
		if( ((isNodeAlive(partnerInGroup)== 0) || (isNodeAlive(partnerInGroup) == 1))
				&& (hasNodeReachedBarrier(partnerInGroup) != 1) )// If node is alived and has not reached barrier proceede
		{
			//printf("\n Sending from rank %d to rank %d Tag %d\n",myActualRank,partnerInGroup,tag);
			returnValue = PMPI_Send(buf,count,datatype,partnerInGroup,tag,comm);
		}
		if( ((isNodeAlive(crossPartner)== 0) || (isNodeAlive(crossPartner) == 1))
				&& (hasNodeReachedBarrier(crossPartner) != 1) )// If node is alived and has not reached barrier proceede
		{
			//printf("\n Sending from rank %d to rank %d Tag %d\n",myActualRank,crossPartner,tag);
			returnValue = PMPI_Send(buf,count,datatype,crossPartner,tag,comm);
		}
	}
	else// Parallel Protocol
	{
		//printf("\n Parallel");
		// Sync up 1st if Replica is alive
		MPI_Status syncStat;
		int ptner = getNodesPartner(myActualRank);
		if( ((isNodeAlive(ptner)== 0) || (isNodeAlive(ptner) == 1))
				&& (hasNodeReachedBarrier(ptner) != 1) )// If node is alived and has not reached barrier proceede
		{
			/*if(isNodeAlive(getNodesPartner(myActualRank)) == 1)// Do sync up only if partner is alive else no use
		{
			 */	if(isPrimary == 1)
			 {
				 int replicaNode = getNodesPartner(myActualRank);
				 if( ((isNodeAlive(replicaNode)== 0) || (isNodeAlive(replicaNode) == 1))
						 && (hasNodeReachedBarrier(replicaNode) != 1) )// If node is alived and has not reached barrier proceede
				 {
					 PMPI_Send(NULL, 0, MPI_INT,replicaNode, SYNC_TAG, MPI_COMM_WORLD);
					 PMPI_Recv(NULL, 0, MPI_INT, replicaNode, SYNC_TAG, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
				 }
			 }
			 else
			 {
				 int  leadNode= getNodesPartner(myActualRank);
				 if( ((isNodeAlive(leadNode)== 0) || (isNodeAlive(leadNode) == 1))
						 && (hasNodeReachedBarrier(leadNode) != 1) )// If node is alived and has not reached barrier proceede
				 {
					 PMPI_Recv(NULL, 0, MPI_INT, leadNode, SYNC_TAG, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
					 PMPI_Send(NULL, 0, MPI_INT,leadNode, SYNC_TAG, MPI_COMM_WORLD);

				 }

			 }
		}
		else
		{
		//	printf("\n Partner is Dead no point syncing up in Parallel");
		}
		// End of Sync up
		// Sending the message
		//printf("\n Send Code");
		int destinationInSameGroup = fetchDestinationInMyGroup(myActualRank,dest);
		int destinationInCrossGroupp = fetchDestinationInOppositeGroup(myActualRank,dest);
		// If i am Primary and i am alive and my destination is alive send it to him
		//also if my replica is dead send it to cross destination if he is alive
		//if i am replica i am alive and my destination is alive send it to him
		//also if my primary is dead send it to my cross destination if he is alive
		if(isPrimary == 1 && ((isNodeAlive(myActualRank)== 0) || (isNodeAlive(myActualRank) == 1))
				&& (hasNodeReachedBarrier(myActualRank) != 1) )// If node is alived and has not reached barrier proceede
		{
			if( ((isNodeAlive(destinationInSameGroup)== 0) || (isNodeAlive(destinationInSameGroup) == 1))
					&& (hasNodeReachedBarrier(destinationInSameGroup) != 1) )// If node is alived and has not reached barrier proceede
			{
		//		printf("\n Sending from rank %d to rank %d\n",myActualRank,destinationInSameGroup);
				returnValue = PMPI_Send(buf,count,datatype,destinationInSameGroup,tag,comm);
			}
			int replicaNode = getNodesPartner(myActualRank);
			if((isNodeAlive(replicaNode) == 0) 	&& (hasNodeReachedBarrier(replicaNode) == 1) )// If node is alive || dead and has not reached barrier proceede
			{
				if( ((isNodeAlive(destinationInCrossGroupp)== 0) || (isNodeAlive(destinationInCrossGroupp) == 1))
						&& (hasNodeReachedBarrier(destinationInCrossGroupp) != 1) )// If node is alived and has not reached barrier proceede
				{
		//			printf("\n Sending from rank %d to rank %d\n",myActualRank,destinationInCrossGroupp);
					returnValue = PMPI_Send(buf,count,datatype,destinationInCrossGroupp,tag,comm);
				}
			}
		}
		else if(isPrimary == 0 && ((isNodeAlive(myActualRank)== 0) || (isNodeAlive(myActualRank) == 1))
				&& (hasNodeReachedBarrier(myActualRank) != 1) )
		{
			if( ((isNodeAlive(destinationInSameGroup)== 0) || (isNodeAlive(destinationInSameGroup) == 1))
					&& (hasNodeReachedBarrier(destinationInSameGroup) != 1) )// If node is alived and has not reached barrier proceede
			{
		//		printf("\n Sending from rank %d to rank %d\n",myActualRank,destinationInSameGroup);
				returnValue = PMPI_Send(buf,count,datatype,destinationInSameGroup,tag,comm);
			}
			int primaryNode = getNodesPartner(myActualRank);
			if((isNodeAlive(primaryNode) == 0) 	&& (hasNodeReachedBarrier(primaryNode) == 1) )// If node is alive || dead and has not reached barrier proceede
			{
				if( ((isNodeAlive(destinationInCrossGroupp)== 0) || (isNodeAlive(destinationInCrossGroupp) == 1))
						&& (hasNodeReachedBarrier(destinationInCrossGroupp) != 1) )// If node is alived and has not reached barrier proceede
				{
		//			printf("\n Sending from rank %d to rank %d\n",myActualRank,destinationInCrossGroupp);
					returnValue = PMPI_Send(buf,count,datatype,destinationInCrossGroupp,tag,comm);
				}
			}
		}





		// End of send
	}
	return returnValue;

}
int MPI_Comm_size( MPI_Comm comm, int *size )
{
	*size = numberOfRanks;
}
int MPI_Comm_rank( MPI_Comm comm, int *rank )
{
	*rank = perceivedMyRank;
}
/*
 *  Function to find the corresponnding Destination , need to resolve the replicas naming convention ( i.e for 8 nodes , MyRank 4 is replicaRank 0)
 */
int fetchDestinationInMyGroup(int myActualRank,int destination)
{
	if(myActualRank < numberOfRanks)
	{
		return destination;
	}
	else
	{
		return destination + numberOfRanks;
	}
}
int fetchDestinationInOppositeGroup(int myActualRank,int destination)
{
	if(myActualRank < numberOfRanks)
	{
		return destination + numberOfRanks;
	}
	else
	{
		return destination ;
	}
}
int isNodeAlive(int rank)
{

	if(isRankAlive[rank] == 1)
	{
		return 1; // Node is Alive
	}
	else
	{
		return 0; // Node is Dead
	}
}

int hasNodeReachedBarrier(int rank)
{

	if(rankReachedBarrier[rank] == 1)
	{
		return 1; // Node is Alive
	}
	else
	{
		return 0; // Node is Dead
	}
}
int getNodesPartner(int rank)
{
	if(rank < numberOfRanks)
	{
		return numberOfRanks+rank;
	}
	else
	{
		return rank-numberOfRanks;
	}
}

int isPrimaryNode(int rank)
{
	if(rank < numberOfRanks)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


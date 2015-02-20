#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#define TAG 13

int MATRIX_SIZE =0,numnodes,maxiters;
double MAX_DIFF=0;
double max_value(double number1,double number2)
{
    if(fabs(number1) > fabs(number2))
    {
     return fabs(number1);
    }
    else
    {
        return fabs(number2);
    }
}
double ** allocateMatrix(int myrank)
{
    double **index;
    double *matrixBlock;
    int i=0;
    if(myrank == 0)
    {
        int sizeOfMatrix = MATRIX_SIZE+2;

        matrixBlock = (double*)malloc( (sizeOfMatrix)*(sizeOfMatrix)*sizeof(double));
        index = (double**)malloc( (sizeOfMatrix)*sizeof(double*));
        for(i = 0 ;	 i < sizeOfMatrix;i++)
        {
            index[i]=&(matrixBlock[i*(sizeOfMatrix)]);
        }
        return index;
    }
    else
    {
        int sizeOfMatrix = MATRIX_SIZE+2;

        matrixBlock = (double*)malloc((sizeOfMatrix)*((MATRIX_SIZE/numnodes)+2)*sizeof(double));
        index = (double**)malloc( (sizeOfMatrix)*sizeof(double*));
        for(i = 0 ;	 i < ((MATRIX_SIZE/numnodes)+2);i++)
        {
            index[i]=&(matrixBlock[i*(sizeOfMatrix)]);
        }
        return index;

    }

}
void initialize_matrix(double ** orginalGrid,int myrank)
{
    int i,j;

        for(i=0;i < MATRIX_SIZE +2 ;i++)
        {
            for(j=0;j<MATRIX_SIZE +2;j++)
            {
                if(i == 0 || j == 0 || i == (MATRIX_SIZE +1) || j == (MATRIX_SIZE +1))// Setting up Boundry
                {
                    orginalGrid[i][j] = 1;
                }
                else
                {
                    orginalGrid[i][j] = 0;
                }
            }
        }


}
void printMatrix(double ** orginalGrid,int myrank)
{
    if(MATRIX_SIZE <  10 && myrank == 0)
    {
        printf("\n Contents of Matrix Are :: \n");
        int i=0,j=0;
        for(i=0;i<MATRIX_SIZE+2;i++)
        {
            for(j=0;j<MATRIX_SIZE+2;j++)
            {
                printf("%lf ",orginalGrid[i][j]);
            }
            printf("\n");
        }
         printf("\n");
    }
}
void printReceivedValues(int myrank,double ** orginalGrid,int stripsize)
{
    int i,j;
    printf("\nResult for Myrank :: %d",myrank);
    printf("\n");
    for(i=1;i<stripsize-1;i++)
    {

        for(j=0;j<(MATRIX_SIZE+2);j++)
        {
            if(i == 0 || j == 0 || i == (MATRIX_SIZE +1) || j == (MATRIX_SIZE +1))// Setting up Boundry
            {
                printf("%lf ",orginalGrid[i][j]);
            }
            else
            {
                printf("%lf ",((orginalGrid[i][j])+(myrank)));
            }

        }
         printf("\n");
    }
    printf("\n");
}
int main(int argc,char *argv[])
{
    int numElements, offset,height, stripsize, myrank, i, j, iter ,startCol;
    double startTime, endTime,temp,local_max_diff=0;
    double **orginalGrid ;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
    MPI_Comm_size(MPI_COMM_WORLD,&numnodes);

    MATRIX_SIZE = atoi(argv[1]);
    maxiters = atoi(argv[2]);
    //printf("\n Size Of Matrix :: %d",MATRIX_SIZE);


    orginalGrid = allocateMatrix(myrank);// Allocate the entire Matrix for rank 0 , else allocate its part

    if(myrank == 0)
    {
        initialize_matrix(orginalGrid,myrank);
       // printMatrix(orginalGrid,myrank);
    }

    height =(MATRIX_SIZE/numnodes);
    stripsize = height +2;
    //printf("\n Strip Size :: %d",stripsize);
    //printf("\n NumNodes :: %d",numnodes);
    if (myrank == 0)
    {
        startTime = MPI_Wtime();
    }
    if(myrank == 0)// Write code to send correcct strip to All
    {
        offset = height;
        numElements = stripsize * (MATRIX_SIZE+2);
        for (i=1; i<numnodes; i++)
        {
            MPI_Send(orginalGrid[offset], numElements, MPI_DOUBLE, i, TAG, MPI_COMM_WORLD);
            offset += height;
        }
        local_max_diff=0;
    }
    else// Write code to receive correct strip
    {
        numElements = stripsize * (MATRIX_SIZE+2);
        MPI_Recv(orginalGrid[0], numElements, MPI_DOUBLE, 0, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        local_max_diff=0;
    }
    //Print what was received For Debugging
    // --------printReceivedValues(myrank,orginalGrid,stripsize);


    // Compute Red and Black Values
    MPI_Barrier(MPI_COMM_WORLD);  // Wait till all the processes arrive with their respective strips before we begin computation
    for(iter=0;iter<=maxiters;iter++)
    {
        for(i=1;i<=height;i++)// Compute Red 1st
        {
             //Check if it is odd row or even
            if(i%2 == 1)
            {
                startCol = 1;//Odd
            }
            else
            {
                startCol =2;//Even
            }
            for(j=startCol;j<=MATRIX_SIZE;j=j+2)
            {
                temp = orginalGrid[i][j];
                orginalGrid[i][j] = (orginalGrid[i-1][j]+orginalGrid[i+1][j]+orginalGrid[i][j-1]+orginalGrid[i][j+1]) * .25;
                if(iter == maxiters)
                    local_max_diff = max_value(MAX_DIFF,(fabs(orginalGrid[i][j])-fabs(temp)));
            }
        }

        MPI_Barrier(MPI_COMM_WORLD); // wait for all processes to complete execution of their red bocks
        if(numnodes < 2)
        {
            // Dont do anything this is like sequential
        }
        else
        {
            if(myrank == 0 )
            {
                MPI_Send(orginalGrid[height], MATRIX_SIZE+2, MPI_DOUBLE, myrank+1 , TAG, MPI_COMM_WORLD);
                MPI_Recv(orginalGrid[height+1], MATRIX_SIZE+2, MPI_DOUBLE, myrank+1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            else if(myrank == (numnodes-1))
            {
                MPI_Recv(orginalGrid[0], MATRIX_SIZE+2, MPI_DOUBLE, myrank-1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(orginalGrid[1], MATRIX_SIZE+2, MPI_DOUBLE, myrank-1 , TAG, MPI_COMM_WORLD);

            }
            if(myrank%2 == 0 && (myrank != 0 && (myrank != (numnodes-1))))
            {
                MPI_Send(orginalGrid[1], MATRIX_SIZE+2, MPI_DOUBLE, myrank-1 , TAG, MPI_COMM_WORLD);
                MPI_Recv(orginalGrid[0], MATRIX_SIZE+2, MPI_DOUBLE, myrank-1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(orginalGrid[height], MATRIX_SIZE+2, MPI_DOUBLE, myrank+1 , TAG, MPI_COMM_WORLD);
                MPI_Recv(orginalGrid[height+1], MATRIX_SIZE+2, MPI_DOUBLE, myrank+1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            if(myrank%2 != 0 && (myrank != 0 && (myrank != (numnodes-1))))
            {
                MPI_Recv(orginalGrid[0], MATRIX_SIZE+2, MPI_DOUBLE, myrank-1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(orginalGrid[1], MATRIX_SIZE+2, MPI_DOUBLE, myrank-1 , TAG, MPI_COMM_WORLD);
                MPI_Recv(orginalGrid[height+1], MATRIX_SIZE+2, MPI_DOUBLE, myrank+1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(orginalGrid[height], MATRIX_SIZE+2, MPI_DOUBLE, myrank+1 , TAG, MPI_COMM_WORLD);
            }
        }
	//MPI_Pcontrol(1);
            MPI_Barrier(MPI_COMM_WORLD); // wait till all processes complete execution of their black blocks
            for(i=1;i<=height;i++)//Compute Black Blocks
            {
                //Check if it is odd row or even
                if(i%2 == 1)
                {
                    startCol = 2;//Odd
                }
                else
                {
                    startCol =1;//Even
                }
                for(j=startCol;j<=MATRIX_SIZE;j=j+2)
                {
                    temp = orginalGrid[i][j];
                    orginalGrid[i][j] = (orginalGrid[i-1][j]+orginalGrid[i+1][j]+orginalGrid[i][j-1]+orginalGrid[i][j+1]) * .25;
                    if(iter == maxiters)
                        local_max_diff = max_value(MAX_DIFF,(fabs(orginalGrid[i][j])-fabs(temp)));
                }
            }
        if(numnodes < 2)
        {
            // Dont do anything this is like sequential
        }
        else
        {
            if(myrank == 0)
            {
                MPI_Send(orginalGrid[height], MATRIX_SIZE+2, MPI_DOUBLE, myrank+1 , TAG, MPI_COMM_WORLD);
                MPI_Recv(orginalGrid[height+1], MATRIX_SIZE+2, MPI_DOUBLE, myrank+1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            else if(myrank == (numnodes-1))
            {
                MPI_Recv(orginalGrid[0], MATRIX_SIZE+2, MPI_DOUBLE, myrank-1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(orginalGrid[1], MATRIX_SIZE+2, MPI_DOUBLE, myrank-1 , TAG, MPI_COMM_WORLD);

            }
            if(myrank%2 == 0 && (myrank != 0 && (myrank != (numnodes-1))))
            {
                MPI_Send(orginalGrid[1], MATRIX_SIZE+2, MPI_DOUBLE, myrank-1 , TAG, MPI_COMM_WORLD);
                MPI_Recv(orginalGrid[0], MATRIX_SIZE+2, MPI_DOUBLE, myrank-1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(orginalGrid[height], MATRIX_SIZE+2, MPI_DOUBLE, myrank+1 , TAG, MPI_COMM_WORLD);
                MPI_Recv(orginalGrid[height+1], MATRIX_SIZE+2, MPI_DOUBLE, myrank+1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            }
            if(myrank%2 != 0 && (myrank != 0 && (myrank != (numnodes-1))))
            {
                MPI_Recv(orginalGrid[0], MATRIX_SIZE+2, MPI_DOUBLE, myrank-1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(orginalGrid[1], MATRIX_SIZE+2, MPI_DOUBLE, myrank-1 , TAG, MPI_COMM_WORLD);
                MPI_Recv(orginalGrid[height+1], MATRIX_SIZE+2, MPI_DOUBLE, myrank+1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(orginalGrid[height], MATRIX_SIZE+2, MPI_DOUBLE, myrank+1 , TAG, MPI_COMM_WORLD);

            }
        }
    }
    MPI_Reduce(&local_max_diff,&MAX_DIFF,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);


    // Collect from everyone
    if(myrank == 0)// Receive from Everyone their chunk
    {
        offset = height;
        numElements = stripsize * (MATRIX_SIZE+2);
        for (i=1; i<numnodes; i++)
        {
            MPI_Recv(orginalGrid[offset+1], (height*(MATRIX_SIZE+2)), MPI_DOUBLE, i, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            offset += height;
        }
    }
    else// Send my chunk to 0
    {
        MPI_Send(orginalGrid[1], (height*(MATRIX_SIZE+2)), MPI_DOUBLE, 0, TAG, MPI_COMM_WORLD);
    }

    //End of Computation
   // printMatrix(orginalGrid,myrank);

    if(myrank == 0)
    {
        endTime = MPI_Wtime();
       // printf("\nResults are ::\n");
        //printf("MPI Ranks :: %d\t",numnodes);
	//printf("Number of threads :: 0 \t");
        printf("\nExecution time :: %0.3f\t",(endTime-startTime));
       printf("Maximum Difference :: %lf\t\n",MAX_DIFF);
    }

    MPI_Finalize();
    return 0;


}

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <math.h>

int numOfThread=0,matrixSize =0;
int MAX_ITERATION = 0;
volatile double **orginalGrid ;
pthread_barrier_t   barrier;
volatile int *barrierArray;
double MAX_DIFF =0;
double *max_diff_array;

double max_value(double number1,double number2)
{
    return (fabs(number1)>fabs(number2) ?fabs(number1) : fabs(number2));
}
double findLargestDiffInArray()
{
    int i =0;
    double temp =0;
    for(i=0;i<numOfThread;i++)
    {
        if( fabs(max_diff_array[i]) > fabs(temp))
        {
            temp = max_diff_array[i];
        }
    }
    return fabs(temp);
}
int * initializeBarrier()
{
    int * barrierIndex;
    int i =0;
    barrierIndex = (int*)malloc((numOfThread)*sizeof(int));
    for(i=1;i<=numOfThread;i++)
    {
        barrierIndex[i] = 0;
    }
    return barrierIndex;
}
double * initialize_max_diff_array()
{
    double * maxDiffArray;
    int i =0;
    maxDiffArray = (double*)malloc((numOfThread)*sizeof(double));
    for(i=1;i<=numOfThread;i++)
    {
        maxDiffArray[i] = 0;
    }
    return maxDiffArray;
}
void barrierFun(int id)
{
    int j =0,lookAt=0;
    for(j=1;j <= ceil(log(numOfThread));j++)
    {
	//printf("\nAt barrierFun barrierArray{} = %d for id : %d",barrierArray[id],id);
        while(barrierArray[id] != 0);
        barrierArray[id] = j;
        lookAt = ( (id + (int)pow(2,(j-1))) % numOfThread);
	//printf("\n Look AT : %d",lookAt);
	//printf("\nAt 2 barrierFun barrierArray{lookAt} = %d for id : %d J= %d",barrierArray[lookAt],id,j);
        while(barrierArray[lookAt] != j);
        barrierArray[lookAt] = 0;

    }
}
double** allocateMatrix()
{
    double **index;
    double *matrixBlock;
    int i=0;
    int sizeOfMatrix = matrixSize+2;

    matrixBlock = (double*)malloc( (sizeOfMatrix)*(sizeOfMatrix)*sizeof(double));
    index = (double**)malloc( (sizeOfMatrix)*sizeof(double*));
    for(i = 0 ;	 i < sizeOfMatrix;i++)
    {
        index[i]=&(matrixBlock[i*(sizeOfMatrix)]);
    }
    return index;

}
void *worker(void *arg)
{
    int id = *((int*)arg);
    computeValue(id);
    return NULL;

}
void computeValue(int id)
{
    int height = matrixSize/numOfThread;
    int firstRow = (id * height) +1;
    int lastRow = firstRow + height -1;
    int i=0,iter=0,j=0,startCol=0;
    double temp=0;
    //pthread_barrier_wait(&barrier);
   // printf("\nAt 1st Barrier id :: %d",id);
    barrierFun(id);//Wait till all threads arrive before computing the RED BLACK Grid
    for(iter=0;iter<=MAX_ITERATION;iter++)
    {

        for(i=firstRow;i<=lastRow;i++)//Compute Red Blocks
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
            for(j=startCol;j<=matrixSize;j=j+2)
            {
                temp = orginalGrid[i][j];
                orginalGrid[i][j] = (orginalGrid[i-1][j]+orginalGrid[i+1][j]+orginalGrid[i][j-1]+orginalGrid[i][j+1]) * .25;
                if(iter == MAX_ITERATION)
                {
                    MAX_DIFF= max_value(MAX_DIFF,orginalGrid[i][j]-fabs(temp));
                }

            }
        }
       // pthread_barrier_wait(&barrier);
	//printf("\nAt 2nd Barrier id :: %d",id);
        barrierFun(id);// WAIT till all previous threads have completed executing the Red Blocks
        for(i=firstRow;i<=lastRow;i++)//Compute Black Blocks
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
            for(j=startCol;j<=matrixSize;j=j+2)
            {
                temp = orginalGrid[i][j];
                orginalGrid[i][j] = (orginalGrid[i-1][j]+orginalGrid[i+1][j]+orginalGrid[i][j-1]+orginalGrid[i][j+1]) * .25;
                if(iter == MAX_ITERATION)
                {
                    MAX_DIFF= max_value(MAX_DIFF,orginalGrid[i][j]-fabs(temp));
                }

            }
        }
               //pthread_barrier_wait(&barrier);
	//printf("\nAt exit Barrier id :: %d",id);
            max_diff_array[id] = MAX_DIFF;
              barrierFun(id);// WAIT till all previous threads have completed executing the Black Blocks
    }

}
void printGrid(double ** orginalGrid)
{
    int i=0,j=0;
    for(i=1;i<=matrixSize;i++)
    {
        for(j=1;j<=matrixSize;j++)
        {
            printf("%lf ",orginalGrid[i][j]);
        }
        printf("\n");
    }

}
int main(int argc,char *argv[])
{
    int i=0,j=0;
    int *p;
    pthread_t* threads;
    clock_t start_time,end_time;
    if(argc != 4 )
    {
        printf("Invalid Number Of Arguments , Please enter MatrixSize , NumberOfThread  MaxIterations");
        return 0;
    }
    matrixSize = atoi(argv[1]);
    numOfThread = atoi(argv[2]);
   // numberOfCores = atoi(argv[3]);
    MAX_ITERATION = atoi(argv[3]);
    printf("Red Black Seq Computation Pthread\n");

    barrierArray = initializeBarrier();
    max_diff_array =initialize_max_diff_array();
   /* if(pthread_barrier_init(&barrier, NULL, numOfThread))
    {
        printf("Could not create a barrier\n");
        return -1;
    }*/

    orginalGrid = allocateMatrix(matrixSize);

    for(i=0;i<matrixSize+2;i++)
    {
       // printf("\n");
        for(j=0;j<matrixSize+2;j++)
        {
            if(i == 0 || i == matrixSize+1 || j == 0 || j == matrixSize+1)
            {
                orginalGrid[i][j] = 1;
            }
            else
            {
                orginalGrid[i][j] = 0;
            }
           // printf("%lf  ",orginalGrid[i][j]);
        }
    }
    start_time = clock();
    // Allocation of PThread
    threads=(pthread_t*)malloc(numOfThread*sizeof(pthread_t));
    //creation of thread
    for(i=0;i<numOfThread;i++)
    {
        p = (int*)malloc(sizeof(int));
        *p=i;
        pthread_create(&threads[i],NULL,worker,(void*)(p));
    }
 //pthread_barrier_wait(&barrier);
    for(i=0;i<numOfThread;i++)
    {
        pthread_join(threads[i],NULL);
    }
    end_time = clock();
    MAX_DIFF=findLargestDiffInArray();
    double executionTime = (double)(end_time-start_time)/CLOCKS_PER_SEC;
    printf("********************** RESULTS ARE **********************\n");
    printf("Number of MPI RANK :: 0\n");
    printf("Number of threads :: %d\n",numOfThread);
    printf("ExecutionTime :: %lf \n",executionTime);
    printf("MAXIMUM Difference :: %lf \n",MAX_DIFF);
    //printf("matrixSize :: %d \n",matrixSize);
    //printf("MAX_ITERATION :: %d \n",MAX_ITERATION);

    //computeValue(orginalGrid);
    if(matrixSize < 10)
    {
        printf("\n RESULT GRID :: \n");
        printGrid(orginalGrid);
    }

    return 0;
}


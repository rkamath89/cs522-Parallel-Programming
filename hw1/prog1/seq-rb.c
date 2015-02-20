#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int matrixSize =0;
int MAX_ITERATION = 0;
double MAX_DIFF =0;

double max_value(double number1,double number2)
{
    return (fabs(number1)>fabs(number2) ?fabs(number1) : fabs(number2));
}
double** allocateMatrix()
{
    double **index;
    double *matrixBlock;
    int i=0;
    int sizeOfMatrix = matrixSize+2;

    matrixBlock = (double*)malloc( (sizeOfMatrix)*(sizeOfMatrix)*sizeof(double));
    index = (double**)malloc( (sizeOfMatrix)*sizeof(double*));
    for(i = 0 ; i < sizeOfMatrix;i++)
    {
        index[i]=&(matrixBlock[i*(sizeOfMatrix)]);
    }
    return index;

}
void computeValue(double ** orginalGrid)
{
    int i=0,iter=0,j=0,startCol=0;
    double temp;
    for(iter=0;iter<=MAX_ITERATION;iter++)
    {
        for(i=1;i<=matrixSize;i++)//Compute Red Blocks
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
        for(i=1;i<=matrixSize;i++)//Compute Black Blocks
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

    clock_t startTime,endTime;
    int i=0,j=0;

    if(argc != 3 )
    {
        printf("Please enter valid values as arguments , matrixSize and MaxIterations\n");
        return 0;
    }
    matrixSize = atoi(argv[1]);

    MAX_ITERATION = atoi(argv[2]);



    double **orginalGrid ;
    orginalGrid = allocateMatrix(matrixSize);

    for(i=0;i<matrixSize+2;i++)
    {
        if(matrixSize < 10)
            printf("\n");
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
            if(matrixSize < 10)
                printf("%lf  ",orginalGrid[i][j]);
        }
    }
    startTime = clock();
    computeValue(orginalGrid);
    printf("\n RESULT GRID :: \n");
    endTime = clock();

    double executionTime = (double)(endTime-startTime)/CLOCKS_PER_SEC;
    printf("\n********************** RESULTS ARE **********************\n");
    printf("\nNumber of MPI RANK :: 0\n");
    printf("Number of threads :: %d\n",);
    printf("ExecutionTime :: %lf \n",executionTime);
    printf("MAXIMUM Difference :: %lf \n",MAX_DIFF);

    printf("********************** RESULTS ARE **********************\n");
    printf("ExecutionTime :: %lf \n",executionTime);
    printf("matrixSize :: %d \n",matrixSize);
    printf("MAX_ITERATION :: %d \n",MAX_ITERATION);
    printf("MAX_DIFF :: %lf \n",MAX_DIFF);
    if(matrixSize < 10)
    {
        printGrid(orginalGrid);
    }
    return 0;
}

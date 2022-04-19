
#include <stdio.h>
#include <stdlib.h>

#define SHARED_SIZE 1024

#ifdef GKLEE_MODE
  #define THREADS 32
  #define BLOCKS 1
#else
  #define THREADS 1024
  #define BLOCKS 1024
  #include "CUDATiming.h"
  #include "Probabilities.h"
#endif


__global__ static void convolution(unsigned int* values, unsigned int *filter, unsigned int* result, int n, int range){

    __shared__ int sharedFilter[SHARED_SIZE];
    const int tid = blockIdx.x * blockDim.x + threadIdx.x;

    int pos = threadIdx.x;
    while (pos < SHARED_SIZE){
      sharedFilter[pos] = filter[pos];
      pos += blockDim.x;
    }

    __syncthreads();

      int sum = 0;
      for (int j = 0; j < range; j++ ){
        for (int i = 0 ; i < range; i++){
          sum += values[(tid + i) % n] * sharedFilter[values[(tid + i) % n] % SHARED_SIZE];
        }
        for (int i = 0 ; i < range; i++){
          sum += values[(n + tid - i) % n] * sharedFilter[values[(n + tid - i) % n] % SHARED_SIZE];
        }
      }

      result[tid] = sum;
}



int main(int argc, char** argv){
   // int BLOCKS = 512;
   // int THREADS = 512;
    int total = THREADS * BLOCKS;
    int range = 7;
    unsigned int *values;
    unsigned int *result;
    unsigned int *filter;

    values = (unsigned int*)malloc(sizeof(unsigned int) * total);
    result = (unsigned int*)malloc(sizeof(unsigned int) * total);
    filter = (unsigned int*)malloc(sizeof(unsigned int) * SHARED_SIZE);

  #ifdef GKLEE_MODE
      printf("WE ARE IN THE SYMBOLIC PART\n");
      klee_make_symbolic(values, sizeof(unsigned int) * total, "values");
      // for (int i = 0 ; i < total; i++){
      //     klee_assume(values[i] >= 0);
      //     klee_assume(values[i] < SHARED_SIZE);
      // }
  #else
    printf("WE ARE IN THE NORMAL PART\n");
    unsigned int fixedInput[32] = {992, 256, 704, 608, 768, 320, 800, 576, 896, 352,
      864, 416, 928, 448, 672, 480, 544, 160, 512, 224, 128, 192, 96, 64, 32, 0, 640,
      384, 832, 736, 288, 960};

      for (int i = 0; i < total; i++){
        values[i] = fixedInput[i % 32];
      }
  #endif

    for (int i = 0; i < SHARED_SIZE; i++){
        filter[i] = i;
    }

    unsigned int * dvalues, *dfilter, *dresult;
    cudaMalloc((void**)&dvalues, sizeof(unsigned int) * total);
    cudaMalloc((void**)&dresult, sizeof(unsigned int) * total);
    cudaMalloc((void**)&dfilter, sizeof(unsigned int) * SHARED_SIZE);
    cudaMemcpy(dvalues, values, sizeof(unsigned int) * total, cudaMemcpyHostToDevice);
    cudaMemcpy(dfilter, filter, sizeof(unsigned int) * SHARED_SIZE, cudaMemcpyHostToDevice);

    #ifndef GKLEE_MODE
      float maxTime = 0, minTime = 10000000;
      std::vector<float> times;
      CUDATiming cudaTiming;
      cudaTiming.startMeasuring();
      int iterations = 100;//1000;
      for (int i =0 ; i < iterations; i++){
        CUDATiming internal;
        internal.startMeasuring();
    #endif

	     convolution<<<BLOCKS, THREADS>>>(dvalues, dfilter, dresult, total, range);

       cudaDeviceSynchronize();

   #ifndef GKLEE_MODE
       internal.stopMeasuring();
       float current = internal.getTime();
       times.push_back(current);
       if (current < minTime){
         minTime = current;
       }
       if (current > maxTime){
         maxTime = current;
       }
     }
     cudaTiming.stopMeasuring();

     printf("Minimum execution time : %f\n", minTime);
     printf("Maximum execution time : %f\n", maxTime);
     float time = cudaTiming.getTime();
     printf("Execution avg of kernel : %f ms for %d iterations\n", time/iterations, iterations);

     float avgIRQ = getAverageIRQ<float>(times);
     printf("Average execution time with quartiles: %f\n", avgIRQ);
     #endif


    cudaMemcpy(result, dresult, sizeof(int) * total, cudaMemcpyDeviceToHost);

    cudaFree(dvalues);
    cudaFree(dresult);
    cudaFree(dfilter);

    free(values);
    free(result);
    free(filter);

}

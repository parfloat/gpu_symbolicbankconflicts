/*
 * Copyright 1993-2015 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

// // CUDA Runtime
// #include <cuda_runtime.h>
//
// // Utility and system includes
// #include <helper_cuda.h>
// #include <helper_functions.h>  // helper for shared that are common to CUDA Samples

// project include
#include "histogram_common.h"
#include "histogram256.h"
#include "histogram_gold.h"

////////////////////////////////////////////////////////////////////////////////
// Host interface to GPU histogram
////////////////////////////////////////////////////////////////////////////////
//histogram256kernel() intermediate results buffer
static const uint PARTIAL_HISTOGRAM256_COUNT = 1;//240;
static uint *d_PartialHistograms;

//Internal memory allocation
void initHistogram256(void)
{
    checkCudaErrors(cudaMalloc((void **)&d_PartialHistograms, PARTIAL_HISTOGRAM256_COUNT * HISTOGRAM256_BIN_COUNT * sizeof(uint)));
}

//Internal memory deallocation
void closeHistogram256(void)
{
    checkCudaErrors(cudaFree(d_PartialHistograms));
}

void histogram256(
    uint *d_Histogram,
    void *d_Data,
    uint byteCount
)
{
    // assert(byteCount % sizeof(uint) == 0);
    histogram256Kernel<<<PARTIAL_HISTOGRAM256_COUNT, HISTOGRAM256_THREADBLOCK_SIZE>>>(
        d_PartialHistograms,
        (uint *)d_Data,
        byteCount / sizeof(uint)
    );
    // getLastCudaError("histogram256Kernel() execution failed\n");

  #ifndef GKLEE_MODE
      mergeHistogram256Kernel<<<HISTOGRAM256_BIN_COUNT, MERGE_THREADBLOCK_SIZE>>>(
          d_Histogram,
          d_PartialHistograms,
          PARTIAL_HISTOGRAM256_COUNT
      );
      // getLastCudaError("mergeHistogram256Kernel() execution failed\n");
  #endif
}

#ifndef GKLEE_MODE
  #include "CUDATiming.h"
  #include "Probabilities.h"
#endif

const int numRuns = 16;
const static char *sSDKsample = "[histogram]\0";

int main(int argc, char **argv)
{
    uchar *h_Data;
    uint  *h_HistogramCPU, *h_HistogramGPU;
    uchar *d_Data;
    uint  *d_Histogram;
    // StopWatchInterface *hTimer = NULL;
    int PassFailFlag = 1;
    #ifdef GKLEE_MODE
    uint byteCount = 32;//64 * 1048576;//64 * 1048576;//256*4;//32*4;//64*1024;//64 * 1048576;
    #else
    uint byteCount = 1048576;//32 * 1048576;
    #endif
    uint uiSizeMult = 1;

    // cudaDeviceProp deviceProp;
    // deviceProp.major = 0;
    // deviceProp.minor = 0;

    // set logfile name and start logs
    printf("[%s] - Starting...\n", sSDKsample);

    //Use command-line specified CUDA device, otherwise use device with highest Gflops/s
    // int dev = findCudaDevice(argc, (const char **)argv);

    // checkCudaErrors(cudaGetDeviceProperties(&deviceProp, dev));

    // printf("CUDA device [%s] has %d Multi-Processors, Compute %d.%d\n",
    //        deviceProp.name, deviceProp.multiProcessorCount, deviceProp.major, deviceProp.minor);
    //
    // sdkCreateTimer(&hTimer);

    // Optional Command-line multiplier to increase size of array to histogram
    // if (1)//(checkCmdLineFlag(argc, (const char **)argv, "sizemult"))
    // {
    //     uiSizeMult = 0;//getCmdLineArgumentInt(argc, (const char **)argv, "sizemult");
    //     uiSizeMult = 1;//MAX(1,MIN(uiSizeMult, 10));
    //     byteCount *= uiSizeMult;
    // }

    printf("Initializing data...\n");
    printf("...allocating CPU memory.\n");
    h_Data         = (uchar *)malloc(byteCount);
    h_HistogramCPU = (uint *)malloc(HISTOGRAM256_BIN_COUNT * sizeof(uint));
    h_HistogramGPU = (uint *)malloc(HISTOGRAM256_BIN_COUNT * sizeof(uint));

    #ifdef GKLEE_MODE
        printf("WE ARE IN THE SYMBOLIC VERSION");
        klee_make_symbolic(h_Data, byteCount, "adi_SYM_input");
    #else
      uint fixedInput[] = {192, 192, 192, 192, 224, 224, 224, 224, 160, 160, 160,
        96, 128, 96, 96, 160, 96, 64, 64, 0, 64, 32, 32, 64, 32, 0, 0, 32, 0, 128, 128, 128};
      printf("...generating input data\n");
      // srand(2009);
      srand(time(NULL));

      for (uint i = 0; i < byteCount; i++)
      {
          // h_Data[i] = 0;//rand() % 256;//0;//(i * 32) % 256;//rand() % 256;
          h_Data[i] = fixedInput[i % 32];
          // h_Data[i] = rand() % 256;
      }
    #endif

    printf("...allocating GPU memory and copying input data\n\n");
    checkCudaErrors(cudaMalloc((void **)&d_Data, byteCount));
    checkCudaErrors(cudaMalloc((void **)&d_Histogram, HISTOGRAM256_BIN_COUNT * sizeof(uint)));
    checkCudaErrors(cudaMemcpy(d_Data, h_Data, byteCount, cudaMemcpyHostToDevice));

    {
        printf("Initializing 256-bin histogram...\n");
        initHistogram256();

        printf("Running 256-bin GPU histogram for %u bytes (%u runs)...\n\n", byteCount, numRuns);

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

            histogram256(d_Histogram, d_Data, byteCount);
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


          printf("\nValidating GPU results...\n");
          printf(" ...reading back GPU results\n");
          checkCudaErrors(cudaMemcpy(h_HistogramGPU, d_Histogram, HISTOGRAM256_BIN_COUNT * sizeof(uint), cudaMemcpyDeviceToHost));

          printf(" ...histogram256CPU()\n");
          histogram256CPU(
              h_HistogramCPU,
              h_Data,
              byteCount
          );

          printf(" ...comparing the results\n");

          for (uint i = 0; i < HISTOGRAM256_BIN_COUNT; i++)
              if (h_HistogramGPU[i] != h_HistogramCPU[i])
              {
                  PassFailFlag = 0;
              }

          printf(PassFailFlag ? " ...256-bin histograms match\n\n" : " ***256-bin histograms do not match!!!***\n\n");
        #endif

        printf("Shutting down 256-bin histogram...\n\n\n");
        closeHistogram256();
    }

    printf("Shutting down...\n");
    // sdkDeleteTimer(&hTimer);
    checkCudaErrors(cudaFree(d_Histogram));
    checkCudaErrors(cudaFree(d_Data));
    free(h_HistogramGPU);
    free(h_HistogramCPU);
    free(h_Data);

    // cudaDeviceReset causes the driver to clean up all state. While
    // not mandatory in normal operation, it is good practice.  It is also
    // needed to ensure correct operation when the application is being
    // profiled. Calling cudaDeviceReset causes all profile data to be
    // flushed before the application exits
    cudaDeviceReset();

    printf("%s - Test Summary\n", sSDKsample);

    // pass or fail (for both 64 bit and 256 bit histograms)
    if (!PassFailFlag)
    {
        printf("Test failed!\n");
        exit(EXIT_FAILURE);
    }

    printf("Test passed\n");
    exit(EXIT_SUCCESS);
}


#include <stdio.h>

#ifndef GKLEE_MODE
#include "CUDATiming.h"
#include "ImageHandling.h"
#include "Probabilities.h"
#include <vector>
#include <fstream>
#include <iostream>
#endif

#define SHARED_ARR_SZ  (37 * 32)//256//1024//256//32//256//(32 * 32)//256

#ifdef GKLEE_MODE
#define THREADS 32
#define BLOCKS 1
#else
#define THREADS (32 * 32)
#define BLOCKS (1024)
#endif

#define memType int

__global__ void heatmapKernel(int* input,
			      int* filterRed, int* filterGreen, int* filterBlue,
			      int* outputRed, int* outputGreen, int* outputBlue,
			      long int totalThreads){
  __shared__ memType sharedFilterRed[SHARED_ARR_SZ];
  __shared__ memType sharedFilterGreen[SHARED_ARR_SZ];
  __shared__ memType sharedFilterBlue[SHARED_ARR_SZ];
  int gid = gridDim.x * blockIdx.x + threadIdx.x;
  int pos = threadIdx.x;

  while (pos < SHARED_ARR_SZ){
    sharedFilterRed[pos] = filterRed[pos];
    sharedFilterGreen[pos] = filterGreen[pos];
    sharedFilterBlue[pos] = filterBlue[pos];
    pos += blockDim.x;
  }

  __syncthreads();

  while (gid < totalThreads){
    outputRed[gid]= sharedFilterRed[input[gid] % SHARED_ARR_SZ];
    outputGreen[gid]= sharedFilterGreen[input[gid] % SHARED_ARR_SZ];
    outputBlue[gid]= sharedFilterBlue[input[gid] % SHARED_ARR_SZ];

    outputRed[gid]+= sharedFilterRed[(input[gid] + 1) % SHARED_ARR_SZ];
    outputGreen[gid]+= sharedFilterGreen[(input[gid]+1) % SHARED_ARR_SZ];
    outputBlue[gid]+= sharedFilterBlue[(input[gid] + 1) % SHARED_ARR_SZ];

    outputRed[gid]+= sharedFilterRed[(input[gid] + 2) % SHARED_ARR_SZ];
    outputGreen[gid]+= sharedFilterGreen[(input[gid]+2) % SHARED_ARR_SZ];
    outputBlue[gid]+= sharedFilterBlue[(input[gid] + 2) % SHARED_ARR_SZ];

    outputRed[gid] /= 3;
    outputGreen[gid] /= 3;
    outputBlue[gid] /= 3;

    gid += blockDim.x * gridDim.x;
  }
}


int main(int argc, char **argv){
  int sh = SHARED_ARR_SZ;
  printf("SHARED_ARR_SZ = %d\n", sh);

  memType *input, *filterRed, *filterGreen, *filterBlue, *outputRed, *outputGreen, *outputBlue;
  memType *gpu_input, *gpu_filterRed, *gpu_filterGreen, *gpu_filterBlue, *gpu_outputRed, *gpu_outputGreen, *gpu_outputBlue;

  long int n = BLOCKS * THREADS;
	printf("Threads = %ld\n", n);

  input = (memType*)malloc(n * sizeof(memType));
  filterRed = (memType*)malloc(SHARED_ARR_SZ * sizeof(memType));
  filterGreen = (memType*)malloc(SHARED_ARR_SZ * sizeof(memType));
  filterBlue = (memType*)malloc(SHARED_ARR_SZ * sizeof(memType));
  outputRed = (memType*)malloc(n * sizeof(memType));
  outputGreen = (memType*)malloc(n * sizeof(memType));
  outputBlue = (memType*)malloc(n * sizeof(memType));

//SHARED_ARR_SZ = (32 * 32)
//inputs obtained by GKLEE analysis
#if SHARED_ARR_SZ == (37 * 32)
  memType maximum[] =
    {832, 160, 576, 1120, 352, 864, 224, 608, 736, 288, 64, 896, 640, 800, 128, 480, 448, 544, 1152, 672, 32, 992, 704, 1056, 96, 1024, 1088, 512, 192, 768, 256, 0};
#elif SHARED_ARR_SZ == (36 * 32)
  memType maximum[] =
    {352, 416, 448, 320, 672, 864, 160, 1120, 1088, 288, 256, 96, 576, 800, 480, 928, 640, 544, 736, 896, 32, 128, 192, 1056, 832, 1024, 224, 512, 64, 768, 384, 0};
#elif SHARED_ARR_SZ == (35 * 32)
  memType maximum[] =
    {848, 112, 592, 336, 400, 880, 240, 624, 688, 304, 176, 496, 752, 816, 1072, 944, 368, 560, 1104, 432, 48, 656, 464, 1040, 144, 528, 208, 720, 80, 784, 272, 16};
#elif SHARED_ARR_SZ == (34 * 32)
  memType maximum[] =
    {895, 511, 671, 479, 127, 639, 191, 735, 863, 319, 1023, 383, 831, 799, 223, 351, 287, 575, 159, 607, 63, 703, 991, 1087, 255, 1055, 447, 927, 95, 543, 415, 31};
#elif SHARED_ARR_SZ == (33 * 32)
  memType maximum[] =
    {336, 976, 496, 368, 1008, 240, 432, 656, 848, 176, 400, 688, 304, 144, 912, 720, 816, 624, 880, 208, 560, 752, 592, 48, 112, 1040, 80, 944, 464, 528, 784, 272};
#elif SHARED_ARR_SZ == (32 * 32)
  memType maximum[] =
    {832, 640, 320, 384, 480, 864, 960, 416, 352, 576, 160, 992, 704, 608, 64, 928, 32, 192, 672, 288, 736, 448, 544, 96, 896, 800, 224, 512, 128, 768, 256, 0};
#elif SHARED_ARR_SZ == (31 * 32)
  memType maximum[] =
    {638, 222, 382, 94, 446, 926, 862, 414, 958, 990, 286, 126, 606, 894, 190, 734, 478, 574, 830, 158, 798, 702, 542, 30, 766, 350, 62, 670, 318, 254, 510, 24};
#elif SHARED_ARR_SZ == (30 * 32)
  memType maximum[] =
    {190, 382, 126, 286, 574, 350, 798, 638, 318, 94, 158, 894, 414, 510, 702, 606, 830, 222, 670, 62, 766, 734, 542, 862, 30, 958, 446, 478, 254, 926, 24, 562};
#elif SHARED_ARR_SZ == (29 * 32)
      memType maximum[] =
        {414, 222, 894, 254, 766, 446, 190, 926, 94, 158, 30, 574, 126, 542, 286, 382, 350, 830, 734, 478, 638, 510, 62, 670, 702, 606, 798, 318, 862, 126, 527, 25};
#elif SHARED_ARR_SZ == (28 * 32)
  memType maximum[] =
    {30, 382, 734, 862, 190, 798, 350, 62, 414, 702, 158, 542, 670, 606, 286, 894, 830, 766, 222, 478, 94, 254, 510, 318, 126, 446, 638, 574, 56, 24, 56, 20};
#elif SHARED_ARR_SZ == (27 * 32)
  memType maximum[] =
    {95, 799, 831, 863, 511, 287, 767, 319, 447, 607, 63, 479, 415, 383, 575, 255, 703, 223, 159, 127, 543, 735, 191, 351, 671, 31, 639, 0, 0, 6, 15, 863};
#elif SHARED_ARR_SZ == (26 * 32)
  memType maximum[] =
    {383, 159, 735, 319, 831, 703, 31, 63, 127, 671, 415, 287, 479, 447, 639, 223, 95, 543, 575, 767, 255, 351, 799, 607, 191, 511, 30, 0, 0, 22, 30, 30};
#elif SHARED_ARR_SZ == (25 * 32)
  memType maximum[] =
    {625, 529, 497, 401, 241, 113, 81, 433, 369, 273, 689, 49, 145, 657, 721, 305, 561, 209, 785, 337, 177, 753, 593, 465, 17, 16, 16, 16, 16, 54, 31, 111};
#elif SHARED_ARR_SZ == (24 * 32)
  memType maximum[] =
    {121, 729, 89, 601, 345, 761, 633, 537, 313, 409, 377, 441, 57, 473, 569, 153, 217, 281, 697, 185, 665, 249, 505, 25, 24, 280, 0, 263, 8, 529, 16, 14};
#elif SHARED_ARR_SZ == (23 * 32)
  memType maximum[] =
    {161, 65, 289, 321, 193, 417, 705, 385, 449, 641, 609, 33, 129, 673, 513, 577, 545, 257, 353, 225, 481, 97, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (22 * 32)
  memType maximum[] =
    {239, 79, 655, 431, 143, 175, 591, 303, 623, 47, 111, 367, 335, 559, 527, 271, 399, 463, 495, 207, 687, 15, 0, 16, 16, 16, 16, 16, 0, 536, 8, 0};
#elif SHARED_ARR_SZ == (21 * 32)
  memType maximum[] =
    {65, 321, 449, 225, 353, 417, 385, 289, 161, 33, 641, 577, 545, 257, 1, 129, 609, 193, 481, 97, 513, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (20 * 32)
  memType maximum[] =
    {194, 578, 226, 98, 322, 130, 290, 450, 34, 514, 386, 546, 354, 162, 258, 610, 418, 66, 2, 482, 190, 1, 1, 256, 35, 16, 127, 0, 35, 19, 5, 35};
#elif SHARED_ARR_SZ == (19 * 32)
  memType maximum[] =
    {253, 413, 573, 541, 509, 189, 29, 125, 157, 93, 221, 317, 381, 285, 605, 61, 477, 445, 349, 118, 24, 240, 20, 240, 4, 42, 4, 255, 290, 520, 520, 384};
#elif SHARED_ARR_SZ == (18 * 32)
  memType maximum[] =
    {65, 449, 289, 545, 385, 129, 225, 193, 353, 513, 417, 33, 1, 481, 321, 97, 161, 257, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (17 * 32)
  memType maximum[] =
    {73, 41, 9, 105, 361, 137, 457, 521, 425, 489, 201, 297, 233, 393, 329, 169, 265, 0, 8, 0, 0, 0, 0, 7, 8, 0, 259, 0, 8, 259, 0, 0};
#elif SHARED_ARR_SZ == (16 * 32)
  memType maximum[] =
    {353, 417, 161, 65, 1, 449, 97, 385, 289, 129, 33, 481, 193, 225, 321, 257, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (15 * 32)
  memType maximum[] =
    {33, 449, 385, 353, 1, 321, 97, 161, 289, 225, 65, 417, 129, 193, 257, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (14 * 32)
  memType maximum[] =
    {97, 321, 33, 65, 129, 225, 385, 289, 257, 193, 353, 417, 161, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (13 * 32)
  memType maximum[] =
    {415, 319, 95, 383, 63, 351, 287, 255, 127, 159, 31, 223, 191, 0, 133, 62, 0, 2, 415, 0, 54, 48, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (12 * 32)
  memType maximum[] =
    {65, 353, 33, 193, 289, 129, 321, 225, 1, 161, 97, 257, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (11 * 32)
  memType maximum[] =
    {161, 193, 65, 33, 289, 257, 225, 97, 321, 129, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (10 * 32)
  memType maximum[] =
    {193, 33, 257, 65, 225, 97, 129, 161, 289, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (9 * 32)
  memType maximum[] =
    {65, 193, 161, 257, 97, 33, 225, 129, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (8 * 32)
  memType maximum[] =
    {97, 65, 225, 33, 161, 193, 129, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (7 * 32)
  memType maximum[] =
    {65, 97, 33, 161, 129, 193, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (6 * 32)
  memType maximum[] =
    {33, 161, 65, 129, 97, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (5 * 32)
  memType maximum[] =
    {65, 97, 129, 33, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (4 * 32)
  memType maximum[] =
    {65, 33, 97, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (3 * 32)
  memType maximum[] =
    {36, 68, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == (2 * 32)
  memType maximum[] =
    {33, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif SHARED_ARR_SZ == 32
  memType maximum[] =
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#else
  memType maximum[SHARED_ARR_SZ];
#endif



  memType minimum[] =
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};





#ifndef GKLEE_MODE
  for (long int i = 0; i < n; i++){
    if (argc > 1){
      if (argv[1][0] == 'M'){
        // printf("Working with maximum\n");
        input[i] = maximum[i % 32];//i % SHARED_ARR_SZ;
      }
      if (argv[1][0] == 'm'){
        // printf("Working with minimum\n");
        input[i] = minimum[i % 32];
      }
    }else{
      input[i] = minimum[i % 32];
    }
  }
#endif


#ifdef GKLEE_MODE
  klee_make_symbolic(input, n * sizeof(memType), "adi_SYM_input");
  for (long int i = 0 ; i < n; i++){
    klee_assume(input[i] >= 0);
    klee_assume(input[i] < SHARED_ARR_SZ);
      //klee_assume(input[i] == maximum[i]);
  }
#endif

  for (int i = 0 ; i < SHARED_ARR_SZ; i++){
    filterRed[i] = 256 - i/4;//i % SHARED_ARR_SZ;//i / 4;
    filterGreen[i] = 256 - i/4;//i % SHARED_ARR_SZ;
    filterBlue[i] = 256 - i%256;//i % SHARED_ARR_SZ;//i/4;
  }

  cudaMalloc(&gpu_input, n * sizeof(memType));
  cudaMalloc(&gpu_filterRed, SHARED_ARR_SZ * sizeof(memType));
  cudaMalloc(&gpu_filterGreen, SHARED_ARR_SZ * sizeof(memType));
  cudaMalloc(&gpu_filterBlue, SHARED_ARR_SZ * sizeof(memType));
  cudaMalloc(&gpu_outputRed, n * sizeof(memType));
  cudaMalloc(&gpu_outputGreen, n * sizeof(memType));
  cudaMalloc(&gpu_outputBlue, n * sizeof(memType));


  cudaMemcpy(gpu_input, input, n * sizeof(memType), cudaMemcpyHostToDevice);
  cudaMemcpy(gpu_filterRed, filterRed, SHARED_ARR_SZ * sizeof(memType), cudaMemcpyHostToDevice);
  cudaMemcpy(gpu_filterGreen, filterGreen, SHARED_ARR_SZ * sizeof(memType), cudaMemcpyHostToDevice);
  cudaMemcpy(gpu_filterBlue, filterBlue, SHARED_ARR_SZ * sizeof(memType), cudaMemcpyHostToDevice);

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

  heatmapKernel<<<BLOCKS, THREADS>>>(gpu_input, gpu_filterRed, gpu_filterGreen, gpu_filterBlue, gpu_outputRed, gpu_outputGreen, gpu_outputBlue, n);

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

	// std::ofstream outFile;
	// outFile.open("times.dat");
	//
	// for (float t : times){
	// 	outFile << t << "\n";
	// }
	// outFile.close();

	// auto timesClean(times);
	// auto quartiles = quantile<float>(timesClean, { 0.25, 0.5, 0.75 });
	// auto outData = removeOutliers<float>(times, quartiles);
	// // outFile.open("timesClean.dat");
	// // for (float t : outData){
	// // 	// std::cout << t << "\n";
	// // 	outFile << t << "\n";
	// // }
	// // outFile.close();
	// float avg = getAverage<float>(outData);
	// printf("Average execution time with quartiles: %f\n", avg);
	float avgIRQ = getAverageIRQ<float>(times);
	printf("Average execution time with quartiles: %f\n", avgIRQ);
#endif

  cudaMemcpy(outputRed, gpu_outputRed, n * sizeof(int), cudaMemcpyDeviceToHost);
  cudaMemcpy(outputGreen, gpu_outputGreen, n * sizeof(int), cudaMemcpyDeviceToHost);
  cudaMemcpy(outputBlue, gpu_outputBlue, n * sizeof(int), cudaMemcpyDeviceToHost);

#ifndef GKLEE_MODE
  // for (int i = 0 ; i < n ; i++){
  //   printf("%d ", output[i]);
  // }
  // printf("\n");


  char name[] = "outputImage.bmp";
  // writeIMG(name, outputRed, outputGreen, outputBlue, THREADS, n / THREADS, n);
#endif

  cudaFree(gpu_input);
  cudaFree(gpu_filterRed);
  cudaFree(gpu_filterGreen);
  cudaFree(gpu_filterBlue);
  cudaFree(gpu_outputRed);
  cudaFree(gpu_outputGreen);
  cudaFree(gpu_outputBlue);
  free(input);
  free(filterRed);
  free(filterGreen);
  free(filterBlue);
  free(outputRed);
  free(outputGreen);
  free(outputBlue);
  return 0;
}

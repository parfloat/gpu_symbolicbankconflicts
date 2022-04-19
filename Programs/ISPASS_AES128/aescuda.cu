
/***************************************************************************
 *   Copyright (C) 2006                                                    *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


/**
	@author Svetlin Manavski <svetlin@manavski.com>
 */

#define BSIZE 4

//#include <iostream>
#include "aesCudaUtils.h"
//#include "aesHost.h"
#include "utilsBox.h"
#include <string.h>


// includes, system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// includes, project
#include "cutil.h"

#include "sbox_E.h"
#include "sbox_D.h"
#include "aesEncrypt128_kernel.h"
#include "aesDecrypt128_kernel.h"
#include "aesEncrypt256_kernel.h"
#include "aesDecrypt256_kernel.h"


//extern "C" void aesEncryptHandler128(unsigned *d_Result, unsigned *d_Input, int inputSize) {
void aesEncryptHandler128(unsigned *d_Result, unsigned *d_Input, int inputSize, unsigned  *d_Key) {

  dim3  threads(BSIZE, 1); //
  // dim3  grid((inputSize/BSIZE)/4, 1); //bsize=4 --> (inputsize=1024)/16 = 2^(10-4)= 64 blocks.
  // if (grid.x == 0){
  //   grid.x = 1;
  // }
  dim3  grid(1, 1);

    aesEncrypt128<<< grid, threads >>>( d_Result, d_Input, inputSize, d_Key);
    CUDA_SAFE_CALL( cudaThreadSynchronize() );
}

void aesDecryptHandler128(unsigned *d_Result, unsigned *d_Input, int inputSize, unsigned *d_Key) {

	dim3  threads(BSIZE, 1);
    // dim3  grid((inputSize/BSIZE)/4, 1);
    //
    // if (grid.x == 0){
    //   grid.x = 1;
    // }
    dim3  grid(1, 1);

    aesDecrypt128<<< grid, threads >>>( d_Result, d_Input, inputSize, d_Key);
    CUDA_SAFE_CALL( cudaThreadSynchronize() );
}

void aesEncryptHandler256(unsigned *d_Result, unsigned *d_Input, int inputSize, unsigned *d_Key) {

	dim3  threads(BSIZE, 1);
    // dim3  grid((inputSize/BSIZE)/4, 1);
    //
    //
    // if (grid.x == 0){
    //   grid.x = 1;
    // }
    dim3  grid(1, 1);
    aesEncrypt256<<< grid, threads >>>( d_Result, d_Input, inputSize, d_Key);
    CUDA_SAFE_CALL( cudaThreadSynchronize() );
}

void aesDecryptHandler256(unsigned *d_Result, unsigned *d_Input, int inputSize, unsigned *d_Key) {

	dim3  threads(BSIZE, 1);
    // dim3  grid((inputSize/BSIZE)/4, 1);
    //
    //
    // if (grid.x == 0){
    //   grid.x = 1;
    // }
    dim3  grid(1, 1);
    aesDecrypt256<<< grid, threads >>>( d_Result, d_Input, inputSize, d_Key);
    CUDA_SAFE_CALL( cudaThreadSynchronize() );
}


int aesHost(unsigned char* result, const unsigned char* inData, int inputSize, const unsigned char* key, int keySize, bool toEncrypt)
{
/*	if (inputSize < 256)
		return -1;
	if (inputSize % 256 > 0)
		return -11;
	if (keySize != 240 && keySize != 176)
		return -2;
	if (!result || !inData || !key)
		return -3;/**/

    // int deviceCount;
    // CUDA_SAFE_CALL_NO_SYNC(cudaGetDeviceCount(&deviceCount));
    // if (deviceCount == 0) {
    //     fprintf(stderr, "There is no device.\n");
    //     exit(EXIT_FAILURE);
    // }
    // int dev;
    // for (dev = 0; dev < deviceCount; ++dev) {
    //     cudaDeviceProp deviceProp;
    //     CUDA_SAFE_CALL_NO_SYNC(cudaGetDeviceProperties(&deviceProp, dev));
    //     if (deviceProp.major >= 1)
    //         break;
    // }
    // if (dev == deviceCount) {
    //     fprintf(stderr, "There is no device supporting CUDA.\n");
    //     exit(EXIT_FAILURE);
    // }
    // else
    //     CUDA_SAFE_CALL(cudaSetDevice(dev));


    // allocate device memory
    unsigned * d_Input;
    CUDA_SAFE_CALL( cudaMalloc((void**) &d_Input, inputSize) );

	// the size of the memory for the key must be equal to keySize (every thread copies one key byte to shared memory)
    unsigned * d_Key;
    CUDA_SAFE_CALL( cudaMalloc((void**) &d_Key, keySize) );

/*	unsigned int ext_timer = 0;
    CUT_SAFE_CALL(cutCreateTimer(&ext_timer));
    CUT_SAFE_CALL(cutStartTimer(ext_timer));
*/
    // copy host memory to device
    printf("h Input size = %d\n", inputSize);
    printf("h Key size = %d\n", keySize);
    CUDA_SAFE_CALL( cudaMemcpy(d_Input, inData, inputSize, cudaMemcpyHostToDevice) );
    CUDA_SAFE_CALL( cudaMemcpy(d_Key, key, keySize, cudaMemcpyHostToDevice) );

	// //texture
	// cudaChannelFormatDesc chDesc;
	// chDesc.x = 32;
	// chDesc.y = 0;
	// chDesc.z = 0;
	// chDesc.w = 0;
	// chDesc.f = cudaChannelFormatKindUnsigned;
	// texEKey.normalized = false;
	// texDKey.normalized = false;
	// texEKey128.normalized = false;
	// texDKey128.normalized = false;

	// CUDA_SAFE_CALL( cudaBindTexture( 0, &texEKey128, d_Key, &chDesc, (size_t)keySize) );
	// CUDA_SAFE_CALL( cudaBindTexture( 0, &texDKey128, d_Key, &chDesc, (size_t)keySize) );
	// CUDA_SAFE_CALL( cudaBindTexture( 0, &texEKey, d_Key, &chDesc, (size_t)keySize) );
	// CUDA_SAFE_CALL( cudaBindTexture( 0, &texDKey, d_Key, &chDesc, (size_t)keySize) );

    // allocate device memory for result
    unsigned int size_Result = inputSize;
    unsigned * d_Result;
    CUDA_SAFE_CALL( cudaMalloc((void**) &d_Result, size_Result) );
	CUDA_SAFE_CALL( cudaMemset(d_Result, 0, size_Result) );


/*	unsigned int int_timer = 0;
    CUT_SAFE_CALL(cutCreateTimer(&int_timer));
    CUT_SAFE_CALL(cutStartTimer(int_timer));
*/
	if (!toEncrypt) {
		printf("\nDECRYPTION.....\n\n");
		if (keySize != 240)
		  aesDecryptHandler128( d_Result, d_Input, inputSize, d_Key);
		else
		  aesDecryptHandler256( d_Result, d_Input, inputSize, d_Key);
	} else {
		printf("\nENCRYPTION.....\n\n");
		if (keySize != 240)
		  aesEncryptHandler128( d_Result, d_Input, inputSize, d_Key);
		else
		  aesEncryptHandler256( d_Result, d_Input, inputSize, d_Key);
	}

/*	CUT_SAFE_CALL(cutStopTimer(int_timer));
    printf("GPU processing time: %f (ms)\n", cutGetTimerValue(int_timer));
    CUT_SAFE_CALL(cutDeleteTimer(int_timer));*/

    // check if kernel execution generated and error
    CUT_CHECK_ERROR("Kernel execution failed");

    // copy result from device to host
    CUDA_SAFE_CALL(cudaMemcpy(result, d_Result, size_Result, cudaMemcpyDeviceToHost) );

  /*  CUT_SAFE_CALL(cutStopTimer(ext_timer));
    printf("Total processing time: %f (ms)\n\n", cutGetTimerValue(ext_timer));
    CUT_SAFE_CALL(cutDeleteTimer(ext_timer));*/

    // cleanup memory
    CUDA_SAFE_CALL(cudaFree(d_Input));
    CUDA_SAFE_CALL(cudaFree(d_Key));
    CUDA_SAFE_CALL(cudaFree(d_Result));

    return 0;
}

//using namespace std;

//const int INPUTSIZE = 33*1024*1024;
const int INPUTSIZE = 1024;
bool MODE = 1;
//extern unsigned MODE;


//handler
//extern "C"
//int aesHost(unsigned char* result, const unsigned char* inData, int inputSize, const unsigned char* key, int keySize, bool toEncrypt);


int main(int argc, char *argv[])
{
    unsigned numPairs = commandLineManager(argc, argv, MODE);

    unsigned aesType = static_cast<unsigned>(atoi(argv[2]));

    /*if (argv[2][0] == '1' && argv[2][1] == '2' && argv[2][2] == '8'){
        aesType = 128;
    }else{
        aesType = 256;
    }*/

    unsigned ekSize=240;
    if (aesType != 256)
            ekSize = 176;

    //per ogni coppia di file di input e di chiave svolge l'operazione
    for (unsigned cnt=0; cnt<numPairs; ++cnt) {

            unsigned char *h_Input = new unsigned char[INPUTSIZE];
//             if ( !h_Input ){
//                     printf("cannot allocate memory for the input\n");
//                     exit(-1);
//                     }
//             memset(h_Input, 0, INPUTSIZE);
            for (int i = 0 ; i < INPUTSIZE; i++){
                h_Input[i] =  0;
            }

            unsigned char myExpKey[ekSize];
//             memset(myExpKey, 0, ekSize);
            for (int i = 0 ; i < ekSize; i++){
                myExpKey[i] =  0;
            }

            //funzione di inizializzazione che legge i file di input ed espande le chiavi.
            unsigned usefulData = initAesCuda(argv[(cnt*2)+4], myExpKey, aesType, argv[(cnt*2)+3], h_Input, INPUTSIZE, MODE);

            // memory for the result
            unsigned char *h_Result = new unsigned char[INPUTSIZE];
//             if ( !h_Result ){
//                     printf("cannot allocate memory for the result\n");
//                     exit(-1);
//                     }

            //la seguente operazione ha il seguente significato: poichè l'input viene scomposto in 256 blocchi di unsigned e cioè 1024 bytes, bisogna assicurarsi che la dimensione dell'input sia sempre multiplo di tale valore e cioè 1024 appunto.
          //  unsigned modUsefulData = ( usefulData - ( usefulData % 64 ) + 64 );
              unsigned modUsefulData = ( usefulData - ( usefulData % 64 ) + 64 );

            //chiamata all'handler
            int errorReturned = aesHost(h_Result, reinterpret_cast<unsigned char*>(h_Input), modUsefulData, myExpKey, sizeof(myExpKey), MODE);

//             if ( errorReturned == -1 ){
//                     printf("aesHost: cannot use an input size minor of 256\n");
//                     exit(-1);
//                     }
//
//             if ( errorReturned == -11 ){
//                     printf("aesHost: cannot use an input size not multiple of 256\n");
//                     exit(-1);
//                     }
//
//             if ( errorReturned == -2 ){
//                     printf("aesHost: cannot use an expanded key size different from 240 or 176\n");
//                     exit(-1);
//                     }
//
//             if ( errorReturned == -3 ){
//                     printf("aesHost: some input not allocated\n");
//                     exit(-1);
//                     }
#ifndef GKLEE_MODE
            printf("\n###############################################################\n\n");

            char numFile[10000];
            sprintf(numFile, "%d", cnt);
            char *nameOutFile = new char[10000];
            strcpy(nameOutFile, "output_");
            strcat(nameOutFile, numFile);
            strcat(nameOutFile, ".dat");
            printf("Final destination string : |%s|\n", nameOutFile);

            //scrittura risultati in uscita
            writeToFile(nameOutFile, reinterpret_cast<char *>(h_Result), usefulData, INPUTSIZE, MODE);
            delete[] nameOutFile;
#endif

            delete[] h_Result;
            delete[] h_Input;

    }

    return EXIT_SUCCESS;
}

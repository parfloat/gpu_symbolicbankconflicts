/*
 ============================================================================
 Name        : Engine-cuda-minimal.cu
 Author      : Adrian Horga
 Version     :
 Copyright   : Your copyright notice
 Description : CUDA compute reciprocals
 ============================================================================
 */
/**
 * @version 0.1.2 (2010) - Copyright (c) 2010.
 *
 * @author Paolo Margara <paolo.margara@gmail.com>
 *
 * Copyright 2010 Paolo Margara
 *
 * This file is part of Engine_cudamrg.
 *
 * Engine_cudamrg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License or
 * any later version.
 *
 * Engine_cudamrg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Engine_cudamrg.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define BUFFER_SIZE 1024

#include "aes_cuda.h"
void AES_cuda_crypt(cuda_crypt_parameters *c) {

	int gridSize = c->nbytes/(MAX_THREAD*AES_BLOCK_SIZE);

	printf("transfer input to device\n");
	cudaError_t cudaerrno;
	_CUDA(cudaMemcpy((DATA_TYPE *)c->d_in, c->in, c->nbytes, cudaMemcpyHostToDevice));
	printf("finished transfer input to device\n");

	//dim3 dimBlock(STATE_THREAD_AES,MAX_THREAD/STATE_THREAD_AES);
	dim3 dimBlock(MAX_THREAD,1);
	if ((c->nbytes%(MAX_THREAD*STATE_THREAD_AES))==0) {
		gridSize = c->nbytes/(MAX_THREAD*STATE_THREAD_AES);
	} else {
		gridSize = c->nbytes/(MAX_THREAD*STATE_THREAD_AES)+1;
	}

	//	CUDA_START_TIME

	//	switch(c->ctx->key_len) {
	//	case 16:
	printf("AES128 kernel run\n");
	AES128encKernel<<<gridSize,dimBlock>>>((DATA_TYPE *)c->d_in, c->d_key);
	printf("finished AES128 kernel run\n");
	//		break;
	//	case 24:
	//		printf("AES192 kernel run\n");
	//		AES192encKernel<<<gridSize,dimBlock>>>((DATA_TYPE *)c->d_in);
	//		printf("finished AES1192 kernel run\n");
	//		break;
	//	case 32:
	//		printf("AES256 kernel run\n");
	//		AES256encKernel<<<gridSize,dimBlock>>>((DATA_TYPE *)c->d_in);
	//		printf("finished AES256 kernel run\n");
	//		break;
	//	}
	printf("copy output back\n");
	_CUDA(cudaMemcpy(c->out, (DATA_TYPE *)c->d_in, c->nbytes, cudaMemcpyDeviceToHost));
	printf("finished copy output back\n");
}

#include <stdint.h>
//#include "common.h"
#include <stdio.h>
//#include <openssl/aes.h>

void aesEnc128_scenario();

int main(int argc, char **argv) {
	aesEnc128_scenario();

	exit(EXIT_SUCCESS);
}

void aesEnc128_scenario(){
	/* AES Test vector */
	#ifndef GKLEE_MODE
	uint8_t input[AES_BLOCK_SIZE] = 	{ 0x4E, 0xC1, 0x37, 0xA4, 0x26, 0xDA, 0xBF,
			0x8A, 0xA0, 0xBE, 0xB8, 0xBC, 0x0C, 0x2B, 0x89, 0xD6 };
	uint8_t enc_output128[AES_BLOCK_SIZE] = { 0xD9, 0xB6, 0x5D, 0x12, 0x32,
			0xBA, 0x01, 0x99, 0xCD, 0xBD, 0x48, 0x7B, 0x2A, 0x1F, 0xD6, 0x46 };
	uint8_t key128[AES_KEY_SIZE_128] = { 0x95, 0xA8, 0xEE, 0x8E, 0x89, 0x97,
			0x9B, 0x9E, 0xFD, 0xCB, 0xC6, 0xEB, 0x97, 0x97, 0x52, 0x8D };
#endif


#ifdef GKLEE_MODE
	printf("\nAllocating memory for input, output and key...\n");
	uint8_t input[AES_BLOCK_SIZE];
	uint8_t enc_output128[AES_BLOCK_SIZE];
	unsigned char key128[AES_KEY_SIZE_128] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

	// uint8_t *input = (uint8_t *)malloc(AES_BLOCK_SIZE * sizeof(uint8_t));
	// uint8_t *enc_output128 = (uint8_t *)malloc(AES_BLOCK_SIZE * sizeof(uint8_t));
	// uint8_t *key128 = (uint8_t *)malloc(AES_KEY_SIZE_128 * sizeof(uint8_t));

	printf("\nMake input symbolic...\n");
	klee_make_symbolic(input, AES_BLOCK_SIZE * sizeof(uint8_t),"adi_SYM_plain-text");
	printf("\nMake key symbolic...\n");
	klee_make_symbolic(key128, AES_KEY_SIZE_128 * sizeof(uint8_t),"symkey");
	// printf("\nkey (128bit): ");
	// for (int i = 0; i < AES_KEY_SIZE_128; i++)
	// 	printf("%x", key128[i]);
#endif

	/* needed variables */
	int i;
	uint8_t *out;
	AES_KEY *ak;

	//	iv_cbc_tmp=(uint8_t *) malloc(AES_BLOCK_SIZE * sizeof(uint8_t));
	out= (uint8_t *) malloc(AES_BLOCK_SIZE * sizeof(uint8_t));
	ak = (AES_KEY *) malloc(sizeof(AES_KEY));

	/* Show the test vector to the user*/
	#ifndef GKLEE_MODE
		printf("\nAES test vector:\n");
		printf("input: ");
		for (i = 0; i < AES_BLOCK_SIZE; i++)
			printf("%x", input[i]);
		printf("\nkey (128bit): ");
		for (i = 0; i < AES_KEY_SIZE_128; i++)
			printf("%x", key128[i]);
		printf("\nEncrypted output for a 128bit key: ");
		for (i = 0; i < AES_BLOCK_SIZE; i++)
			printf("%x", enc_output128[i]);
	#endif
	/* Encrypt with a 128bit key */
	// printf("\n\nPress any key to start encrypting with a 128bit key...\n");
	/* GPU work */
	AES_cuda_set_encrypt_key((unsigned char*) key128, 128, ak);
	#ifndef GKLEE_MODE
		printf("\nPrinting expanded key...\n");
		for (i = 0; i < 60; i++)
			printf("%x", ak->rd_key[i]);
		printf("\n");
	#endif

	cuda_crypt_parameters *c;
	c = (cuda_crypt_parameters *)malloc(sizeof(cuda_crypt_parameters));
	EVP_CIPHER_CTX ctx;
	c->ctx = &ctx;
	c->ctx->key_len = 16;
	c->in = input;
	c->out = out;
	c->nbytes = AES_BLOCK_SIZE;
	int buffer_size = BUFFER_SIZE;
	cudaError_t cudaerrno;
	printf("allocating d_in\n");
	_CUDA(cudaMalloc((void **)&c->d_in, buffer_size));
	printf("allocating d_out\n");
	_CUDA(cudaMalloc((void **)&c->d_out, buffer_size));
	AES_cuda_transfer_key_schedule(ak, c);

	AES_cuda_crypt(c);

	#ifndef GKLEE_MODE
		printf("output GPU: ");
		for (i = 0; i < AES_BLOCK_SIZE; i++)
			printf("%x", c->out[i]);
		printf("\n");
	#endif

	free(out);
	free(ak);
	cudaFree(c->d_in);
	cudaFree(c->d_out);
	cudaFree(c->d_key);
	free(c);
}

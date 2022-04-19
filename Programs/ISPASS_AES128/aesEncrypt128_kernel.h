
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

/* aes encryption operation:
 * Device code.
 *
 */

#ifndef _AESENCRYPT128_KERNEL_H_
#define _AESENCRYPT128_KERNEL_H_

#include <stdio.h>

// Thread block size
//#define BSIZE 256


#define STAGEBLOCK1(index)	CUT_BANK_CHECKER( stageBlock1, index )
#define STAGEBLOCK2(index)	CUT_BANK_CHECKER( stageBlock2, index )

#define TBOXE0(index)	    CUT_BANK_CHECKER( tBox0Block, index )
#define TBOXE1(index)		CUT_BANK_CHECKER( tBox1Block, index )
#define TBOXE2(index)		CUT_BANK_CHECKER( tBox2Block, index )
#define TBOXE3(index)		CUT_BANK_CHECKER( tBox3Block, index )

//texture<unsigned, 1, cudaReadModeElementType> texEKey128;

__global__ void aesEncrypt128( unsigned * result, unsigned * inData, int inputSize, unsigned *inKey)
{
  unsigned bx		= blockIdx.x;
  unsigned tx		= threadIdx.x;
  unsigned mod4tx = tx%4;
  unsigned int4tx = tx/4;
  unsigned idx2	= int4tx*4;
  int x;
  unsigned keyElem;

   // __shared__ UByte4 stageBlock1[BSIZE];
   // __shared__ UByte4 stageBlock2[BSIZE];
  __shared__ UByte4 stageBlock1[32];
  __shared__ UByte4 stageBlock2[32];


  __shared__ UByte4 tBox0Block[256];
  __shared__ UByte4 tBox1Block[256];
  __shared__ UByte4 tBox2Block[256];
  __shared__ UByte4 tBox3Block[256];

  // input caricati in memoria
/*  printf("inputSize = %d\n", inputSize);
  printf("bx = %d\n", bx);
  printf("tx = %d\n", tx);
  for (int i = 0 ; i < inputSize ; i++){
    char value;
    value = inData[i] & 0xFF;
    printf("indata[%d]0=%d\n", i, value);
    value = (inData[i] >> 8)& 0xFF;
    printf("indata[%d]1=%d\n", i, value);
    value = (inData[i] >> 16) & 0xFF;
    printf("indata[%d]2=%d\n", i, value);
    value = (inData[i] >> 24) & 0xFF;
    printf("indata[%d]3=%d\n", i, value);
  }*/
  STAGEBLOCK1(tx).uival	= inData[BSIZE * bx + tx ];

  unsigned elemPerThread = 256/BSIZE;
  for (unsigned cnt=0; cnt<elemPerThread; cnt++) {
    TBOXE0(tx*elemPerThread + cnt).uival	= TBox0[tx*elemPerThread + cnt];
    TBOXE1(tx*elemPerThread + cnt).uival	= TBox1[tx*elemPerThread + cnt];
    TBOXE2(tx*elemPerThread + cnt).uival	= TBox2[tx*elemPerThread + cnt];
    TBOXE3(tx*elemPerThread + cnt).uival	= TBox3[tx*elemPerThread + cnt];
  }

  __syncthreads();

  //----------------------------------- 1st stage -----------------------------------

  x = mod4tx;
  //keyElem = tex1Dfetch(texEKey128, x);
  keyElem = inKey[x];
  STAGEBLOCK2(tx).uival = STAGEBLOCK1(tx).uival ^ keyElem; //line 0

  __syncthreads();

  //-------------------------------- end of 1st stage --------------------------------


  //----------------------------------- 2nd stage -----------------------------------

  unsigned op1; //should it be unsigned int? we use them as byte values
  unsigned op2;
  unsigned op3;
  unsigned op4;
  unsigned char op1c;
  unsigned char op2c;
  unsigned char op3c;
  unsigned char op4c;

  // unsigned char val1, val2, val3, val4;
  // val1 = posIdx_E[mod4tx*4]   + idx2;
  // val2 = posIdx_E[mod4tx*4+1] + idx2;
  // val3 = posIdx_E[mod4tx*4+2] + idx2;
  // val4 = posIdx_E[mod4tx*4+3] + idx2;
  // printf("Stage 2 tx=%d mod4tx=%d idx2=%d\n", tx, mod4tx, idx2);
  // printf("val1=%d val2=%d val3=%d val4=%d\n", val1, val2, val3, val4);

  op1c = STAGEBLOCK2( posIdx_E[mod4tx*4]   + idx2 ).ubval[0]; //line 1
  op2c = STAGEBLOCK2( posIdx_E[mod4tx*4+1] + idx2 ).ubval[1]; //line 2
  op3c = STAGEBLOCK2( posIdx_E[mod4tx*4+2] + idx2 ).ubval[2]; //line 3
  op4c = STAGEBLOCK2( posIdx_E[mod4tx*4+3] + idx2 ).ubval[3]; //line 4

  // printf("op1=%d op2=%d op3=%d op4=%d\n", op1, op2, op3, op4);

  op1 = TBOXE0(op1c).uival; //line 5

  op2 = TBOXE1(op2c).uival; //line 6

  op3 = TBOXE2(op3c).uival; //line 7

  op4 = TBOXE3(op4c).uival; //line 8

  x = mod4tx+4;
  //keyElem = tex1Dfetch(texEKey128, x);
  keyElem = inKey[x];
  STAGEBLOCK1(tx).uival = op1^op2^op3^op4^keyElem;

  __syncthreads();

  // -------------------------------- end of 2nd stage --------------------------------

  //----------------------------------- 3th stage -----------------------------------

  // val1 = posIdx_E[mod4tx*4]   + idx2;
  // val2 = posIdx_E[mod4tx*4+1] + idx2;
  // val3 = posIdx_E[mod4tx*4+2] + idx2;
  // val4 = posIdx_E[mod4tx*4+3] + idx2;
  // printf("Stage 3 tx=%d mod4tx=%d idx2=%d\n", tx, mod4tx, idx2);
  // printf("val1=%d val2=%d val3=%d val4=%d\n", val1, val2, val3, val4);

  op1c = STAGEBLOCK1( posIdx_E[mod4tx*4]   + idx2 ).ubval[0]; //line 9
  op2c = STAGEBLOCK1( posIdx_E[mod4tx*4+1] + idx2 ).ubval[1]; //line 10
  op3c = STAGEBLOCK1( posIdx_E[mod4tx*4+2] + idx2 ).ubval[2]; //line 11
  op4c = STAGEBLOCK1( posIdx_E[mod4tx*4+3] + idx2 ).ubval[3]; //line 12

  // printf("op1=%d op2=%d op3=%d op4=%d\n", op1, op2, op3, op4);

  op1 = TBOXE0(op1c).uival; //line 13

  op2 = TBOXE1(op2c).uival; //line 14

  op3 = TBOXE2(op3c).uival; //line 15

  op4 = TBOXE3(op4c).uival; //line 16

   x = mod4tx+8;
  /* //keyElem = tex1Dfetch(texEKey128, x); */
   keyElem = inKey[x];
   STAGEBLOCK2(tx).uival = op1^op2^op3^op4^keyElem;

   __syncthreads();

  /* //-------------------------------- end of 3th stage -------------------------------- */

  /* //----------------------------------- 4th stage ----------------------------------- */

   op1c = STAGEBLOCK2( posIdx_E[mod4tx*4]   + idx2 ).ubval[0]; //line 17
   op2c = STAGEBLOCK2( posIdx_E[mod4tx*4+1] + idx2 ).ubval[1]; //line 18
   op3c = STAGEBLOCK2( posIdx_E[mod4tx*4+2] + idx2 ).ubval[2]; //line 19
   op4c = STAGEBLOCK2( posIdx_E[mod4tx*4+3] + idx2 ).ubval[3]; //line 20

   op1 = TBOXE0(op1c).uival; //line 21

   op2 = TBOXE1(op2c).uival; //line 22

   op3 = TBOXE2(op3c).uival; //line 23
   //
   op4 = TBOXE3(op4c).uival; //line 24

//    x = mod4tx+12;
//     //keyElem = tex1Dfetch(texEKey128, x); *\/ */
//    keyElem = inKey[x];
//    STAGEBLOCK1(tx).uival = op1^op2^op3^op4^keyElem;
//
//    __syncthreads();
//
// /*   //-------------------------------- end of 4th stage -------------------------------- */
//
// /*   //----------------------------------- 5th stage ----------------------------------- */
//
//    op1c = STAGEBLOCK1( posIdx_E[mod4tx*4]   + idx2 ).ubval[0]; //line 25
//    op2c = STAGEBLOCK1( posIdx_E[mod4tx*4+1] + idx2 ).ubval[1]; //line 26
//    op3c = STAGEBLOCK1( posIdx_E[mod4tx*4+2] + idx2 ).ubval[2]; //line 27
//    op4c = STAGEBLOCK1( posIdx_E[mod4tx*4+3] + idx2 ).ubval[3]; //line 28
//
//    op1 = TBOXE0(op1c).uival; //line 29
//
//    op2 = TBOXE1(op2c).uival; //line 30 */
//
//    op3 = TBOXE2(op3c).uival; //line 31 */
//
//    op4 = TBOXE3(op4c).uival; //line 32 */
//
//    x = mod4tx+16;
//  //keyElem = tex1Dfetch(texEKey128, x); *\/ */
//    keyElem = inKey[x];
//    STAGEBLOCK2(tx).uival = op1^op2^op3^op4^keyElem;
//
//    __syncthreads();
//
// /*   //-------------------------------- end of 5th stage -------------------------------- */
//
// /*   //----------------------------------- 6th stage ----------------------------------- */
//
//    op1c = STAGEBLOCK2( posIdx_E[mod4tx*4]   + idx2 ).ubval[0]; //line 33 */
//    op2c = STAGEBLOCK2( posIdx_E[mod4tx*4+1] + idx2 ).ubval[1]; //line 34 */
//    op3c = STAGEBLOCK2( posIdx_E[mod4tx*4+2] + idx2 ).ubval[2]; //line 35 */
//    op4c = STAGEBLOCK2( posIdx_E[mod4tx*4+3] + idx2 ).ubval[3]; //line 36 */
//
//    op1 = TBOXE0(op1c).uival; //line 37 */
//
//    op2 = TBOXE1(op2c).uival; //line 38 */
//
//    op3 = TBOXE2(op3c).uival; //line 39 */
//
//    op4 = TBOXE3(op4c).uival; //line 40 */
//
//    x = mod4tx+20;
//    //keyElem = tex1Dfetch(texEKey128, x); */
//    keyElem = inKey[x];
//    STAGEBLOCK1(tx).uival = op1^op2^op3^op4^keyElem;
//
//    __syncthreads();
//
// /*   //-------------------------------- end of 6th stage -------------------------------- */
//
// /*   //----------------------------------- 7th stage ----------------------------------- */
//
//    op1c = STAGEBLOCK1( posIdx_E[mod4tx*4]   + idx2 ).ubval[0]; //line 41 */
//    op2c = STAGEBLOCK1( posIdx_E[mod4tx*4+1] + idx2 ).ubval[1]; //line 42 */
//    op3c = STAGEBLOCK1( posIdx_E[mod4tx*4+2] + idx2 ).ubval[2]; //line 43 */
//    op4c = STAGEBLOCK1( posIdx_E[mod4tx*4+3] + idx2 ).ubval[3]; //line 44 */
//
//    op1 = TBOXE0(op1c).uival; //line 45 */
//
//    op2 = TBOXE1(op2c).uival; //line 46 */
//
//    op3 = TBOXE2(op3c).uival; //line 47 */
//
//    op4 = TBOXE3(op4c).uival; //line 48 */
//
//    x = mod4tx+24;
//    //keyElem = tex1Dfetch(texEKey128, x); */
//    keyElem = inKey[x];
//    STAGEBLOCK2(tx).uival = op1^op2^op3^op4^keyElem;
//
//    __syncthreads();
//
//    //-------------------------------- end of 7th stage -------------------------------- */
//
//    //----------------------------------- 8th stage ----------------------------------- */
//
//    op1c = STAGEBLOCK2( posIdx_E[mod4tx*4]   + idx2 ).ubval[0]; //line 49 */
//    op2c = STAGEBLOCK2( posIdx_E[mod4tx*4+1] + idx2 ).ubval[1]; //line 50 */
//    op3c = STAGEBLOCK2( posIdx_E[mod4tx*4+2] + idx2 ).ubval[2]; //line 51 */
//    op4c = STAGEBLOCK2( posIdx_E[mod4tx*4+3] + idx2 ).ubval[3]; //line 52 */
//
//    op1 = TBOXE0(op1c).uival; //line 53 */
//
//    op2 = TBOXE1(op2c).uival; //line 54 */
//
//    op3 = TBOXE2(op3c).uival; //line 55 */
//
//    op4 = TBOXE3(op4c).uival; //line 56 */
//
//    x = mod4tx+28;
//    //keyElem = tex1Dfetch(texEKey128, x); */
//    keyElem = inKey[x];
//    STAGEBLOCK1(tx).uival = op1^op2^op3^op4^keyElem;
//
//    __syncthreads();
//
// /*   //-------------------------------- end of 8th stage -------------------------------- */
//
// /*   //----------------------------------- 9th stage ----------------------------------- */
//
//    op1c = STAGEBLOCK1( posIdx_E[mod4tx*4]   + idx2 ).ubval[0]; //line 57 */
//   op2c = STAGEBLOCK1( posIdx_E[mod4tx*4+1] + idx2 ).ubval[1]; //line 58 */
//    op3c = STAGEBLOCK1( posIdx_E[mod4tx*4+2] + idx2 ).ubval[2]; //line 59 */
//    op4c = STAGEBLOCK1( posIdx_E[mod4tx*4+3] + idx2 ).ubval[3]; //line 60 */
//
//    op1 = TBOXE0(op1c).uival; //line 61 */
//
//    op2 = TBOXE1(op2c).uival; //line 62 */
//
//    op3 = TBOXE2(op3c).uival; //line 63 */
//
//    op4 = TBOXE3(op4c).uival; //line 64 */
//
//    x = mod4tx+32;
//    //keyElem = tex1Dfetch(texEKey128, x); */
//    keyElem = inKey[x];
//    STAGEBLOCK2(tx).uival = op1^op2^op3^op4^keyElem;
//
//    __syncthreads();
//
// /*   //-------------------------------- end of 9th stage -------------------------------- */
//
// /*   //----------------------------------- 10th stage ----------------------------------- */
//
//    op1c = STAGEBLOCK2( posIdx_E[mod4tx*4]   + idx2 ).ubval[0];  //line 65 */
//    op2c = STAGEBLOCK2( posIdx_E[mod4tx*4+1] + idx2 ).ubval[1]; //line 66 */
//    op3c = STAGEBLOCK2( posIdx_E[mod4tx*4+2] + idx2 ).ubval[2]; //line 67 */
//    op4c = STAGEBLOCK2( posIdx_E[mod4tx*4+3] + idx2 ).ubval[3]; //line 68 */
//
//    op1 = TBOXE0(op1c).uival; //line 69 */
//
//    op2 = TBOXE1(op2c).uival; //line 70 */
//
//    op3 = TBOXE2(op3c).uival; //line 71 */
//
//    op4 = TBOXE3(op4c).uival; //line 72 */
//
//    x = mod4tx+36;
//    //keyElem = tex1Dfetch(texEKey128, x); */
//    keyElem = inKey[x];
//    STAGEBLOCK1(tx).uival = op1^op2^op3^op4^keyElem;
//
//    __syncthreads();
//
// /*   //-------------------------------- end of 10th stage -------------------------------- */
//
// /*   //----------------------------------- 11th stage ----------------------------------- */
//
//    op1c = STAGEBLOCK1( posIdx_E[mod4tx*4]   + idx2 ).ubval[0]; //line 73 */
//    op2c = STAGEBLOCK1( posIdx_E[mod4tx*4+1] + idx2 ).ubval[1]; //line 74 */
//    op3c = STAGEBLOCK1( posIdx_E[mod4tx*4+2] + idx2 ).ubval[2]; //line 75 */
//    op4c = STAGEBLOCK1( posIdx_E[mod4tx*4+3] + idx2 ).ubval[3]; //line 76 */
//
//    x = mod4tx+40;
//    //keyElem = tex1Dfetch(texEKey128, x); */
//    keyElem = inKey[x];
//
//
//    STAGEBLOCK2(tx).ubval[3] = TBOXE1(op4c).ubval[3]^( keyElem>>24);  //line 77 */
//    STAGEBLOCK2(tx).ubval[2] = TBOXE1(op3c).ubval[3]^( (keyElem>>16) & 0x000000FF);  //line 78 */
//    STAGEBLOCK2(tx).ubval[1] = TBOXE1(op2c).ubval[3]^( (keyElem>>8)  & 0x000000FF); //line 79 */
//    STAGEBLOCK2(tx).ubval[0] = TBOXE1(op1c).ubval[3]^( keyElem       & 0x000000FF); //line 80 */
//
//    __syncthreads();
//
// /* //   -------------------------------- end of 15th stage -------------------------------- */
//
//    result[BSIZE * bx + tx] = STAGEBLOCK2(tx).uival;  //line 81 */
/*   // end of AES */

}

#endif // #ifndef _AESENCRYPT_KERNEL_H_

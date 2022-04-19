
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

//#include "boost/filesystem/operations.hpp"
//#include <iostream>
//#include <vector>
#include "UVector.h"
#include <stdio.h>
#include <stdlib.h>
//#include <iterator>
//#include <functional>
//#include <fstream>

//using namespace boost::filesystem;

//------------------------------------------help-functions-------------------------------------------------------
unsigned mySbox(unsigned num);

unsigned myXor(unsigned num1, unsigned num2);

void transform_UVector_mySbox(UVector &first, unsigned start1, unsigned end1, UVector &result, unsigned result_start){
   while (start1 != end1) {
    result[result_start] = mySbox(first[start1]);
    ++start1; ++result_start;
  }
}

void transform_UVector_myXor(UVector &first, unsigned start1, unsigned end1, UVector &second, unsigned start2, UVector &result, unsigned result_start){
   while (start1 != end1) {
    result[result_start] = myXor(first[start1], second[start2]);
    ++start1; ++start2; ++result_start;
  }
}


//------------------------------------------help-functions-------------------------------------------------------


//------------------------------------------AESCudaUtils-----------------------------------------------------------
//------------------------------------------AESCudaUtils-----------------------------------------------------------
//------------------------------------------AESCudaUtils-----------------------------------------------------------
unsigned commandLineManager(int argc, char *argv[]); //gklee safe

void usage(); //gklee safe

//unsigned initAesCuda(std::string myKeyFile, unsigned char myKeyBuffer[], const unsigned int myKeyBitsSize, std::string myInputFile, char inputArray[], const unsigned inputArraySize);
unsigned initAesCuda(char* myKeyFile, unsigned char myKeyBuffer[], const unsigned int myKeyBitsSize, char* myInputFile, unsigned char inputArray[], const unsigned inputArraySize);

//long int getFileSize(path &myPath);
long int getFileSize(char *myPath); // gklee safe

//void readFromFileNotForm(path &myPath, char *storingArray, unsigned dataSize);
void readFromFileNotForm(char *myPath, unsigned char *storingArray, unsigned dataSize); //gklee safe

//void readFromFileForm(path &myPath, std::vector<unsigned> &storingArray);
void readFromFileForm(char* myPath, UVector &storingArray); //gklee safe

void expFunc(UVector &keyArray, UVector &expKeyArray);

void singleStep(UVector &expKey, unsigned step);

void invExpFunc(UVector &expKey, UVector &invExpKeyArray);

void invMixColumn(UVector &temp);

unsigned galoisProd(unsigned a, unsigned b); //gklee safe

//void writeToFile(const std::string &outPath, char *storingArray, boost::intmax_t dataSize, unsigned maxInputSize);
void writeToFile(const char* outPath, char *storingArray, long int dataSize, unsigned maxInputSize); //gklee safe
//---------------------------------------------start the implementation-------------------------------------------------


extern unsigned Rcon[];
extern unsigned SBox[];
extern unsigned LogTable[];
extern unsigned ExpoTable[];

//extern unsigned MODE;
//extern bool MODE;
//bool MODE = 1;

//------------------------------------------help-functions-------------------------------------------------------
unsigned mySbox(unsigned num) {
	return SBox[num];
}

unsigned myXor(unsigned num1, unsigned num2) {
	return num1 ^ num2;
}

//------------------------------------------help-functions-------------------------------------------------------


//------------------------------------------AESCudaUtils-----------------------------------------------------------
//------------------------------------------AESCudaUtils-----------------------------------------------------------
//------------------------------------------AESCudaUtils-----------------------------------------------------------

// gestisce la riga di commando
unsigned commandLineManager(int argc, char *argv[], bool &MODE) {

// 	if ( argc<5 )
// 		usage();
//
// 	if ( argc%2 != 1)
// 		usage();
//
// 	if ( (*argv[1] != 'e') && (*argv[1] != 'd') )
// 		usage();
//
// 	if ( ( strcmp(argv[2], "256") ) && ( strcmp(argv[2], "128") ) )
// 		usage();
//
	if (*argv[1] != 'e')
		MODE = 0;

	//controllo esistenza file in futuro

	return ( (argc-3) / 2 );
}

void usage() {
// 	printf("\nAES - CUDA by Svetlin Manavski\n");
// 	printf("Version 0.90\n\n");
// 	printf("Usage:\n\n");
// 	printf("aescuda <e|d> <128|256> input1 key1 input2 key2 .... inputN keyN\n");
// 	printf("e\t- means to encrypt the input file\n");
// 	printf("d\t- means to decrypt the input file\n");
// 	printf("128/256\t- selects the aes type\n\n");
// 	printf("input file maximum size: 34.603.007 bytes\n");
// 	printf("key file must contain 32 or 16 elements in hex format separated by blanks\n\n");
	exit(0);
}

//funzione principale
//unsigned initAesCuda(std::string myKeyFile, unsigned char myKeyBuffer[], const unsigned int myKeyBitsSize, std::string myInputFile, char inputArray[], const unsigned inputArraySize){
unsigned initAesCuda(char* myKeyFile, unsigned char myKeyBuffer[], const unsigned int myKeyBitsSize, char* myInputFile, unsigned char inputArray[], const unsigned inputArraySize, bool &MODE){

	if ( myKeyBitsSize!=256 && myKeyBitsSize!=128){
//             printf("cannot use a key dimension different from 256 or 128\n");
            exit(-1);
        }

	if ( !myKeyBuffer ){
//             printf("key array not allocated\n");
            exit(-1);
        }

	if ( !inputArray ){
//             printf("input array not allocated\n");
            exit(-1);
        }

	long int inputFileSize;
	long int keyFileSize;

#ifndef GKLEE_MODE
      inputFileSize = getFileSize(myInputFile);
      // inputFileSize = 16;
       keyFileSize = getFileSize(myKeyFile);
#else
      inputFileSize = getFileSize(myInputFile);;
#endif



// 	if ( keyFileSize==0 ){
//             printf("cannot use an empty input file\n");
//             exit(-1);
//         }
//
// 	if ( inputFileSize==0 ){
//             printf("cannot use an empty key file\n");
//             exit(-1);
//         }
//
// 	if ( inputFileSize > inputArraySize - 1 && MODE){
//             printf("cannot encrypt a file bigger than 34.603.007 bytes\n");
//             exit(-1);
//         }
//
// 	if ( inputFileSize > inputArraySize && !MODE){
//             printf("cannot decrypt a file bigger than 33MB\n");
//             exit(-1);
//         }

	//legge l'input
	readFromFileNotForm(myInputFile, inputArray, inputFileSize);

	// klee_make_symbolic(inputArray, inputArraySize,"adi_SYM_plain-text");

  UVector keyArray(myKeyBitsSize/8);

	unsigned ekSize = (myKeyBitsSize != 256) ? 176 : 240;

	UVector expKeyArray(ekSize);
	UVector invExpKeyArray(ekSize);


	//legge la chiave
#ifndef GKLEE_MODE
        printf("Reading key from file -- regular execution\n");
        readFromFileForm(myKeyFile, keyArray);
         printf("\n###############################################################\n\n");
         printf("AES - CUDA by Svetlin Manavski)\n\n");
         printf("AES %u is running....\n\n", myKeyBitsSize);
         printf("Input file size: %ld bytes\n\n", inputFileSize);
         printf("Key: ");
        for (unsigned cnt=0; cnt<keyArray.size(); ++cnt)
         	// std::cout << std::hex << keyArray[cnt];
                   printf("%X", keyArray[cnt]);
#else
  klee_make_symbolic(inputArray, inputArraySize,"adi_SYM_plain-text");
  unsigned char fixedPlain[] = {0, 65, 134, 8, 14, 46, 35, 55, 25, 4, 55, 25, 153, 222, 83, 221};
  printf("Set the symbolic input to be fixed for datasize %d : \n", inputArraySize);
  for (int i  = 0 ; i < inputFileSize; i++){
          klee_assume(inputArray[i] == fixedPlain[i]);//adrianh : for a fixed plain text
        printf("%c", inputArray[i]);
    }
    printf("\n");
	//symbolic key
           printf("Making key symbolic -- symbolic execution\n");
        unsigned char symbKeyBuffer[myKeyBitsSize/8];

	klee_make_symbolic(symbKeyBuffer, myKeyBitsSize/8 * sizeof(unsigned char), "symkey");

  unsigned char fixedKey[] = {0x20, 0x61, 0xe6, 0x48, 0x0e, 0x4e, 0x03, 0x37, 0x59, 0x04, 0x37, 0x39, 0x19, 0x9e, 0x13, 0xbd};
	for(unsigned i=0; i < keyArray.size(); ++i){
	  keyArray[i]=symbKeyBuffer[i];
	  klee_assume(keyArray[i] <= 0xFF);
    // klee_assume(keyArray[i] == fixedKey[i]); //adrianh set key to fixed -- remove if searching for keys
	}/**/

  //klee_assume(keyArray[0] == 0x20);
  // klee_assume(keyArray[0] == 0x21);
  //klee_assume(keyArray[0] == 0x22);
#endif



	if (MODE){
		//ENCRYPTION MODE

		//PADDING MANAGEMENT FOLLOWING THE PKCS STANDARD
		unsigned mod16 = inputFileSize % 16;
		unsigned div16 = inputFileSize / 16;
    printf("mod16 = %d , div16 = %d", mod16, div16);

		unsigned padElem;
		if ( mod16 != 0 )
			padElem =  16 - mod16;
		else
			padElem =  16;

		for (unsigned cnt = 0; cnt < padElem; ++cnt)
				inputArray[div16*16 + mod16 + cnt] = padElem;

		inputFileSize = inputFileSize + padElem;

		//IN THE ENCRYPTION MODE I NEED THE EXPANDED KEY
		expFunc(keyArray, expKeyArray);
		for (unsigned cnt=0; cnt<expKeyArray.size(); ++cnt){
			unsigned val = expKeyArray[cnt];
			unsigned char *pc = reinterpret_cast<unsigned char *>(&val);
			myKeyBuffer[cnt] = *(pc);
		}

// 		if(myKeyBuffer[0])
// 		  printf("achoum\n");
// 		else
// 		  printf("moucha\n");
	} else {
		//DECRYPTION MODE

		//IN THE ENCRYPTION MODE I NEED THE INVERSE EXPANDED KEY
		expFunc(keyArray, expKeyArray);
		invExpFunc(expKeyArray, invExpKeyArray);
		for (unsigned cnt=0; cnt<invExpKeyArray.size(); ++cnt){
			unsigned val = invExpKeyArray[cnt];
			unsigned char *pc = reinterpret_cast<unsigned char *>(&val);
			myKeyBuffer[cnt] = *(pc);
		}
	}
	//std::cout << std::endl;
        printf("\n");

	return inputFileSize;
}

long int fsize(FILE *fp){
    long int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    long int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}

long int getFileSize(char *myPath){

    FILE* in_file = fopen(myPath, "rb");

    if ( !in_file ){
         printf("file %s doesn't exist\n", myPath);
        exit(-1);
    }
    long int file_size = fsize(in_file);
    fclose(in_file);
    return file_size;
}

//legge file non formattati come l'input
/*void readFromFileNotForm(path &myPath, char *storingArray, unsigned dataSize){
    if ( !exists(myPath) )
		throw std::string("file "+myPath.string()+" doesn't exist");
	if ( !storingArray )
		throw std::string("readFromFileNotForm: array not allocated");

	std::ifstream inStream;
	inStream.open( myPath.string().c_str(), std::ifstream::binary );
	inStream.read(storingArray, dataSize);
	inStream.close();
}*/

/*void readFromFileNotForm(char* myPath, char *storingArray, unsigned dataSize){ //actual reading from file

    if ( !storingArray ){
            printf("writeToFile: array not allocated");
            exit(-1);
    }


        FILE* in_file = fopen(myPath, "rb");

        if ( !in_file ){
            printf("file %s doesn't exist\n", myPath);
            exit(-1);
        }

        int read = fread(storingArray, sizeof(char), dataSize, in_file);
         if (read == 0) {
            printf("WARNING: The read input file is empty !");
        }

    fclose(in_file);
}*/

void readFromFileNotForm(char* myPath, unsigned char *storingArray, unsigned dataSize){

    if ( !storingArray ){
//             printf("writeToFile: array not allocated");
            exit(-1);
    }


        FILE* in_file = fopen(myPath, "rb");

        if ( !in_file ){
            printf("file %s doesn't exist\n", myPath);
            exit(-1);
        }

        int read = fread(storingArray, sizeof(unsigned char), dataSize, in_file);
         if (read == 0) {
            printf("WARNING: The read input file is empty !");
        }

    fclose(in_file);

  /*  for (int i  = 0 ; i < dataSize; i++){
        storingArray[i] = 48 + i;
    }*/
    unsigned char fixedPlain[] = {0, 65, 134, 8, 14, 46, 35, 55, 25, 4, 55, 25, 153, 222, 83, 221};
    // {7, 30, 17, 7, 106, 3, 218, 10, 7, 26, 116, 9, 71, 46, 55, 103};
    //{239, 148, 79, 120, 167, 114, 167, 144, 19, 47, 84, 28, 251, 4, 97, 211};//adrianh : for a fixed plain text
    printf("Read the input %d : \n", dataSize);
    for (int i  = 0 ; i < dataSize; i++){
          storingArray[i] = fixedPlain[i];//adrianh : for a fixed plain text
          printf("%c", storingArray[i]);
      }
      printf("\n");
    for (int i  = 0 ; i < dataSize; i++){
          storingArray[i] = fixedPlain[i];//adrianh : for a fixed plain text
          printf("%u ", (unsigned)storingArray[i]);
      }
      printf("\n");
}

//legge file formattati come la chiave (esadecimali separati da spazi)
/*void readFromFileForm(path &myPath, std::vector<unsigned> &storingArray){
	if ( !exists(myPath) )
		throw std::string("file "+myPath.string()+" doesn't exist");
	if ( storingArray.size()!=32 && storingArray.size()!=16)
		throw std::string("readFromFileForm: storing array of wrong dimension");

	std::ifstream inStream;
	inStream>>std::hex;
	inStream.open( myPath.string().c_str() );

	for (unsigned cnt=0; cnt<storingArray.size(); ++cnt){
		inStream >> storingArray[cnt];
		if ( ( inStream.eof() ) && ( cnt != storingArray.size()-1 ) )
			throw std::string("cannot use a key with less than 32 or 16 elements ");
		if ( ( !inStream.eof() ) && ( cnt == storingArray.size()-1 ) )
			std::cout << "WARNING: your key file has more than 32 or 16 elements. It will be cut down to this threshold\n";
		if ( inStream.fail() )
			throw std::string("Check that your key file elements are in hexadecimal format separeted by blanks");
	}

	inStream.close();
}*/

void readFromFileForm(char *myPath, UVector &storingArray){
	FILE* in_file = fopen(myPath, "r");

        if ( !in_file ){
//             printf("file %s doesn't exist\n", myPath);
            exit(-1);
        }
	if ( storingArray.size()!=32 && storingArray.size()!=16){
//             printf("readFromFileForm: storing array of wrong dimension\n");
            exit(-1);
        }

	//inStream>>std::hex;
	int hexChar;
	for (unsigned cnt=0; cnt<storingArray.size(); ++cnt){
		fscanf(in_file, "%x", &hexChar);
                //printf("%X ", hexChar);
                storingArray[cnt] = hexChar;
                //printf("value=%X ", storingArray[cnt]);
		if ( ( feof(in_file) ) && ( cnt != storingArray.size()-1 ) ) {
//                     printf("cannot use a key with less than 32 or 16 elements\n");
                    exit(-1);
                }
		if ( ( !feof(in_file) ) && ( cnt == storingArray.size()-1 ) ) {
//                     printf("WARNING: your key file has more than 32 or 16 elements. It will be cut down to this threshold\n");
                    //exit(-1);
                }
		//if ( inStream.fail() )
		//	throw std::string("Check that your key file elements are in hexadecimal format separeted by blanks");
	}

        fclose(in_file);
}

//espande la chiave
void expFunc(UVector &keyArray, UVector &expKeyArray){
	if ( keyArray.size()!=32 && keyArray.size()!=16 ){
//             printf("expFunc: key array of wrong dimension");
            exit(-1);
        }
	if ( expKeyArray.size()!=240 && expKeyArray.size()!=176 ){
//             printf("expFunc: expanded key array of wrong dimension");
            exit(-1);
        }

	//copy(keyArray.begin(), keyArray.end(), expKeyArray.begin());
        copy_UVector(keyArray, keyArray.begin(), keyArray.end(), expKeyArray, expKeyArray.begin());

	unsigned cycles = (expKeyArray.size()!=240) ? 11 : 8;

	for (unsigned i=1; i<cycles; ++i){
		singleStep(expKeyArray, i);
	}
}

void singleStep(UVector &expKey, unsigned stepIdx){
	if ( expKey.size()!=240 && expKey.size()!=176 ){
//             printf("singleStep: expanded key array of wrong dimension");
            exit(-1);
        }
	if ( stepIdx<1 && stepIdx>11 ){
//             printf("singleStep: index out of range");
            exit(-1);
        }

	unsigned num = (expKey.size()!=240) ? 16 : 32;
	unsigned idx = (expKey.size()!=240) ? 16*stepIdx : 32*stepIdx;

	//copy(expKey.begin()+(idx)-4, expKey.begin()+(idx),expKey.begin()+(idx));
        copy_UVector(expKey, (idx)-4, (idx), expKey, (idx));
	//rotate(expKey.begin()+(idx), expKey.begin()+(idx)+1, expKey.begin()+(idx)+4);
        rotate_UVector(expKey, expKey.begin()+(idx), expKey.begin()+(idx)+1, expKey.begin()+(idx)+4);

	//transform(expKey.begin()+(idx), expKey.begin()+(idx)+4, expKey.begin()+(idx), mySbox);
        transform_UVector_mySbox(expKey, expKey.begin()+(idx), expKey.begin()+(idx)+4, expKey, expKey.begin()+(idx));

	expKey[idx] = expKey[idx] ^ Rcon[stepIdx-1];

	//transform(expKey.begin()+(idx), expKey.begin()+(idx)+4, expKey.begin()+(idx)-num, expKey.begin()+(idx), myXor);
        transform_UVector_myXor(expKey, expKey.begin()+(idx), expKey.begin()+(idx)+4, expKey, expKey.begin()+(idx)-num, expKey, expKey.begin()+(idx));

	for (unsigned cnt=0; cnt<3; ++cnt){
		//copy(expKey.begin()+(idx)+4*cnt, expKey.begin()+(idx)+4*(cnt+1),expKey.begin()+(idx)+(4*(cnt+1)));
                copy_UVector(expKey, (idx)+4*cnt, (idx)+4*(cnt+1), expKey, (idx)+(4*(cnt+1)));
		//transform(expKey.begin()+(idx)+4*(cnt+1), expKey.begin()+(idx)+4*(cnt+2), expKey.begin()+(idx)-(num-4*(cnt+1)), expKey.begin()+(idx)+4*(cnt+1), myXor);
                transform_UVector_myXor(expKey, expKey.begin()+(idx)+4*(cnt+1), expKey.begin()+(idx)+4*(cnt+2), expKey, expKey.begin()+(idx)-(num-4*(cnt+1)), expKey, expKey.begin()+(idx)+4*(cnt+1));
	}

	if(stepIdx!=7 && expKey.size()!=176){
		//copy(expKey.begin()+(idx)+12, expKey.begin()+(idx)+16,expKey.begin()+(idx)+16);
                copy_UVector(expKey, (idx)+12, (idx)+16, expKey, (idx)+16);
		//transform(expKey.begin()+(idx)+16, expKey.begin()+(idx)+20, expKey.begin()+(idx)+16, mySbox);
                transform_UVector_mySbox(expKey, expKey.begin()+(idx)+16, expKey.begin()+(idx)+20, expKey, expKey.begin()+(idx)+16);
		//transform(expKey.begin()+(idx)+16, expKey.begin()+(idx)+20, expKey.begin()+(idx)-(32-16), expKey.begin()+(idx)+16, myXor);
                transform_UVector_myXor(expKey, expKey.begin()+(idx)+16, expKey.begin()+(idx)+20, expKey, expKey.begin()+(idx)-(32-16), expKey, expKey.begin()+(idx)+16);

		for (unsigned cnt=4; cnt<7; ++cnt){
			//copy(expKey.begin()+(idx)+4*cnt, expKey.begin()+(idx)+4*(cnt+1),expKey.begin()+(idx)+(4*(cnt+1)));
                        copy_UVector(expKey, (idx)+4*cnt, (idx)+4*(cnt+1), expKey, (idx)+(4*(cnt+1)));
			//transform(expKey.begin()+(idx)+4*(cnt+1), expKey.begin()+(idx)+4*(cnt+2), expKey.begin()+(idx)-(32-4*(cnt+1)), expKey.begin()+(idx)+4*(cnt+1), myXor);
                        transform_UVector_myXor(expKey, expKey.begin()+(idx)+4*(cnt+1), expKey.begin()+(idx)+4*(cnt+2), expKey, expKey.begin()+(idx)-(32-4*(cnt+1)), expKey, expKey.begin()+(idx)+4*(cnt+1));
		}
	}
}

//espande la chiave inversa per la decriptazione
void invExpFunc(UVector &expKey, UVector &invExpKey){
	if ( expKey.size()!=240 && expKey.size()!=176 ){
//             printf("invExpFunc: expanded key array of wrong dimension");
            exit(-1);
        }
	if ( invExpKey.size()!=240 && invExpKey.size()!=176 ){
//             printf("invExpFunc: inverse expanded key array of wrong dimension");
            exit(-1);
        }
	UVector temp(16);

	//copy(expKey.begin(), expKey.begin()+16,invExpKey.end()-16);
        copy_UVector(expKey, expKey.begin() , expKey.begin()+16, invExpKey, invExpKey.end()-16);
	//copy(expKey.end()-16, expKey.end(),invExpKey.begin());
        copy_UVector(expKey, expKey.end()-16, expKey.end(), invExpKey, invExpKey.begin());

	unsigned cycles = (expKey.size()!=240) ? 10 : 14;

	for (unsigned cnt=1; cnt<cycles; ++cnt){
		//copy(expKey.end()-(16*cnt+16), expKey.end()-(16*cnt), temp.begin());
                copy_UVector(expKey, expKey.end()-(16*cnt+16), expKey.end()-(16*cnt), temp, temp.begin());
		invMixColumn(temp);
		//copy(temp.begin(), temp.end(), invExpKey.begin()+(16*cnt));
                copy_UVector(temp, temp.begin(), temp.end(), invExpKey, invExpKey.begin()+(16*cnt));
	}
}

void invMixColumn(UVector &temp){
	if ( temp.size()!=16 ){
//             printf("invMixColumn: array of wrong dimension");
            exit(-1);
        }

	UVector result(4);

	for(unsigned cnt=0; cnt<4; ++cnt){
		result[0] = galoisProd(0x0e, temp[cnt*4]) ^ galoisProd(0x0b, temp[cnt*4+1]) ^ galoisProd(0x0d, temp[cnt*4+2]) ^ galoisProd(0x09, temp[cnt*4+3]);
		result[1] = galoisProd(0x09, temp[cnt*4]) ^ galoisProd(0x0e, temp[cnt*4+1]) ^ galoisProd(0x0b, temp[cnt*4+2]) ^ galoisProd(0x0d, temp[cnt*4+3]);
		result[2] = galoisProd(0x0d, temp[cnt*4]) ^ galoisProd(0x09, temp[cnt*4+1]) ^ galoisProd(0x0e, temp[cnt*4+2]) ^ galoisProd(0x0b, temp[cnt*4+3]);
		result[3] = galoisProd(0x0b, temp[cnt*4]) ^ galoisProd(0x0d, temp[cnt*4+1]) ^ galoisProd(0x09, temp[cnt*4+2]) ^ galoisProd(0x0e, temp[cnt*4+3]);

		//copy(result.begin(), result.end(), temp.begin()+(4*cnt));
                copy_UVector(result, result.begin(), result.end(), temp, temp.begin()+(4*cnt));
	}
}

//prodotto di Galois di due numeri
unsigned galoisProd(unsigned a, unsigned b){

	if(a==0 || b==0) return 0;
	else {
		a = LogTable[a];
		b = LogTable[b];
		a = a+b;
		a = a % 255;
		a = ExpoTable[a];
		return a;
	}
}

//scrive su file il risultato
//void writeToFile(const std::string &outPath, char *storingArray, boost::intmax_t dataSize, unsigned maxInputSize){
void writeToFile(const char* outPath, char *storingArray, long int dataSize, unsigned maxInputSize, bool &MODE){
	if ( !storingArray ){
// 		printf("writeToFile: array not allocated");
                exit(-1);
        }

	if ( dataSize >  maxInputSize)
		dataSize = maxInputSize;

	/*std::ofstream outStream;
	outStream.open( outPath , std::ifstream::binary);
        */
	if (!MODE)
		dataSize = dataSize - storingArray[dataSize-1];
        /*
	outStream.write(storingArray, dataSize);
	outStream.close();
        */

         FILE* out_file = fopen(outPath, "wb");
         if ( !out_file ){
             printf("file %s doesn't exist\n", outPath);
            exit(-1);
        }

        printf("dataSize=%ld\n", dataSize);
        for (size_t i = 0; i < dataSize; i++) {
          /* code */
          printf("%c ", storingArray[i]);
        }
        printf("\n");
         //int written = fwrite(storingArray, sizeof(char), dataSize * sizeof(storingArray), out_file);
          int written = fwrite(storingArray, sizeof(char), dataSize, out_file);
         if (written == 0) {
//             printf("Error during writing to file !");
            exit(-1);
        }
        fclose(out_file);
}

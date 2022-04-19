// vim:foldenable:foldmethod=marker:foldmarker=[[,]]
/**
 * @version 0.1.3 (2011)
 * @author Johannes Gilger <heipei@hackvalue.de>
 *
 * Copyright 2011 Johannes Gilger
 *
 * This file is part of engine-cuda
 *
 * engine-cuda is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License or
 * any later version.
 *
 * engine-cuda is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with engine-cuda. If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef common_h
#define common_h

static int __attribute__((unused)) output_verbosity;
static int __attribute__((unused)) isIntegrated;

#define OUTPUT_QUIET		0
#define OUTPUT_NORMAL		1
#define OUTPUT_VERBOSE		2

#define NUM_BLOCK_PER_MULTIPROCESSOR	3
#define SIZE_BLOCK_PER_MULTIPROCESSOR	256*1024
#ifndef MAX_THREAD
#define MAX_THREAD			4//256
#endif

#define STATE_THREAD_AES	4
#define AES_BLOCK_SIZE		16
#define AES_KEY_SIZE_128	16
#define AES_KEY_SIZE_192	24
#define AES_KEY_SIZE_256	32

#define STATE_THREAD_DES	2
#define DES_MAXNR		8
#define DES_BLOCK_SIZE		8
#define DES_KEY_SIZE		8
#define DES_KEY_SIZE_64		8

#define IDEA_BLOCK_SIZE		8
#define IDEA_KEY_SIZE		8
#define IDEA_KEY_SIZE_64	8

#define BF_BLOCK_SIZE		8
#define BF_KEY_SIZE_64		8
#define BF_KEY_SIZE_128		16

#define CAST_BLOCK_SIZE		8
#define CAST_KEY_SIZE_128	16

#define CMLL_BLOCK_SIZE		16
#define CMLL_KEY_SIZE_128	16
#define CMLL_KEY_SIZE_192	24
#define CMLL_KEY_SIZE_256	32

#define CAMELLIA_BLOCK_SIZE	16
#define CAMELLIA_KEY_SIZE_128	16
#define CAMELLIA_KEY_SIZE_192	24
#define CAMELLIA_KEY_SIZE_256	32

void cuda_device_init(int *nm, int buffer_size, int output_verbosity, uint8_t **host_data, uint64_t **device_data, uint64_t**);
void cuda_device_finish(uint8_t *host_data, uint64_t *device_data);

//#include <openssl/evp.h>
//#include "common/evp.h"
#define EVP_MAX_MD_SIZE			64	/* longest known is SHA512 */
#define EVP_MAX_KEY_LENGTH		64
#define EVP_MAX_IV_LENGTH		16
#define EVP_MAX_BLOCK_LENGTH		32

typedef struct evp_cipher_st EVP_CIPHER;
typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
//typedef struct engine_st ENGINE;

struct evp_cipher_ctx_st
{
	const EVP_CIPHER *cipher;
	//	ENGINE *engine;	/* functional reference if 'cipher' is ENGINE-provided */ commented - don't think it is needed
	int encrypt;		/* encrypt or decrypt */
	int buf_len;		/* number we have left */

	unsigned char  oiv[EVP_MAX_IV_LENGTH];	/* original iv */
	unsigned char  iv[EVP_MAX_IV_LENGTH];	/* working iv */
	unsigned char buf[EVP_MAX_BLOCK_LENGTH];/* saved partial block */
	int num;				/* used by cfb/ofb/ctr mode */

	void *app_data;		/* application stuff */
	int key_len;		/* May change for variable length cipher */
	unsigned long flags;	/* Various flags */
	void *cipher_data; /* per EVP data */
	int final_used;
	int block_mask;
	unsigned char final[EVP_MAX_BLOCK_LENGTH];/* possible final block */
} /* EVP_CIPHER_CTX */;


struct evp_cipher_st
{
	int nid;
	int block_size;
	int key_len;		/* Default value for variable length ciphers */
	int iv_len;
	unsigned long flags;	/* Various flags */
	int (*init)(EVP_CIPHER_CTX *ctx, const unsigned char *key,
			const unsigned char *iv, int enc);	/* init key */
	int (*do_cipher)(EVP_CIPHER_CTX *ctx, unsigned char *out,
			const unsigned char *in, size_t inl);/* encrypt/decrypt data */
	int (*cleanup)(EVP_CIPHER_CTX *); /* cleanup ctx */
	int ctx_size;		/* how big ctx->cipher_data needs to be */
	//	int (*set_asn1_parameters)(EVP_CIPHER_CTX *, ASN1_TYPE *); /* Populate a ASN1_TYPE with parameters */ commented - don't think it is needed
	//	int (*get_asn1_parameters)(EVP_CIPHER_CTX *, ASN1_TYPE *); /* Get parameters from a ASN1_TYPE */ commented - don't think it is needed
	int (*ctrl)(EVP_CIPHER_CTX *, int type, int arg, void *ptr); /* Miscellaneous operations */
	void *app_data;		/* Application data */
} /* EVP_CIPHER */;



typedef struct cuda_crypt_parameters_st {
	const unsigned char *in;	// host input buffer
	unsigned char *out;		// host output buffer
	size_t nbytes;			// number of bytes to be operated on
	EVP_CIPHER_CTX *ctx;		// EVP OpenSSL structure
	uint8_t *host_data;		// possible page-locked host memory
	uint64_t *d_in;		// Device memory (input)
	uint64_t *d_out;		// Device memory (output)
	uint32_t *d_key; // device key memory
} cuda_crypt_parameters;


// Split uint64_t into two uint32_t and convert each from BE to LE
#define nl2i(s,a,b)      a = ((s >> 24L) & 0x000000ff) | \
		((s >> 8L ) & 0x0000ff00) | \
		((s << 8L ) & 0x00ff0000) | \
		((s << 24L) & 0xff000000),   \
		b = ((s >> 56L) & 0x000000ff) | \
		((s >> 40L) & 0x0000ff00) | \
		((s >> 24L) & 0x00ff0000) | \
		((s >> 8L) & 0xff000000)

// Convert uint64_t endianness
#define flip64(a)	(a= \
		(a << 56) | \
		((a & 0xff00) << 40) | \
		((a & 0xff0000) << 24) | \
		((a & 0xff000000) << 8)  | \
		((a & 0xff00000000) >> 8)  | \
		((a & 0xff0000000000) >> 24) | \
		((a & 0xff000000000000) >> 40) | \
		(a >> 56))

#endif

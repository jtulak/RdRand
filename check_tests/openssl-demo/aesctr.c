/* vim: set expandtab cindent fdm=marker ts=2 sw=2: */

/*
gcc -mrdrnd -Wall -Wextra -O2 -o openssl-aesctr aesctr.c -lcrypto
*/

#include <assert.h>
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <openssl/evp.h>   
#include <x86intrin.h>

#define SIZEOF(a) ( sizeof (a) / sizeof (a[0]) )

/*
Key:
head -c16 /dev/random | xxd -p
*/

int hex2byte(const char *hex, size_t hex_length, unsigned char* byte, size_t byte_length) {
  size_t i;
  int rc;
  unsigned int n;
  assert(hex_length==2*byte_length);

  for (i=0; i<byte_length;++i) {
      rc = sscanf(hex, "%02x", &n);
      if ( rc != 1 ) {
        fprintf(stderr, "Error during sscanf\n");
        fprintf(stderr, "Read %d bytes\n",rc);
        fprintf(stderr, "%x", *byte);
        return 1;
      }
      *byte = (unsigned char) n;
      hex+=2;
      byte+=1;
  }
  return 0;  
}

void
dump_hex_byte_string (const unsigned char* data, const unsigned int size, const char* message) {
  unsigned int i;
  if (message)
	  fprintf(stderr,"%s",message);

  for (i=0; i<size; ++i) {
	fprintf(stderr,"%02x",data[i]);
  }
  fprintf(stderr,"\n");
}

void mem_dump(unsigned char *mem, unsigned int length) {
    unsigned i;
    for (i=0; length > i; i++){
        printf("%02x",mem[i]);
    }
    printf("\n");
}



int main() {

  unsigned char key[16];
  unsigned char nonce_counter[16]={0};
  char key_hex[32]="c96b8a45affc5c9050378dd32168c381";
  //char key_hex[32]="00000000000000000000000000000000";
  char nonce_hex[16]="41e31e41e3f8c26f"; //only upper 64-bits
  //char nonce_hex[16]="0000000000000000"; //only upper 64-bits

  unsigned char input [4096]={0};
  //Allow enough space in output buffer for additional block (padding)
  unsigned char output[4096 + EVP_MAX_BLOCK_LENGTH];

  hex2byte(key_hex, SIZEOF(key_hex), key, SIZEOF(key));
  dump_hex_byte_string(key, SIZEOF(key), "key:\t");
  hex2byte(nonce_hex, SIZEOF(nonce_hex), nonce_counter, SIZEOF(nonce_counter)/2); //Only upper 64-bits, lower bits are 0
  dump_hex_byte_string(nonce_counter, SIZEOF(nonce_counter), "nonce_counter:\t");
  
  int rc = 0;
  //unsigned int i;

  EVP_CIPHER_CTX en; 
  EVP_CIPHER_CTX_init( &en );
  if ( EVP_CipherInit_ex( &en, EVP_aes_128_ctr(), NULL, key, nonce_counter, 1 ) != 1 ) { // enc=1 => encryption, enc=0 => decryption
    perror("EVP_CipherInit_ex");
    return 1;
  };
  int in_len;
  int out_len;



  while(1) {
#if 0
    in_len = fread(input, 1, SIZEOF(input), stdin);
#else
    in_len=32;
    memset(input, -1, 4096); // set with "0xff...."
#endif
    if ( in_len <=0 ) {
      perror("fread");
			break;
    }


    if( EVP_EncryptUpdate(&en, output, &out_len, input, in_len) != 1 ) {
      perror("EVP_EncryptUpdate");
      return 1;
    };
    //dump_hex_byte_string(input, SIZEOF(input), "input:\t");
    //dump_hex_byte_string(nonce_counter, SIZEOF(nonce_counter), "nonce_counter:\t");

    //rc = fwrite(output, 1, out_len, stdout);
    
    printf("\n");
    mem_dump(output, 32);

    return 0;

  }

  if ( EVP_EncryptFinal_ex(&en, output, &out_len) != 1 ) {
    perror("EVP_CIPHER_CTX_cleanup");
    return 1;
  }
  rc = fwrite(output, 1, out_len, stdout);
  fprintf(stderr, "AES CTR has no padding. Expecting out_len to be 0. Got %d.\n", out_len);

  if ( EVP_CIPHER_CTX_cleanup(&en) != 1 ) {
    perror("EVP_CIPHER_CTX_cleanup");
    return 1;
  }
  return 0;
}

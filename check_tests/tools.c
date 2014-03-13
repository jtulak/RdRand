#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "tools.h"

void mem_dump(unsigned char *mem, unsigned int length) {
    unsigned i;
    for (i=0; length > i; i++){
        printf("%02x",mem[i]);
    }
    printf("\n");
} 
 
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

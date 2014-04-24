/* vim: set expandtab cindent fdm=marker ts=4 sw=4: */

#ifndef CHECK_TOOLS_H
#define  CHECK_TOOLS_H


#include <stdlib.h>
#define SIZEOF(a) ( sizeof (a) / sizeof (a[0]) )
void mem_dump(unsigned char *mem, unsigned int length);
int hex2byte(const char *hex, size_t hex_length, unsigned char* byte, size_t byte_length);
void dump_hex_byte_string (const unsigned char* data, const unsigned int size, const char* message);

void stdout_to_null();
void stdout_restore();

#endif // CHECK_TOOLS_H


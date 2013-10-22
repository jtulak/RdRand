RdRand
======

Library for RNG with Intel's RdRand usage.


Usage (of test)
---------------
cd tests
gcc -DHAVE_X86INTRIN_H -Wall -Wextra  -fopenmp -mrdrnd -I../src -O3 -o jh jh.c ../src/rdrand.c

Usage: ./jh [OUTPUT_FILE] [OPTIONS]
If no OUTPUT_FILE is specified, the program will print numbers to stdout.
OPTIONS
  --help	-h	Print this help
  --verbose	-v	Be verbose
  --numbers	-n	Print generated values as numbers, not raw, MAY IMPACT PERFORMANCE (default will print raw)
  --method	-m NAME	Will test only method NAME (default all)
  --threads	-t NUM	Will run the generator in NUM threads (default %u)
  --duration	-d NUM	Each tested method will run for NUM seconds (default %u)
  --repetition	-r NUM	All tests will be run for NUM times (default %u)
  --chunk-size	-c NUM	The size of the chunk generated at once as count of 64 bit numbers (default %u)
  
Possible methods to test:
  rdrand_get_bytes_retry
  rdrand_get_uint8_array_retry
  rdrand_get_uint16_array_retry
  rdrand_get_uint32_array_retry
  rdrand_get_uint64_array_retry
  rdrand16_step
  rdrand32_step
  rdrand64_step
  rdrand_get_uint16_retry
  rdrand_get_uint32_retry
  rdrand_get_uint64_retry
  rdrand_get_uint64_array_reseed_delay
  rdrand_get_uint64_array_reseed_skip
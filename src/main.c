/*
gcc -Wall -Wextra -fopenmp -mrdrnd -O2 -o main main.c -lrt
*/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>
#include <time.h>
#include <omp.h>

#define RETRY_LIMIT 10

int fill(uint64_t* buf, int size) {
  int j,k;
  int rc;

  for (j=0; j<size; ++j) {
    k = 0;
    do { 
      rc = _rdrand64_step ( &buf[j] );
      ++k;
    } while ( rc == 0 && k < RETRY_LIMIT);
    if ( rc == 0 ) return 0;
  }
  return 1;
}

int main(void) {
 uint64_t* buf;
 const int size=2048;
 const int threads=2;
 omp_set_num_threads(threads);
 int i, j, k;
 struct timespec t[2];
 double run_time, throughput;
 int thread_error;
 int rc;



 buf = malloc(threads * size * sizeof(uint64_t) );

 
 clock_gettime(CLOCK_REALTIME, &t[0]);
 while (1) {

         thread_error = 0;
         //#pragma omp parallel for reduction(+ : thread_error) schedule(static, 1024)
         #pragma omp parallel for
         for (i=0; i<threads; ++i) {
             rc = fill ( &buf[i*size], size );
           //if ( rc == 0 ) { ++thread_error; }
         }
         //if ( thread_error ) return 1;
         fwrite(buf, sizeof(uint64_t), threads * size , stdout);
  }

 clock_gettime(CLOCK_REALTIME, &t[1]);

 run_time = (double)(t[1].tv_sec) - (double)(t[0].tv_sec) + 
            ( (double)(t[1].tv_nsec) - (double)(t[0].tv_nsec) ) / 1.0E9;

  throughput = (double) (i) * size * sizeof(uint64_t) / run_time/1024.0/1024.0;

  fprintf(stderr, "Runtime %g, throughput %gMiB/s\n", run_time, throughput);
             


return 0;
}


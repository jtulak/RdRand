RdRand
======

Library for RNG with Intel's RdRand usage.

Still in development.




Usage (of test)
---------------
Test has its own Makefile in tests directory.

There are two ways how to run the the test: directly call `RdRand` binary, 
or use `make run` to run automated testing for different thread counts.

In case of the automated testing, the test parameters are set at the beginning 
of `_test-threads.sh` (threads count, single test run duration, ...). 
The result is saved to file `output_DATETIME.xml` and  if gnuplot is installed, 
then also graph is created.

**Example**

    $ cd tests
    $ make
    make: `all' is up to date.
    $ make run
    ./run-test.sh
    Will test from 1 to 200 threads.
    Currently testing 1 threads:  Mon Nov  4 16:50:00 CET 2013
    rdrand_get_bytes_retry 175.845 MiB/s 100.00 %
    Currently testing 2 threads:  Mon Nov  4 16:50:04 CET 2013
    rdrand_get_bytes_retry 336.609 MiB/s 100.00 %
    Currently testing 3 threads:  Mon Nov  4 16:50:08 CET 2013
    ...
    
    $ cat output_2013-11-04_16-50-00.xml
    <root>
    <test threads='1'>
    rdrand_get_bytes_retry 175.845 MiB/s 100.00 %
    </test>
    <test threads='2'>
    rdrand_get_bytes_retry 336.609 MiB/s 100.00 %
    </test>
    ...
    </root>

    
    
**Help output**

    Usage: ./RdRand [OUTPUT_FILE] [OPTIONS]
    If no OUTPUT_FILE is specified, the program will print numbers to stdout.
    
    OPTIONS
      --help       -h      Print this help
      --verbose    -v      Be verbose
      --print      -p      Do really print the generated numbers, instead just measure the speed.
      --numbers    -n      Print generated values as numbers, not raw, MAY IMPACT PERFORMANCE (default will print raw)
      --method     -m NAME Will test only method NAME (default all)
      --threads    -t NUM  Will run the generator in NUM threads (default 2)
      --duration   -d NUM  Each tested method will run for NUM seconds (default 3)
      --repetition -r NUM  All tests will be run for NUM times (default 2)
      --chunk-size -c NUM  The size of the chunk generated at once as count of 64 bit numbers (default 2048)

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

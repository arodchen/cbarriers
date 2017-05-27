cbarriers
=========

Description
-----------
**cbarriers** is a framework for evaluating different barrier synchronization
algorithms. It was written to evaluate hybrid barrier with other known
barrier synchronization algorithms. You can find more details in the paper:

*"Effective Barrier Synchronization on Intel Xeon Phi Coprocessor",
Andrey Rodchenko, Andy Nisbet, Antoniu Pop, and Mikel Lujan, Euro-Par 2015*. [[paper]](https://www.research.manchester.ac.uk/portal/files/48218868/FULL_TEXT.PDF) [[slides]](http://apt.cs.manchester.ac.uk/people/arodchenko/Effective_Barrier_Synchronization_on_Intel_Xeon_Phi_Coprocessor_Slides.pdf)

##### Acknowledgements
This work is supported by EPSRC grants DOME EP/J016330/1, PAMELA EP/K008730/1
and EP/M004880/1. A. Rodchenko is funded by a Microsoft Research PhD
Scholarship, A. Pop is funded by a Royal Academy of Engineering Research
Fellowship and M. Lujan is funded by a Royal Society University Research
Fellowship.

Dependencies
-----
`gcc, r-base, r-cran-gplots, data.table package in R`

Usage
-----
`make build` - builds barrier evaluation framework

`make test ` - runs tests

`make db   ` - creates data base needed for plotting

`make plot ` - plots charts in pdf

`make all  ` - does all mentioned above

`make help ` - prints help message

Recipes
--------
The following command runs tests on 60-Core Intel Xeon Phi:

`make test TEST_RUNNER='ssh mic0 "ulimit -s unlimited; export KMP_LIBRARY=turnaround; export KMP_BLOCKTIME=infinite;' TEST_RUNNER_PASS_ARG_END='"' HOST_NAME=mic0 TEST_RUNNER_INIT='scp -r bin mic0:' TEST_RUNNER_FINI='ssh mic0 "rm -rf bin"' CC=icc CPUS_NUM=232 EXTRACFLAGS="-mmic" BARRIERS_NUM=10000 ARCH=mic TOPOLOGY_NUMA_AWARE_ALLOC=no THREADS_MIN_NUM=8 THREADS_INC=+=8 RADIX_INC=*=2 EXPERIMENTS_NUM=10`

#!/usr/bin/make -f
# Copyright 2015 Andrey Rodchenko, School of Computer Science, The University of Manchester
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

REDIRECT_APPEND = >>
REDIRECT = >
REDIRECT_ERR = 2>
ECHO = echo
RM = rm -rf
HGCLONE = hg clone
CD = cd
MAKE = make
MKDIR = mkdir -p
TEST_FILE = test -e
CAT = cat
HOSTNAME = hostname
LSCPU = lscpu
RSCRIPT = Rscript --vanilla
CP = cp
FIND = find
UP_F = $(shell $(ECHO) $(1) | tr a-z A-Z)

HOST_NAME ?= $(shell $(HOSTNAME))

ifeq ($(CC),icc)
F77 = ifort
AS = icc -c -mmic
else
F77 = gfortran
AS = as
endif

EXTRACFLAGS ?=
EXTRALDFLAGS ?=
TEST_RUNNER ?=
TEST_RUNNER_PASS_ARG_BEG ?=
TEST_RUNNER_PASS_ARG_END ?=
TEST_RUNNER_INIT ?=
TEST_RUNNER_FINI ?=

PRINT_SYNCH_UNSYNCH_PHASE_TIME ?= yes
ifeq ($(PRINT_SYNCH_UNSYNCH_PHASE_TIME),yes)
TABLE_HEADER =Hostname,Architecture,Experiment Number,Benchmark,Barrier,Radix,Spinning,Threads Number,Affinity,Nanoseconds per Synchronized Phase,Nanoseconds per Unsynchronized Phase,Nanoseconds per Barrier
CFLAGS += -DPRINT_SYNCH_UNSYNCH_PHASE_TIME
else
TABLE_HEADER =Hostname,Architecture,Experiment Number,Benchmark,Barrier,Radix,Spinning,Threads Number,Affinity,Nanoseconds per Barrier
endif
CFLAGS += -DTABLE_HEADER='"$(TABLE_HEADER)"'

CPU_MAP_PRIORITY_DELTA ?= 1
CFLAGS += -DCPU_MAP_PRIORITY_DELTA=$(CPU_MAP_PRIORITY_DELTA)

THREADS_INC ?= +=1
CFLAGS += -DTHREADS_INC=$(THREADS_INC)

RADIX_INC ?= +=1
CFLAGS += -DRADIX_INC=$(RADIX_INC)

CPUS_NUM ?= $(shell $(LSCPU) -p | grep -c -P '^\d')
CFLAGS += -DCPUS_NUM=$(CPUS_NUM)

ifeq ($(origin THREADS_SURPLUS), undefined)
    ifeq ($(origin THREADS_MAX_NUM), undefined)
        THREADS_SURPLUS = 0 
        THREADS_MAX_NUM = $(shell $(ECHO) $(CPUS_NUM) + $(THREADS_SURPLUS) | bc)
    else
        THREADS_SURPLUS = $(shell $(ECHO) $(THREADS_MAX_NUM) - $(CPUS_NUM) | bc)
    endif
else
    THREADS_MAX_NUM = $(shell $(ECHO) $(CPUS_NUM) + $(THREADS_SURPLUS) | bc)
endif

THREADS_MIN_NUM ?= 1
CFLAGS += -DTHREADS_MIN_NUM=$(THREADS_MIN_NUM)

RADIX_MAX ?= THREADS_MAX_NUM
CFLAGS += -DRADIX_MAX=$(RADIX_MAX)

CEIL_LOG2_THREADS_MAX_NUM = $(shell printf %f `echo '(l($(THREADS_MAX_NUM) * 2 - l(2))/l(2))' | bc -l` | grep -P -o "^\d")
ifeq ($(CEIL_LOG2_THREADS_MAX_NUM),0)
CEIL_LOG2_THREADS_MAX_NUM = 1
endif
CFLAGS += -DTHREADS_MAX_NUM=$(THREADS_MAX_NUM)
CFLAGS += -DCEIL_LOG2_THREADS_MAX_NUM=$(CEIL_LOG2_THREADS_MAX_NUM)

BARRIERS_NUM ?= 1000000
CFLAGS += -DBARRIERS_NUM=$(BARRIERS_NUM)

EXPERIMENTS_NUM ?= 10
CFLAGS += -DEXPERIMENTS_NUM=$(EXPERIMENTS_NUM)

DEF_INTERPOLATE_RADIX = 0
CFLAGS += -DDEF_INTERPOLATE_RADIX=$(DEF_INTERPOLATE_RADIX)
ifeq ($(DEF_INTERPOLATE_RADIX), 1)
    INTERPOLATE_RADIX ?= yes
else
    INTERPOLATE_RADIX ?= no
endif

DEF_TOPOLOGY_AWARE_MAPPING = 1
CFLAGS += -DDEF_TOPOLOGY_AWARE_MAPPING=$(DEF_TOPOLOGY_AWARE_MAPPING)
ifeq ($(DEF_TOPOLOGY_AWARE_MAPPING), 1)
    TOPOLOGY_AWARE_MAPPING ?= yes
else
    TOPOLOGY_AWARE_MAPPING ?= no
endif

DEF_TOPOLOGY_NUMA_AWARE_ALLOC = 1
CFLAGS += -DDEF_TOPOLOGY_NUMA_AWARE_ALLOC=$(DEF_TOPOLOGY_NUMA_AWARE_ALLOC)
ifeq ($(DEF_TOPOLOGY_NUMA_AWARE_ALLOC), 1)
    TOPOLOGY_NUMA_AWARE_ALLOC ?= yes
else
    TOPOLOGY_NUMA_AWARE_ALLOC ?= no
endif


DEF_USER_DEFINED_ACTIVE_PUS_SELECTION = 0
CFLAGS += -DDEF_USER_DEFINED_ACTIVE_PUS_SELECTION=$(DEF_USER_DEFINED_ACTIVE_PUS_SELECTION)
ifeq ($(DEF_USER_DEFINED_ACTIVE_PUS_SELECTION), 1)
    USER_DEFINED_ACTIVE_PUS_SELECTION ?= yes
else
    USER_DEFINED_ACTIVE_PUS_SELECTION ?= no
endif

R_OPT_IGNORE_BARRIERS = ignoreBarriers=$(CHARTS_IGNORE_BARRIERS)
R_OPT_SUR_ONLY_SPINNINGS = surOnlySpinnings=$(CHARTS_SUR_ONLY_SPINNINGS)
R_OPT_INTERPOLATE_RADIX = interpolateRadix=$(INTERPOLATE_RADIX)

#FIXME: Cache coherency line size should be defined more precisely depending on MP architecture.
#       Default value was chosen due to X86 ISA 8.10.6.7 Place Locks and Semaphores in Aligned,
#       128-Byte Blocks of Memory
CCL_SIZE ?= 128
CFLAGS += -DCCL_SIZE=$(CCL_SIZE)

ARCH ?= $(shell arch)
DARCH = $(call UP_F, $(ARCH))
CFLAGS += -DARCH_$(DARCH)

DVFSOFF_FREQNO = 1
ifeq ($(DARCH),X86_64)
DVFSOFF_FREQNO = 2
endif

CFLAGS += -O3 $(EXTRACFLAGS)
LDFLAGS = -lpthread -lrt -lm $(EXTRALDFLAGS)

PATH_SEPARATOR = /
CFG_SEPARATOR = __
HEADER_EXT = .h
ERR_EXT = .err
BIN_EXT = .exe
TEST_REP_EXT = .csv
CFG_EXT = .cfg
BIN_DIR = bin
ROOT_TEST_REP_DIR = testrep

TEST_REP_DIR = $(ROOT_TEST_REP_DIR)$(PATH_SEPARATOR)$(HOST_NAME)
TEST_DB_FILENAME = test.db
TEST_DB_DIR = testdb
CHARTS_DIR = charts
GENCHARTS_SCRIPT = scripts$(PATH_SEPARATOR)genchart.r

OMP_IMPL ?= gnu
CFLAGS += -DOMP_$(call UP_F,$(OMP_IMPL))

START_ID = 0
CURR_ID_FILE_NAME = .curr_exp_id

CURR_ID = $(shell $(TEST_FILE) $(TEST_REP_DIR)$(PATH_SEPARATOR)$(CURR_ID_FILE_NAME) \
		         && $(CAT) $(TEST_REP_DIR)$(PATH_SEPARATOR)$(CURR_ID_FILE_NAME))
ifeq ($(CURR_ID),)
CURR_ID = 0
endif

NEXT_ID = $(shell $(ECHO) $(CURR_ID) + 1 | bc)

ifeq ($(TOPOLOGY_NUMA_AWARE_ALLOC),yes)
LIBNUMA_LINK_OPTION = -lnuma
LIBNUMA_INSTALLED = $(shell which numastat > /dev/null;echo "$$?")
ifeq ($(LIBNUMA_INSTALLED),0)
LDFLAGS += $(LIBNUMA_LINK_OPTION)
CFLAGS += -DLIB_NUMA
endif
endif

BARRIERS = \
	DSMNH_BARRIER \
	DSMN_BARRIER \
	STNGS_BARRIER \
	STNLS_BARRIER \
	DTNGS_BARRIER \
	DTNLS_BARRIER \
	CTRGS_BARRIER \
	CTRLS_BARRIER \
	SR_BARRIER \
	OMP_BARRIER \
    PTHREAD_BARRIER

SPINNINGS = \
	SPIN_SPINNING \
	PAUSE_SPINNING \
	PTYIELD_SPINNING \
	HWYIELD_SPINNING \
	WFE_SPINNING

BENCHMARKS = \
	PURE_BENCHMARK \
	LDIMBL_BENCHMARK \
	SANITY_BENCHMARK \
	NBODY_BENCHMARK

SHORTEN_CFG_NAME_F = $(subst _BARRIER,, $(subst _SPINNING,, $(subst _BENCHMARK,, $(1))))

CFG_NAME_T = $(1)$(CFG_SEPARATOR)$(2)$(CFG_SEPARATOR)$(3)

BIN_NAME_F = $(call SHORTEN_CFG_NAME_F,$(BIN_DIR)$(PATH_SEPARATOR)$(CFG_NAME_T)$(BIN_EXT))

TEST_REP_NAME_F = $(call SHORTEN_CFG_NAME_F,$(TEST_REP_DIR)$(PATH_SEPARATOR)$(CFG_NAME_T)$(CFG_SEPARATOR)$(CURR_ID)$(TEST_REP_EXT))

TARGETS_BIN = \
$(foreach cbarrier, $(BARRIERS), \
	$(foreach spinning, $(SPINNINGS), \
		$(foreach benchmark, $(BENCHMARKS), \
			$(call BIN_NAME_F,$(cbarrier),$(spinning),$(benchmark)))))

TARGETS_TEST_REP = \
$(foreach cbarrier, $(BARRIERS), \
	$(foreach spinning, $(SPINNINGS), \
		$(foreach benchmark, $(BENCHMARKS), \
			$(call TEST_REP_NAME_F,$(cbarrier),$(spinning),$(benchmark)))))

all : test

build: $(TARGETS_BIN)

$(TARGETS_BIN) : BARRIER_SPINNING_BENCHMARK = $(subst $(CFG_SEPARATOR), ,$(subst $(BIN_EXT),,$(subst $(BIN_DIR)$(PATH_SEPARATOR),,$@)))
$(TARGETS_BIN) : DBARRIER = -D$(word 1,$(BARRIER_SPINNING_BENCHMARK))_BARRIER
$(TARGETS_BIN) : DSPINNING = -D$(word 2,$(BARRIER_SPINNING_BENCHMARK))_SPINNING
$(TARGETS_BIN) : DBENCHMARK = -D$(word 3,$(BARRIER_SPINNING_BENCHMARK))_BENCHMARK
$(TARGETS_BIN) : CFLAGS += $(DBARRIER) $(DSPINNING) $(DBENCHMARK)
$(TARGETS_BIN) : CFLAGS += $(subst OMP,-fopenmp,$(findstring OMP, $(word 1,$(BARRIER_SPINNING_BENCHMARK))))
$(TARGETS_BIN) : cbarriers/barrier.c cbarriers/barrier.h | $(BIN_DIR)
	$(CC) $(CFLAGS)	cbarriers/barrier.c $(LDFLAGS) -o $@

$(BIN_DIR) : | $(TEST_REP_DIR)
	$(MKDIR) $(BIN_DIR)
	$(ECHO) $(START_ID) $(REDIRECT) $(TEST_REP_DIR)$(PATH_SEPARATOR)$(CURR_ID_FILE_NAME)

test : build $(TARGETS_TEST_REP)
	$(ECHO) $(NEXT_ID) $(REDIRECT) $(TEST_REP_DIR)$(PATH_SEPARATOR)$(CURR_ID_FILE_NAME)
	$(ECHO) "$(LSCPU):" $(REDIRECT) $(TEST_REP_DIR)$(PATH_SEPARATOR)$(HOST_NAME)$(CFG_SEPARATOR)$(ARCH)$(CFG_SEPARATOR)$(CURR_ID)$(CFG_EXT)
	$(LSCPU) $(REDIRECT_APPEND) $(TEST_REP_DIR)$(PATH_SEPARATOR)$(HOST_NAME)$(CFG_SEPARATOR)$(ARCH)$(CFG_SEPARATOR)$(CURR_ID)$(CFG_EXT)
	$(TEST_RUNNER_FINI)

$(TARGETS_TEST_REP) : TEST_BIN = $(subst $(CFG_SEPARATOR)$(CURR_ID),,$(subst $(TEST_REP_EXT),$(BIN_EXT),$(subst $(TEST_REP_DIR),$(BIN_DIR),$@)))
$(TARGETS_TEST_REP) : | $(TEST_REP_DIR) testrunnerinit
	$(TEST_RUNNER) $(TEST_BIN) $(TEST_RUNNER_PASS_ARG_BEG) $(HOST_NAME) $(CURR_ID) $(INTERPOLATE_RADIX) $(TOPOLOGY_AWARE_MAPPING) $(TOPOLOGY_NUMA_AWARE_ALLOC) $(USER_DEFINED_ACTIVE_PUS_SELECTION) $(TEST_RUNNER_PASS_ARG_END) $(REDIRECT) $@ $(REDIRECT_ERR)$(subst $(TEST_REP_EXT),$(ERR_EXT),$@)

testrunnerinit:
	$(TEST_RUNNER_INIT)

$(TEST_REP_DIR) :
	$(MKDIR) $(TEST_REP_DIR)

clean : cleanbuild cleantest 

cleanbuild :
	$(RM) $(BIN_DIR)

cleantest :
	$(RM) $(ROOT_TEST_REP_DIR)

$(TEST_DB_DIR)$(PATH_SEPARATOR)$(TEST_DB_FILENAME) :
	$(MKDIR) $(TEST_DB_DIR)
	echo "$(TABLE_HEADER)" >> $(TEST_DB_DIR)$(PATH_SEPARATOR)$(TEST_DB_FILENAME)

db: | $(TEST_DB_DIR)$(PATH_SEPARATOR)$(TEST_DB_FILENAME)
	find $(ROOT_TEST_REP_DIR) -name *$(CFG_EXT) | xargs -i -t $(CP) {} $(TEST_DB_DIR)
	find $(ROOT_TEST_REP_DIR) -name *$(TEST_REP_EXT) | xargs -i -t sed -n '2,$${p;}' {} $(REDIRECT_APPEND) $(TEST_DB_DIR)$(PATH_SEPARATOR)$(TEST_DB_FILENAME)

cleandb:
	$(RM) $(TEST_DB_DIR)

$(CHARTS_DIR):
	$(MKDIR) $(CHARTS_DIR)

plot: | $(CHARTS_DIR)
	$(RSCRIPT) $(GENCHARTS_SCRIPT) $(TEST_DB_DIR) $(CHARTS_DIR) $(THREADS_SURPLUS) $(R_OPT_IGNORE_BARRIERS) $(R_OPT_SUR_ONLY_SPINNINGS) $(R_OPT_INTERPOLATE_RADIX)

cleanplot:
	$(RM) $(CHARTS_DIR)

all: build test db plot

cleanall: cleanbuild cleantest cleandb cleanplot

help :
	@echo "Usage: make <target> [args]"
	@echo ""
	@echo "Availiable targets: help"
	@echo "                    build cleanbuild"
	@echo "                    test cleantest"
	@echo "                    db cleandb"
	@echo "                    plot cleanplot"
	@echo "                    all cleanall"
	@echo ""
	@echo ""
	@echo "Availiable args [argname={valid values} (default) - description]:"
	@echo ""                                 
	@echo "  EXTRACFLAGS={...} ( ) - extra CFLAGS"
	@echo "  EXTRALDFLAGS={...} ( ) - extra LDFLAGS"
	@echo ""                                 
	@echo "  TEST_RUNNER={...} ( ) - test runner"
	@echo "  TEST_RUNNER_PASS_ARG_BEG={...} ( ) - option to begin passing args to test runner"
	@echo "  TEST_RUNNER_PASS_ARG_END={...} ( ) - option to end passing args to test runner"
	@echo "  TEST_RUNNER_INIT={...} ( ) - option to initialize test runner"
	@echo "  TEST_RUNNER_FINI={...} ( ) - option to finalize test runner"
	@echo ""                                 
	@echo "  CPUS_NUM={1..N} (shell nproc) - number of availiable CPUs"
	@echo "  THREADS_SURPLUS={(1-CPUS_NUM)..N} (0) - difference between the number of threads and cpus"
	@echo "  THREADS_MAX_NUM={1..N} (CPUS_NUM + THREADS_SURPLUS) - maximum number of threads"
	@echo "                                                        (THREADS_SURPLUS has priority over THREADS_MAX_NUM if boths are defined)"
	@echo "  THREADS_MIN_NUM={1..THREADS_MAX_NUM} (1) - minimum number of threads"
	@echo "  BARRIERS_NUM={1..N} (1000000) - number of barriers in experiment"
	@echo "  EXPERIMENTS_NUM={1..N} (10) - number of experiments"
	@echo "  CCL_SIZE={1..N} (128) - cache coherency line size"
	@echo "  ARCH={x86_64|armv7l} (shell arch) - target architecture"
	@echo "  HOST_NAME={...} (shell hostname) - target hostname"
	@echo "  CPU_MAP_PRIORITY_DELTA={1..CPUS_NUM} (1) - priority delta for mapping threads to cpus"
	@echo "  THREADS_INC={+=1,*=2,...} (+= 1) - compound assignment operator for increment of number of threads"
	@echo "  RADIX_INC={+=1,*=2,...} (+= 1) - compound assignment operator for increment of radix"
	@echo "  RADIX_MAX={2..THREADS_MAX_NUM} (THREADS_MAX_NUM) - maximum radix"
	@echo "  OMP_IMPL={gnu,intel} (gnu) - omp library implementation"
	@echo "  INTERPOLATE_RADIX={yes,no} (no) - when number of threads participating in barrier is less than radix then the result is taken"
	@echo "                                    from experiment when number of threads participating in a barrier is equal to radix"
	@echo "  PRINT_SYNCH_UNSYNCH_PHASE_TIME={yes,no} (yes) - print synchronized and unsynchronized phase execution time,"
	@echo "                                                  where phase is an interval by two adjacent barriers"
	@echo "  TOPOLOGY_AWARE_MAPPING={yes,no} (yes) - use topology information while mapping threads to nodes of the barrier"
	@echo "  TOPOLOGY_NUMA_AWARE_ALLOC={yes,no} (yes) - use topology information while allocating memory for barriers on NUMA machines"
	@echo "  USER_DEFINED_ACTIVE_PUS_SELECTION={yes,no} (no) - user defined active PUs selection controlled by CPU_MAP_PRIORITY_DELTA"
	@echo "  CHARTS_IGNORE_BARRIERS={sr,...,pthread} () - comma-separated list of barriers to ignore on charts"
	@echo "  CHARTS_SUR_ONLY_SPINNINGS={ptyield,...,hwyield} () - comma-separated list of spinning to be used on surplused charts"
	@echo ""

.PHONY: help build cleanbuild test cleantest db cleandb plot cleanplot all cleanall 
.NOTPARALLEL:


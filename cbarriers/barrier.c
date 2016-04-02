/*
 * Copyright 2015 Andrey Rodchenko, School of Computer Science, The University of Manchester
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifdef ARCH_MIC
#   include <immintrin.h>
#   include <zmmintrin.h>
#endif
#ifdef ARCH_X86_64
#   include <emmintrin.h>
#endif
#ifdef OMP_BARRIER
#   include <omp.h>
#endif
#ifdef LIB_NUMA
#   include <numa.h>
#endif
#ifdef NBODY_BENCHMARK
#   include <math.h>
#endif
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <sys/times.h>
#include "barrier.h"

const char ARCH_STR [ ] =
#ifdef ARCH_MIC
    "mic";
#endif
#ifdef ARCH_X86_64
    "x86_64";
#endif
#ifdef ARCH_ARMV7L
    "armv7l";
#endif

const char BENCH_STR [ ] =
#ifdef NBODY_BENCHMARK
    "nbody";
#endif
#ifdef SANITY_BENCHMARK
    "sanity";
#endif
#ifdef PURE_BENCHMARK
    "pure";
#endif
#ifdef LDIMBL_BENCHMARK
    "ldimbl";
#endif
#ifdef TMPL_BENCHMARK
    "tmpl";
#endif

const char * BARRIER_STR =
#ifdef PTHREAD_BARRIER
    "pthread";
#endif
#ifdef SR_BARRIER
    "sr";
#endif
#ifdef CTRGS_BARRIER
    "ctrgs";
#endif
#ifdef CTRLS_BARRIER
    "ctrls";
#endif
#ifdef DSMN_BARRIER
    "dsmn";
#endif
#ifdef STNGS_BARRIER
    "stngs";
#endif
#ifdef STNLS_BARRIER
    "stnls";
#endif
#ifdef DTNGS_BARRIER
    "dtngs";
#endif
#ifdef DTNLS_BARRIER
    "dtnls";
#endif
#ifdef OMP_BARRIER
    "omp";
#endif
#ifdef DSMNH_BARRIER
    "dsmnh";
#endif

const char SPINNING_STR [ ] =
#ifdef SPIN_SPINNING
    "spin";
#endif
#ifdef HWYIELD_SPINNING
    "hwyield";
#endif
#ifdef PTYIELD_SPINNING
    "ptyield";
#endif
#ifdef PAUSE_SPINNING
    "pause";
#endif
#ifdef WFE_SPINNING
    "wfe";
#endif

static const char * HOSTNAME_STR;

static const char * EXP_ID_STR;

static bool INTERPOLATE_RADIX;

static bool TOPOLOGY_AWARE_MAPPING;

static bool USER_DEFINED_ACTIVE_PUS_SELECTION;

static bool TOPOLOGY_NUMA_AWARE_ALLOC;

static const char * bar_MachineIdToHostnameMap [ MACHINES_MAX_NUM ] =
    { 
    "alpha",
    "beta",
    "gamma", 
    "delta",
    "epsilpon",
    "zeta",
    "eta",
    "mic0",
    "omega"
    };

static const char * tpl_TopologyLevelName [ TPL_NODES_ALL_NUM ] =
    { 
    "Machine", 
    "Socket", 
    "NumaNode",
    "Core",
    "PU",
    "Undefined",
    };


static int bar_ThreadIdToOsIdMap [ THREADS_MAX_NUM ] [ THREADS_MAX_NUM ];

static tpl_PU_t * bar_ThreadIdToPUMap [ THREADS_MAX_NUM ];

static tpl_PU_t * bar_OsIdToPUMap [ PUS_PER_MACHINE_MAX_NUM ];

/**
 * PUs topology descriptions set.
 */
static tpl_PU_t tpl_PUDescriptionsSet [ MACHINES_MAX_NUM ] 
                                      [ SOCKETS_PER_MACHINE_MAX_NUM ] 
                                      [ NUMANODES_PER_SOCKET_MAX_NUM ]
                                      [ CORES_PER_NUMANODE_MAX_NUM ] 
                                      [ PUS_PER_CORE_MAX_NUM ] = 
    {
    MACHINE_TOPOLOGY_INIT( SO( NU( CO( PU( 0)), 
                                   CO( PU( 1)) ))),
    MACHINE_TOPOLOGY_INIT( SO( NU( CO( PU( 0), PU( 1)), 
                                   CO( PU( 2), PU( 3)) ))),
    MACHINE_TOPOLOGY_INIT( SO( NU( CO( PU( 0), PU( 4)), 
                                   CO( PU( 1), PU( 5)), 
                                   CO( PU( 2), PU( 6)), 
                                   CO( PU( 3), PU( 7)) ))),
    MACHINE_TOPOLOGY_INIT( SO( NU( CO( PU( 0), PU( 4)), 
                                   CO( PU( 1), PU( 5)), 
                                   CO( PU( 2), PU( 6)), 
                                   CO( PU( 3), PU( 7)) ))),
    MACHINE_TOPOLOGY_INIT( SO( NU( CO( PU( 0), PU( 12)), 
                                   CO( PU( 1), PU( 13)), 
                                   CO( PU( 2), PU( 14)), 
                                   CO( PU( 3), PU( 15)), 
                                   CO( PU( 4), PU( 16)), 
                                   CO( PU( 5), PU( 17)) )),
                           SO( NU( CO( PU( 6), PU( 18)), 
                                   CO( PU( 7), PU( 19)), 
                                   CO( PU( 8), PU( 20)), 
                                   CO( PU( 9), PU( 21)), 
                                   CO( PU( 10), PU( 22)), 
                                   CO( PU( 11), PU( 23)) ))),
    MACHINE_TOPOLOGY_INIT( SO( NU( CO( PU( 0), 
                                       PU( 4)), 
                                   CO( PU( 8), 
                                       PU( 12)) ), 
                               NU( CO( PU( 16), 
                                       PU( 20)), 
                                   CO( PU( 24),
                                       PU( 28)), )),
                           SO( NU( CO( PU( 1),
                                       PU( 5)),
                                   CO( PU( 9),
                                       PU( 13)) ),
                               NU( CO( PU( 17),
                                       PU( 21)),
                                   CO( PU( 25),
                                       PU( 29)), )),
                           SO( NU( CO( PU( 2), 
                                       PU( 6)), 
                                   CO( PU( 10), 
                                       PU( 14)) ), 
                               NU( CO( PU( 18), 
                                       PU( 22)),
                                   CO( PU( 26), 
                                       PU( 30)), )),
                           SO( NU( CO( PU( 3),
                                       PU( 7)),
                                   CO( PU( 11),
                                       PU( 15)) ),
                               NU( CO( PU( 19),
                                       PU( 23)),
                                   CO( PU( 27),
                                       PU( 31)), ))),
    MACHINE_TOPOLOGY_INIT( SO( NU( CO( PU( 0), 
                                       PU( 4)), 
                                   CO( PU( 8), 
                                       PU( 12)), 
                                   CO( PU( 16), 
                                       PU( 20)), 
                                   CO( PU( 24), 
                                       PU( 28)), ),
                               NU( CO( PU( 32),
                                       PU( 36)),
                                   CO( PU( 40),
                                       PU( 44)),
                                   CO( PU( 48),
                                       PU( 52)),
                                   CO( PU( 56),
                                       PU( 60)), )),
                           SO( NU( CO( PU( 1), 
                                       PU( 5)), 
                                   CO( PU( 9), 
                                       PU( 13)), 
                                   CO( PU( 17), 
                                       PU( 21)), 
                                   CO( PU( 25), 
                                       PU( 29)), ),
                               NU( CO( PU( 33),
                                       PU( 37)),
                                   CO( PU( 41),
                                       PU( 45)),
                                   CO( PU( 49),
                                       PU( 53)),
                                   CO( PU( 57),
                                       PU( 61)), )),
                           SO( NU( CO( PU( 2), 
                                       PU( 6)), 
                                   CO( PU( 10),
                                       PU( 14)), 
                                   CO( PU( 18),
                                       PU( 22)),
                                   CO( PU( 26), 
                                       PU( 30)), ),
                               NU( CO( PU( 34),
                                       PU( 38)),
                                   CO( PU( 42),
                                       PU( 46)),
                                   CO( PU( 50),
                                       PU( 54)),
                                   CO( PU( 58),
                                       PU( 62)), )),
                           SO( NU( CO( PU( 3),
                                       PU( 7)),
                                   CO( PU( 11),
                                       PU( 15)),
                                   CO( PU( 19), 
                                       PU( 23)),
                                   CO( PU( 27), 
                                       PU( 31)), ),
                               NU( CO( PU( 35),
                                       PU( 39)),
                                   CO( PU( 43),
                                       PU( 47)),
                                   CO( PU( 51),
                                       PU( 55)),
                                   CO( PU( 59),
                                       PU( 63)), ))),
    MACHINE_TOPOLOGY_INIT( SO( NU( CO( PU( 0), PU( 237), PU( 238), PU( 239)),
                                   CO( PU( 1), PU( 2), PU( 3), PU( 4)), 
                                   CO( PU( 5), PU( 6), PU( 7), PU( 8)), 
                                   CO( PU( 9), PU( 10), PU( 11), PU( 12)), 
                                   CO( PU( 13), PU( 14), PU( 15), PU( 16)), 
                                   CO( PU( 17), PU( 18), PU( 19), PU( 20)), 
                                   CO( PU( 21), PU( 22), PU( 23), PU( 24)), 
                                   CO( PU( 25), PU( 26), PU( 27), PU( 28)), 
                                   CO( PU( 29), PU( 30), PU( 31), PU( 32)), 
                                   CO( PU( 33), PU( 34), PU( 35), PU( 36)), 
                                   CO( PU( 37), PU( 38), PU( 39), PU( 40)), 
                                   CO( PU( 41), PU( 42), PU( 43), PU( 44)), 
                                   CO( PU( 45), PU( 46), PU( 47), PU( 48)), 
                                   CO( PU( 49), PU( 50), PU( 51), PU( 52)), 
                                   CO( PU( 53), PU( 54), PU( 55), PU( 56)), 
                                   CO( PU( 57), PU( 58), PU( 59), PU( 60)), 
                                   CO( PU( 61), PU( 62), PU( 63), PU( 64)), 
                                   CO( PU( 65), PU( 66), PU( 67), PU( 68)), 
                                   CO( PU( 69), PU( 70), PU( 71), PU( 72)), 
                                   CO( PU( 73), PU( 74), PU( 75), PU( 76)), 
                                   CO( PU( 77), PU( 78), PU( 79), PU( 80)), 
                                   CO( PU( 81), PU( 82), PU( 83), PU( 84)), 
                                   CO( PU( 85), PU( 86), PU( 87), PU( 88)), 
                                   CO( PU( 89), PU( 90), PU( 91), PU( 92)), 
                                   CO( PU( 93), PU( 94), PU( 95), PU( 96)), 
                                   CO( PU( 97), PU( 98), PU( 99), PU( 100)), 
                                   CO( PU( 101), PU( 102), PU( 103), PU( 104)), 
                                   CO( PU( 105), PU( 106), PU( 107), PU( 108)), 
                                   CO( PU( 109), PU( 110), PU( 111), PU( 112)), 
                                   CO( PU( 113), PU( 114), PU( 115), PU( 116)), 
                                   CO( PU( 117), PU( 118), PU( 119), PU( 120)), 
                                   CO( PU( 121), PU( 122), PU( 123), PU( 124)), 
                                   CO( PU( 125), PU( 126), PU( 127), PU( 128)), 
                                   CO( PU( 129), PU( 130), PU( 131), PU( 132)), 
                                   CO( PU( 133), PU( 134), PU( 135), PU( 136)), 
                                   CO( PU( 137), PU( 138), PU( 139), PU( 140)), 
                                   CO( PU( 141), PU( 142), PU( 143), PU( 144)), 
                                   CO( PU( 145), PU( 146), PU( 147), PU( 148)), 
                                   CO( PU( 149), PU( 150), PU( 151), PU( 152)), 
                                   CO( PU( 153), PU( 154), PU( 155), PU( 156)), 
                                   CO( PU( 157), PU( 158), PU( 159), PU( 160)), 
                                   CO( PU( 161), PU( 162), PU( 163), PU( 164)), 
                                   CO( PU( 165), PU( 166), PU( 167), PU( 168)), 
                                   CO( PU( 169), PU( 170), PU( 171), PU( 172)), 
                                   CO( PU( 173), PU( 174), PU( 175), PU( 176)), 
                                   CO( PU( 177), PU( 178), PU( 179), PU( 180)), 
                                   CO( PU( 181), PU( 182), PU( 183), PU( 184)), 
                                   CO( PU( 185), PU( 186), PU( 187), PU( 188)), 
                                   CO( PU( 189), PU( 190), PU( 191), PU( 192)), 
                                   CO( PU( 193), PU( 194), PU( 195), PU( 196)), 
                                   CO( PU( 197), PU( 198), PU( 199), PU( 200)), 
                                   CO( PU( 201), PU( 202), PU( 203), PU( 204)), 
                                   CO( PU( 205), PU( 206), PU( 207), PU( 208)), 
                                   CO( PU( 209), PU( 210), PU( 211), PU( 212)), 
                                   CO( PU( 213), PU( 214), PU( 215), PU( 216)), 
                                   CO( PU( 217), PU( 218), PU( 219), PU( 220)), 
                                   CO( PU( 221), PU( 222), PU( 223), PU( 224)), 
                                   CO( PU( 225), PU( 226), PU( 227), PU( 228)), 
                                   CO( PU( 229), PU( 230), PU( 231), PU( 232)), 
                                   CO( PU( 233), PU( 234), PU( 235), PU( 236)), ))),
    MACHINE_TOPOLOGY_INIT( SO( NU( CO( PU( 0)), 
                                   CO( PU( 1)) )))
    };

/**
 * Machine topology summaries set.
 */
static tpl_MachineSummary_t tpl_MachineSummariesSet [ MACHINES_MAX_NUM ] = {
    MACHINE_SUMMARY_INIT( TPL_TYPE_HOMOGENEOUS_SYMMETRIC, TPL_SMT_NONE, 1, 1, 2, 1, MALLOC_ANY, AVOID_INTERNUMANODE_CONNECTIONS, 2),
    MACHINE_SUMMARY_INIT( TPL_TYPE_HOMOGENEOUS_SYMMETRIC, TPL_SMT_HT, 1, 1, 2, 2, MALLOC_ANY, AVOID_INTERNUMANODE_CONNECTIONS, 4),
    MACHINE_SUMMARY_INIT( TPL_TYPE_HOMOGENEOUS_SYMMETRIC, TPL_SMT_HT, 1, 1, 4, 2, MALLOC_ANY, AVOID_INTERNUMANODE_CONNECTIONS, 8),
    MACHINE_SUMMARY_INIT( TPL_TYPE_HOMOGENEOUS_SYMMETRIC, TPL_SMT_HT, 1, 1, 4, 2, MALLOC_ANY, AVOID_INTERNUMANODE_CONNECTIONS, 8),
    MACHINE_SUMMARY_INIT( TPL_TYPE_HOMOGENEOUS_SYMMETRIC, TPL_SMT_HT, 2, 1, 6, 2, MALLOC_ON_W, AVOID_INTERNUMANODE_CONNECTIONS, 12),
    MACHINE_SUMMARY_INIT( TPL_TYPE_HOMOGENEOUS_SYMMETRIC, TPL_SMT_CMT, 4, 2, 2, 2, MALLOC_ANY, AVOID_INTERCORE_CONNECTIONS, 4),
    MACHINE_SUMMARY_INIT( TPL_TYPE_HOMOGENEOUS_SYMMETRIC, TPL_SMT_CMT, 4, 2, 4, 2, MALLOC_ANY, AVOID_INTERCORE_CONNECTIONS, 4),
    MACHINE_SUMMARY_INIT( TPL_TYPE_HOMOGENEOUS_SYMMETRIC, TPL_SMT_MIC, 1, 1, 60, 4, MALLOC_ANY, AVOID_INTERCORE_CONNECTIONS, 60),
    MACHINE_SUMMARY_INIT( TPL_TYPE_HOMOGENEOUS_SYMMETRIC, TPL_SMT_NONE, 1, 1, 2, 1, MALLOC_ANY, AVOID_INTERNUMANODE_CONNECTIONS, 2) };

/**
 * Machine description.
 */
static tpl_MachineDescription_t machineDescription;


#if defined( PTHREAD_BARRIER)
static pthread_barrier_t bar_pthreadBarrier [ BARRIERS_MAX_NUM ];
#endif

#ifdef SR_BARRIER
static sr_barrier_t bar_srBarrier [ BARRIERS_MAX_NUM ];
#endif

#ifdef TREE_BARRIER
static tree_barrier_t bar_treeBarrier [ BARRIERS_MAX_NUM ];
#endif

#if defined( DSMN_BARRIER) || defined( DSMNH_BARRIER)
static dsmn_barrier_t bar_dsmnBarrier [ BARRIERS_MAX_NUM ];
#endif

#ifdef DSMNH_BARRIER
static sr_barrier_t bar_srBarrier [ BARRIERS_MAX_NUM ][ THREADS_MAX_NUM ];
#endif

/**
 * Online cpu set
 */
static cpu_set_t bar_onlineCpuSet;

#ifdef SANITY_BENCHMARK
/**
 * Array for sanity testing of barriers semantics
 */
static int * bar_TestArray [ THREADS_MAX_NUM ];
#endif /* SANITY_BENCHMARK */

#ifdef NBODY_BENCHMARK
/**
 * Data buffer.
 */
static bar_Particle_t bar_ParticlesBuf [ THREADS_MAX_NUM ];
#endif /* NBODY_BENCHMARK */

static inline void
bar_Assert( int i)
{
#ifndef NASSERT
    assert( i);
#endif
}

static inline void
bar_InternalError( char * fileName, 
                   unsigned lineNo)
{
    fprintf( stderr, "Internal error %s:%u !\n", fileName, lineNo);
    exit( 1);
}

static inline void
sys_SetOnlineCpuSet( cpu_set_t * onlineCpuSet)
{
    * onlineCpuSet = bar_onlineCpuSet;
}


/* Lightweight memory manager. */
#define MEM_INIT_SIZE ( 8 * 1024 * 4096)
static void * baseMemNuma [ NUMANODES_PER_MACHINE_MAX_NUM ];
static void * curMemNuma [ NUMANODES_PER_MACHINE_MAX_NUM ];
static void * baseMem;
static void * curMem;
unsigned memSize;

static inline void
bar_MemInit( )
{
#ifdef LIB_NUMA
    if ( TOPOLOGY_NUMA_AWARE_ALLOC &&
         (machineDescription.machineId != UNDEFINED_MACHINE_ID) &&
         ((machineDescription.summary->socketsPerMachineNum * machineDescription.summary->numaNodesPerSocketNum) > 1))
    {
        int numaId;
        
        for ( numaId = 0;
              numaId < (machineDescription.summary->socketsPerMachineNum * machineDescription.summary->numaNodesPerSocketNum);
              numaId++ )
        {
            baseMemNuma [ numaId ] = numa_alloc_onnode( MEM_INIT_SIZE, numaId) + (CCL_SIZE * numaId);
            curMemNuma [ numaId ] = baseMemNuma [ numaId ];
        }
        memSize = MEM_INIT_SIZE;
    } else
#endif /* NUMA_ALLOC */
    {
        baseMem = (void *) malloc( MEM_INIT_SIZE * NUMANODES_PER_MACHINE_MAX_NUM);
        curMem = (void *) ((((size_t) baseMem + CCL_SIZE - 1) / CCL_SIZE) * CCL_SIZE);
        memSize = MEM_INIT_SIZE;
    }
}

static inline void *
bar_MemAlloc( int memSize,
              int threadRId,
              int threadWId,
              int * threadAllocId)
{
    int deltaMem = (((memSize + CCL_SIZE - 1) / CCL_SIZE) * CCL_SIZE);
    void * retMem = 0;

#ifdef LIB_NUMA
    if ( TOPOLOGY_NUMA_AWARE_ALLOC &&
         (machineDescription.machineId != UNDEFINED_MACHINE_ID) &&
         ((machineDescription.summary->socketsPerMachineNum * machineDescription.summary->numaNodesPerSocketNum) > 1))
    {
        int threadId = (machineDescription.summary->numaMallocPolicy == MALLOC_ON_W) ? threadWId : threadRId;
        int numaId = bar_ThreadIdToPUMap [ threadId ]->numaOsId;
        
        if ( threadAllocId != NULL )
        {
            (* threadAllocId) = threadId;
        }
        
        retMem = curMemNuma [ numaId ];
        curMemNuma [ numaId ] += deltaMem;
        if ( (curMemNuma [ numaId ] - baseMemNuma [ numaId ]) > MEM_INIT_SIZE )
        {
            bar_InternalError(  __FILE__, __LINE__);
        }
    } else
#endif /* NUMA_ALLOC */
    {
        retMem = curMem;
        curMem += deltaMem;
        if ( (curMem - baseMem) > (MEM_INIT_SIZE * NUMANODES_PER_MACHINE_MAX_NUM) )
        {
            bar_InternalError(  __FILE__, __LINE__);
        }
    }
    return retMem;
}

static inline void
bar_MemReuse( )
{
#ifdef LIB_NUMA
    if ( TOPOLOGY_NUMA_AWARE_ALLOC &&
         (machineDescription.machineId != UNDEFINED_MACHINE_ID) &&
         ((machineDescription.summary->socketsPerMachineNum * machineDescription.summary->numaNodesPerSocketNum) > 1))
    {
        int numaId;
        
        for ( numaId = 0;
              numaId < (machineDescription.summary->socketsPerMachineNum * machineDescription.summary->numaNodesPerSocketNum);
              numaId++ )
        {
            curMemNuma [ numaId ] = baseMemNuma [ numaId ];
        }
    } else
#endif /* NUMA_ALLOC */
    {
        curMem = (void *) ((((size_t) baseMem + CCL_SIZE - 1) / CCL_SIZE) * CCL_SIZE);
    }
}

static inline void
bar_MemFini( )
{
#ifdef LIB_NUMA
    if ( TOPOLOGY_NUMA_AWARE_ALLOC &&
         (machineDescription.machineId != UNDEFINED_MACHINE_ID) &&
         ((machineDescription.summary->socketsPerMachineNum * machineDescription.summary->numaNodesPerSocketNum) > 1))
    {
        int numaId;
        
        for ( numaId = 0;
              numaId < (machineDescription.summary->socketsPerMachineNum * machineDescription.summary->numaNodesPerSocketNum);
              numaId++ )
        {
            numa_free( baseMemNuma [ numaId ], MEM_INIT_SIZE);
        }
    } else
#endif /* NUMA_ALLOC */
    {
        free( baseMem);
    }
}

static inline int
math_log2Ceil( int threadsNum)
{
    int res = 0;
   
    bar_Assert( threadsNum > 0);
    while ( (1 << res) < threadsNum )
    {
        res++;
    }
    
    return res;
}

static inline int
math_logNCeil( int n, 
               int threadsNum)
{
    int res = 0;
    int ceil = 1;

    bar_Assert( threadsNum > 0);
    while ( ceil < threadsNum )
    {
        ceil *= n;
        res ++;
    }
    
    return res;
}

#ifdef ARCH_X86_FAMILY
static void
x86_Cpuid( int in_eax, int * eax, int * ebx, int * ecx, int * edx )
{
  asm volatile( "pushq %%rbx   \n\t"
                "cpuid         \n\t"
                "movq %%rbx, %1\n\t"
                "popq %%rbx    \n\t"
                : "=a"( eax), "=r"(ebx), "=c"(ecx), "=d"(edx)
                : "a"( in_eax)
                : "cc" );
}
#endif

static inline int
sys_GetPrivilegeLevel( )
{

#   ifdef ARCH_ARM_FAMILY
    bar_InternalError( __FILE__, __LINE__);
#   else
#       ifdef ARCH_X86_FAMILY
    {
        short int csReg;
        short int ringMask = X86_RINGS_MASK;

        asm volatile( " mov %%cs, %0\n\t"
                      : "=r"( csReg)
                      );

        return csReg & ringMask;
    }
#       else
    bar_InternalError( __FILE__, __LINE__);
#       endif
#   endif
}


#ifdef ARCH_LOAD_NC
#   ifdef ARCH_X86_64
static inline int
load_nc_int( void * addr)
{
    int res;
    __m128i siVec;

    asm volatile( " movntdqa  %1, %0\n\t"
                  : "=x"(siVec)
                  : "m"(*(__m128i *)addr)
                  );
    res = _mm_cvtsi128_si32( siVec);

    return res;
}

static inline bool
load_nc_bool( void * addr)
{
    return load_nc_int( addr);
}

static inline int_max_ma_t
load_nc_int_max_ma( void * addr)
{
    return load_nc_int( addr);
}
#   endif
#endif

#ifdef ARCH_STORE_NR_NGO
#   ifdef ARCH_MIC
static inline void
store_nr_ngo_int( void * addr,
                  int data)
{
    __m512i siVec = _mm512_set1_epi32( data);

    _mm512_storenrngo_ps( addr, _mm512_castsi512_ps( siVec));
    asm volatile( "lock; addl $0,(%rsp)\n");
}

static inline void
store_nr_ngo_bool( void * addr,
                   bool data)
{
    store_nr_ngo_int( addr, data);
}

static inline void
store_nr_ngo_int_max_ma( void * addr, 
                         int_max_ma_t data)
{
    __m512i siVec = _mm512_set1_epi64( (__int64)data);

    bar_Assert( sizeof( __int64) <= HW_MAX_MA_SIZE_IN_BITS);
    _mm512_storenrngo_ps( addr, _mm512_castsi512_ps( siVec));
    asm volatile( "lock; addl $0,(%rsp)\n");
}
#   endif /* ARCH_MIC */
#endif /* ARCH_STORE_NR_NGO */

#ifdef ARCH_STORE_NR
#   ifdef ARCH_MIC
static inline void
store_nr_int( void * addr,
              int data)
{
    __m512i siVec = _mm512_set1_epi32( data);

    _mm512_storenr_ps( addr, _mm512_castsi512_ps( siVec));
}

static inline void
store_nr_bool( void * addr,
               bool data)
{
    store_nr_int( addr, data);
}

static inline void
store_nr_int_max_ma( void * addr, 
                     int_max_ma_t data)
{
    __m512i siVec = _mm512_set1_epi64( (__int64)data);

    bar_Assert( sizeof( __int64) <= HW_MAX_MA_SIZE_IN_BITS);
    _mm512_storenr_ps( addr, _mm512_castsi512_ps( siVec));
}
#   endif /* ARCH_MIC */
#   ifdef ARCH_X86_64
static inline void
store_nr_int( void * addr,
              int data)
{
    __m128i siVec = _mm_cvtsi32_si128( data);

    _mm_stream_si128( addr, siVec);
}

static inline void
store_nr_bool( void * addr,
               bool data)
{
    store_nr_int( addr, data);
}

static inline void
store_nr_int_max_ma( void * addr, 
                     int_max_ma_t data)
{
    __m128i siVec = _mm_cvtsi64_si128( data);

    _mm_stream_si128( addr, siVec);
}

#   endif /* ARCH_X86_64 */
#endif /* ARCH_STORE_NR */

inline static void
memory_barrier( )
{
#   ifdef ARCH_ARM_FAMILY
        asm volatile ( "dmb" : : : "memory");
#   else
#       if defined( ARCH_X86_FAMILY) && !defined( ARCH_MIC)
        asm volatile ( "mfence" : : : "memory");
#       else
#           if !defined( ARCH_MIC)
            bar_InternalError( __FILE__, __LINE__);
#           endif
#       endif
#   endif
}

#ifdef YIELD_SPINNING
inline static void
spinning_thread_yield( )
{
#ifdef HWYIELD_SPINNING
#   ifdef ARCH_ARM_FAMILY
    asm volatile ( "yield\n\t");
#   else
#       ifdef ARCH_X86_FAMILY
    asm volatile ( "hlt\n\t");
#       else
    bar_InternalError( __FILE__, __LINE__);
#       endif
#   endif
#endif /* HWYIELD_SPINNING */
#ifdef PTYIELD_SPINNING
    pthread_yield( );
#endif /* PTYIELD_SPINNING */
}
#endif

#ifdef PAUSE_SPINNING
inline static void
spinning_pause( )
{
#   ifdef ARCH_ARM_FAMILY
    asm volatile ( "wfi\n\t");
#   else
#       ifdef ARCH_X86_FAMILY
#           ifdef ARCH_MIC
    _mm_delay_32( ARCH_MIC_DELAY);
#           else
    asm volatile ( "pause\n\t");
#           endif
#       else
    bar_InternalError( __FILE__, __LINE__);
#       endif
#   endif
}
#endif /* PAUSE_SPINNING */

#ifdef WFE_SPINNING
inline static void
spinning_thread_wfe_init( void * mem)
{
#   ifdef ARCH_ARM_FAMILY
#   else
#       if defined( ARCH_X86_FAMILY) && defined( ARCH_X86_MONITOR_MWAIT)
    {
        asm volatile( " mov  %0, %%rax\n\t"
                      " mov  $0x0, %%rcx\n\t"
                      " mov  $0x0, %%rdx\n\t"
                      " monitor %%rax, %%rcx, %%rdx\n\t"
                      :
                      : "r"( mem)
                      : "%rax", "%rcx", "%rdx");
    }
#       else
    bar_InternalError( __FILE__, __LINE__);
#       endif
#   endif
}

inline static void
spinning_thread_wfe_wait( )
{
#   ifdef ARCH_ARM_FAMILY
    asm volatile ( "wfe\n");
#   else
#       if defined( ARCH_X86_FAMILY) && defined( ARCH_X86_MONITOR_MWAIT)
    asm volatile( " movl  0, %%eax\n\t"
                  " movl  0, %%ecx\n\t"
                  " mwait\n"
                  :
                  :
                  : "%eax", "%ecx");
#       else
    bar_InternalError( __FILE__, __LINE__);
#       endif
#   endif
}

inline static void
spinning_thread_wfe_send( )
{
#   ifdef ARCH_ARM_FAMILY
    asm volatile ( "sev\n");
#   else
#       if defined( ARCH_X86_FAMILY) && defined( ARCH_X86_MONITOR_MWAIT)
#       else
    bar_InternalError( __FILE__, __LINE__);
#       endif
#   endif
}
#endif

inline static int
load_linked( volatile int * x)
{
    int val = 0;
#ifdef ARCH_LL_SC
#   ifdef ARCH_ARM_FAMILY

    asm volatile ("ldrex %0, [%1]\n\t"
       : "=r"( val)
       : "r"( x)
       : "memory");

    return val;
#   else /* ARCH_ARM_FAMILY */
    bar_InternalError( __FILE__, __LINE__);

    return val;
#   endif /* !ARCH_ARM_FAMILY */
#else /* ARCH_LL_SC */
    bar_InternalError( __FILE__, __LINE__);

    return val;
#endif /* !ARCH_LL_SC */
}

inline static bool
store_conditional( volatile int * x, int newVal)
{
#ifdef ARCH_LL_SC
#   ifdef ARCH_ARM_FAMILY
    int res;

    asm volatile ("strex %0, %1, [%2]\n\t"
       : "=r"( res)
       : "r"( newVal), "r"( x)
       : "memory", "r0" );
    
    return !res;
#   else /* ARCH_ARM_FAMILY */
    bar_InternalError( __FILE__, __LINE__);

    return RETURN_FAIL;
#   endif /* !ARCH_ARM_FAMILY */
#else /* ARCH_LL_SC */
    bar_InternalError( __FILE__, __LINE__);

    return RETURN_FAIL;
#endif /* !ARCH_LL_SC */
}

inline static bool
compare_and_swap( volatile int * x, int oldVal, int newVal)
{
#ifdef ARCH_CAS
#   ifdef ARCH_X86_FAMILY
    int res;

    __asm__ __volatile__ (
        "  lock\n\t"
        "  cmpxchgl %2,%1\n\t"
        "  sete %%al\n\t"
        "  movzbl %%al, %0\n\t"
        : "=q" (res), "=m" (*x)
        : "r" (newVal), "m" (*x), "a" (oldVal)
        : "memory");

    return (bool)res;
#   endif
#else /* ARCH_CAS */
#   ifdef ARCH_LL_SC
    {
        bool noSucc;
        
        do 
        {
            if ( load_linked( x) != oldVal )
            {
                return FALSE;
            } 
            noSucc = store_conditional( x, newVal);
        } while ( !noSucc );

        return TRUE;
    }
#   else /* ARCH_LL_SC */
    bar_InternalError( __FILE__, __LINE__);

    return RETURN_FAIL;
#   endif /* !ARCH_LL_SC */
#endif /* !ARCH_CAS */
}

inline static int
fetch_and_add( volatile int * variable, 
               int inc)
{
#ifdef ARCH_FETCH_AND_ADD
#   ifdef ARCH_X86_FAMILY
    asm volatile( "lock; xaddl %0, %1;\n\t"
                  :"=r" (inc)                   /* Output */
                  :"m" (*variable), "0" (inc)  /* Input */
                  :"memory" );
    return inc;
#   else
    bar_InternalError( __FILE__, __LINE__);
#   endif
#else /* ARCH_FETCH_AND_ADD */
#   ifdef ARCH_LL_SC
    {
        bool noSucc;
        int val;
        
        do 
        {
            val = load_linked( variable);
            noSucc = store_conditional( variable, val + inc);
        } while ( !noSucc );

        return val;
    }
#   else /* ARCH_LL_SC */
#       ifdef ARCH_CAS
        {
            bool succ = FALSE;
            
            do 
            {
                int val;

                val = *variable;
                succ = compare_and_swap( variable, val, val + inc);
            } while ( !succ );
        }
#       else
        bar_InternalError( __FILE__, __LINE__);
#       endif
#   endif /* !ARCH_LL_SC */
#endif /* !ARCH_FETCH_AND_ADD */
}

#if defined( SR_BARRIER) || defined( DSMNH_BARRIER)
static void
sr_barrier_init( sr_barrier_t * sr_barrier,
                 void * dummy,
                 int threadsNum,
                 int barrierId)
{
    sr_barrier->sense = 0;
    sr_barrier->count = threadsNum;
    sr_barrier->threadsNum = threadsNum;
    sr_barrier->barrierId = barrierId;
}

static inline void
sr_barrier_set_sense( volatile bool * addr,
                      bool sense)
{
#ifdef ARCH_STORE_NR_NGO
    store_nr_ngo_bool( (void *) addr, sense);
#else
#   ifdef ARCH_STORE_NR
    store_nr_bool( (void *) addr, sense);
#   else
    (* addr) = sense;
#   endif 
#endif
}

static inline bool
sr_barrier_load_bool( volatile bool * addr)
{
#ifdef ARCH_LOAD_NC
    return load_nc_bool( (void *) addr);
#else
    return (* addr);
#endif
}

static inline bool
sr_barrier_load_sense( volatile bool * sense_addr)
{
    return sr_barrier_load_bool( sense_addr);
}

static inline void
sr_barrier_set_count( volatile bool * addr,
                      int count)
{
#ifdef ARCH_STORE_NR
    store_nr_int( (void *) addr, count);
#else
    (* addr) = count;
#endif 
}

#ifdef DSMNH_BARRIER
static inline void dsmn_barrier_wait( dsmn_barrier_t * dsmn_barrier, tls_Data_t * dsmn_barrier_tls_data);
#endif

static inline void
sr_barrier_wait( sr_barrier_t * sr_barrier,
                 tls_Data_t * tlsData)
{
#ifdef DSMNH_BARRIER
    int * senseP = & tlsData->senseSR[ sr_barrier->barrierId ][ tlsData->groupId ].data;
#else
    int * senseP = & tlsData->sense[ sr_barrier->barrierId ].data;
#endif
    int currCount = fetch_and_add( & (sr_barrier->count), -1);
    int currSense = *senseP;

    if ( currCount == 1 )
    {
#ifdef DSMNH_BARRIER
        dsmn_barrier_wait( & bar_dsmnBarrier [ sr_barrier->barrierId ], tlsData->dsmnTlsData);
#endif
        sr_barrier_set_count( & (sr_barrier->count), sr_barrier->threadsNum);
        sr_barrier_set_sense( & (sr_barrier->sense), currSense);
#ifdef WFE_SPINNING
        spinning_thread_wfe_send( );
#endif
    } else
    {
#ifdef WFE_SPINNING
        if ( currSense != sr_barrier_load_sense( & (sr_barrier->sense)) )
        {
            spinning_thread_wfe_init( (void *) & sr_barrier->sense);
        }
#endif
        while ( currSense != sr_barrier_load_sense( & (sr_barrier->sense)) )
        {
#ifdef YIELD_SPINNING
            spinning_thread_yield( );
#endif
#ifdef PAUSE_SPINNING
            spinning_pause( );
#endif
#ifdef WFE_SPINNING
            spinning_thread_wfe_wait( );
#endif
        };
    }
    sr_barrier_set_sense( senseP, !currSense);
}
#endif /* SR_BARRIER || DSMNH_BARRIER */

#ifdef TREE_BARRIER
#   ifdef TRNM_BARRIER
/**
 * Specialized (see assertions) power function
 */
static inline int
math_pow( int val, int power)
{
    int res = 1;

    bar_Assert( (power >= 0) && (power <= CEIL_LOG2_THREADS_MAX_NUM));
    while ( power-- )
    {
        res *= val;        
    }
    
    return res;
}
#   endif /* TRNM_BARRIER */

static void
tree_InitBoundsGetActiveNodesHelper( tpl_PU_t * puToPlace,
                                     tpl_NodeType_t startTplLevel,
                                     int (* startId) [ TPL_NODES_DEFINED_NUM],
                                     int (* stopId) [ TPL_NODES_DEFINED_NUM])
{
    tpl_NodeType_t nt;
    int lb, hb;

    for ( nt = TPL_NODE_SOCKET;
          nt < TPL_NODES_DEFINED_NUM;
          nt++ )
    {
        if ( nt < startTplLevel )
        {
            switch ( nt )
            {
            case TPL_NODE_SOCKET:
            {
                lb = puToPlace->socketId;
                hb = puToPlace->socketId + 1;
            }
            break;
            case TPL_NODE_NUMANODE:
            {
                lb = puToPlace->numaNodeId;
                hb = puToPlace->numaNodeId + 1;
            }
            break;
            case TPL_NODE_CORE:
            {
                lb = puToPlace->coreId ;
                hb = puToPlace->coreId + 1;
            }
            break;
            case TPL_NODE_PU:
            default:
                bar_InternalError(  __FILE__, __LINE__);
            }
        } else
        {
            switch ( nt )
            {
            case TPL_NODE_SOCKET:
            {
                lb = 0;
                hb = machineDescription.summary->socketsPerMachineNum;
            }
            break;
            case TPL_NODE_NUMANODE:
            {
                lb = 0;
                hb = machineDescription.summary->numaNodesPerSocketNum;
            }
            break;
            case TPL_NODE_CORE:
            {
                lb = 0;
                hb = machineDescription.summary->coresPerNumaNodeNum;
            }
            break;
            case TPL_NODE_PU:
            {
                lb = 0;
                hb = machineDescription.summary->pusPerCoreNum;
            }
            break;
            default:
                bar_InternalError(  __FILE__, __LINE__);
            }
        }
        (*startId) [ nt ] = lb;
        (*stopId) [ nt ] = hb;
    }
}

static int
tree_GetActiveNodesFromUpToLevel( tree_barrier_t * tree_barrier,
                                  tree_build_context_t * tbc, 
                                  tpl_NodeType_t fromTplLevel, 
                                  tpl_NodeType_t upToTplLevel)
{
    int so, nu, co, pu;
    tpl_PU_t * puToPlace = bar_ThreadIdToPUMap [ tree_barrier->leavesNum ];
    int startId [ TPL_NODES_DEFINED_NUM ];
    int stopId [ TPL_NODES_DEFINED_NUM ];
    int activeNodesNum = 0;

    if ( TOPOLOGY_AWARE_MAPPING == TRUE )
    {
        tree_InitBoundsGetActiveNodesHelper( puToPlace, fromTplLevel, & startId, & stopId);
        for ( so = startId [ TPL_NODE_SOCKET ];
              so < stopId [ TPL_NODE_SOCKET ];
              so ++ )
        {
            if ( upToTplLevel == TPL_NODE_SOCKET )
            {
                activeNodesNum += (machineDescription.sockets [ so ].activeThreadsNum > 0) ? 1 : 0;
            }

            for ( nu = startId [ TPL_NODE_NUMANODE ];
                  nu < stopId [ TPL_NODE_NUMANODE ];
                  nu ++ )
            {
                if ( upToTplLevel == TPL_NODE_NUMANODE )
                {
                    activeNodesNum += (machineDescription.sockets [ so ].numaNodes [ nu ].activeThreadsNum > 0) ? 1 : 0;
                }

                for ( co = startId [ TPL_NODE_CORE ];
                      co < stopId [ TPL_NODE_CORE ];
                      co ++ )
                {
                    if ( upToTplLevel == TPL_NODE_CORE )
                    {
                        activeNodesNum += (machineDescription.sockets [ so ].numaNodes [ nu ].cores [ co ].activeThreadsNum > 0) ? 1 : 0;
                    }

                    for ( pu = startId [ TPL_NODE_PU ];
                          pu < stopId [ TPL_NODE_PU ];
                          pu ++ )
                    {
                        if ( upToTplLevel == TPL_NODE_PU )
                        {
                            activeNodesNum += machineDescription.sockets [ so ].numaNodes [ nu ].cores [ co ].pus [ pu]->activeThreadsNum;
                        }
                    }
                }
            }
        }
    } else
    {
        activeNodesNum = tree_barrier->threadsNum;
    } 
    return activeNodesNum;
}

#ifdef TRNM_STAT_WIN
static int
tree_GetTPLNodeLevelSignatureByPU( tpl_NodeType_t tplLevel, 
                                   tpl_PU_t * pu)
{
    switch ( tplLevel )
    {
    case TPL_NODE_SOCKET:
        return pu->socketId;
    case TPL_NODE_NUMANODE:
        return pu->numaNodeId + (pu->socketId * machineDescription.summary->numaNodesPerSocketNum);
    default:
        /* precondition */
        bar_InternalError( __FILE__, __LINE__);
    }
}

static int
tree_CalculateSTNInodeThreadWriteId( tree_barrier_t * tree_barrier,
                                     tree_build_context_t * tbc)
{
    int threadWriteId = tree_barrier->inodesNum;
    int tplDelta;
    int curDelta = 0;
    int curTplSignature, newTplSignature;
    
    if ( tbc->curTplLevel > TPL_NODE_NUMANODE )
    {
        return threadWriteId;
    }

    tplDelta =
        math_pow( tree_barrier->radix, (tbc->reachHeight [ tbc->curTplLevel ] - tbc->curHeight [ tbc->curTplLevel ] - 1));

    curTplSignature = tree_GetTPLNodeLevelSignatureByPU( tbc->curTplLevel,
        bar_ThreadIdToPUMap [ tree_barrier->inodesNum ]);

    for ( threadWriteId = tree_barrier->inodesNum;
          threadWriteId < tree_barrier->threadsNum;
          threadWriteId ++ )
    {
        newTplSignature = tree_GetTPLNodeLevelSignatureByPU( tbc->curTplLevel,
            bar_ThreadIdToPUMap [ threadWriteId ]);
        if ( newTplSignature != curTplSignature )
        {
            curDelta++;
            curTplSignature = newTplSignature;
        }
        if ( curDelta == tplDelta )
        {
            break;
        }
    }
    if ( threadWriteId == tree_barrier->threadsNum)
    {
        bar_InternalError( __FILE__, __LINE__);
    }
    
    return threadWriteId;
}
#endif

static int
tree_inode_construct( tree_barrier_t * tree_barrier,
                      tree_build_context_t * tbc,
                      tree_node_t ** child)
{
    int threadAllocId;
    int threadWriteId = 
#ifdef TRNM_STAT_WIN
        tree_CalculateSTNInodeThreadWriteId( tree_barrier, tbc);
#else
        tree_barrier->inodesNum;
#endif
#ifdef T_GLOBAL_SENSE
    if ( tbc->parent == NULL )
    {
        tree_barrier->sense =
            bar_MemAlloc( sizeof( bool), threadWriteId, tree_barrier->inodesNum, NULL);
        (* tree_barrier->sense) = FALSE;
    }
#endif
    tree_barrier->inodes [ tree_barrier->inodesNum ] =
        bar_MemAlloc( sizeof( tree_node_t), tree_barrier->inodesNum, threadWriteId, & threadAllocId);
    *child  = tree_barrier->inodes [ tree_barrier->inodesNum ];
#ifdef T_LOCAL_SENSE
    (*child)->sense = 
        bar_MemAlloc( sizeof( bool), threadWriteId, tree_barrier->inodesNum, NULL);
    (* ((*child)->sense)) = FALSE;
#endif
    tree_barrier->inodesNum ++;
#ifdef COMBINED_BARRIER
    (*child)->count = 0;
    (*child)->threadsNum = 0;
#endif
#ifdef TRNM_BARRIER
    (*child)->tier = tbc->curHeight [ tbc->curTplLevel ];
    (*child)->trnmDataCurr.full = TRNM_FALSE;
    (*child)->trnmDataInit [ PARITY_EVEN ].full = TRNM_FALSE;
    (*child)->trnmDataInit [ PARITY_ODD ].full = TRNM_FALSE;
#endif
    (*child)->parent = tbc->parent;

    return threadAllocId;
}

static inline void
tree_AdjustReachHeight( tree_barrier_t * tree_barrier,
                       tree_build_context_t * tbc,
                       int edgeId)
{
    int curHeight, threadsRest, deltaHeight, curHeightEdgesRest;

    if ( 
#ifdef TRNM_STAT_WIN
         edgeId == TRNM_STAT_WIN_ID
#else
         edgeId == 0
#endif
       )
    {
        return;
    }

    threadsRest = tbc->tplLevelLeavesToConstruct [ tbc->curTplLevel ];
#ifdef TRNM_STAT_WIN
    curHeightEdgesRest = (tree_barrier->radix - 1) - edgeId;
#else
    curHeightEdgesRest = tree_barrier->radix - edgeId;
#endif
    bar_Assert( threadsRest > 0);
    deltaHeight = math_logNCeil( tree_barrier->radix, (threadsRest + curHeightEdgesRest - 1) / curHeightEdgesRest) + 1;
    if ( deltaHeight <  tbc->reachHeight [ tbc->curTplLevel ] - tbc->curHeight [ tbc->curTplLevel ] )
    {
        tbc->reachHeight [ tbc->curTplLevel ] = tbc->curHeight [ tbc->curTplLevel ] + deltaHeight;
    }
}

static inline void
tree_barrier_action_before_edge_construction( tree_barrier_t * tree_barrier,
                                              tree_build_context_t * tbc, 
                                              int edgeId,
                                              tree_node_t * child,
                                              int * leavesNumBEC)
{
    /* actions on barrier */
    (*leavesNumBEC) = tree_barrier->leavesNum;

    /* adjust reach height */
    tree_AdjustReachHeight( tree_barrier, tbc, edgeId);

    /* actions on context */
    tbc->parent = child;
    tbc->curHeight [ tbc->curTplLevel ] ++;
    if ( tbc->curHeight [ tbc->curTplLevel ] == tbc->reachHeight [ tbc->curTplLevel ] )
    {
        tbc->tplLevelLeavesToConstruct [ tbc->curTplLevel ] --;
    }
}

static inline void
tree_barrier_action_after_edge_construction( tree_barrier_t * tree_barrier,
                                             tree_build_context_t * tbc, 
                                             int edgeId,
                                             tree_node_t * child,
                                             int * leavesNumBEC)
{

    /* actions on context */
    tbc->curHeight [ tbc->curTplLevel ]--;

    /* actions on barrier */
#ifdef COMBINED_BARRIER
    child->count++;
    child->threadsNum++;
#endif
#ifdef TRNM_BARRIER
#   ifdef TRNM_STAT_WIN
    if ( edgeId != TRNM_STAT_WIN_ID )
    {
        child->trnmDataInit [ PARITY_ODD ].part [ edgeId ] = TRNM_TRUE;
    }
#   endif
#   ifdef TRNM_DYNM_WIN
    child->trnmDataInit [ PARITY_ODD ].part [ edgeId ] = TRNM_TRUE;
#   endif
    {
        int i;

        for ( i = (*leavesNumBEC); i < tree_barrier->leavesNum; i++ )
        {
            tree_barrier->partIdMap [ i ] [ tbc->curHeight [ tbc->curTplLevel ] ] = edgeId;
        }
    }
#endif 
}

static inline void
tree_barrier_action_on_inode_construction( tree_build_context_t * tbc)
{
}

static void
tree_barrier_action_on_node_start( tree_barrier_t * tree_barrier, 
                                   tree_build_context_t * tbc)
{
    while ( (tbc->curHeight [ tbc->curTplLevel ] == tbc->reachHeight [ tbc->curTplLevel ]) &&
            (tbc->curTplLevel < TPL_NODE_PU) )
    {
        int deltaHeight;
        int activeNodesNum;
        tpl_NodeType_t startTplLevel = tbc->curTplLevel + 1;
        tpl_NodeType_t nextTplLevel = startTplLevel;
        tbc->curTplLevel = startTplLevel;
        tbc->curHeight [ tbc->curTplLevel ] = tbc->reachHeight [ tbc->curTplLevel - 1 ];

        while ( !machineDescription.summary->avoidInterNodeConnections [ nextTplLevel ] &&
                (nextTplLevel != TPL_NODE_PU) )
        {
            nextTplLevel++;
        }
        while ( tbc->curTplLevel < nextTplLevel )
        {
            tbc->reachHeight [ tbc->curTplLevel ] = tbc->curHeight [ tbc->curTplLevel ];
            tbc->tplLevelLeavesToConstruct [ tbc->curTplLevel ] = 0;
            tbc->curTplLevel++;
            tbc->curHeight [ tbc->curTplLevel ] = tbc->reachHeight [ tbc->curTplLevel - 1 ];
        }
        activeNodesNum = tree_GetActiveNodesFromUpToLevel( tree_barrier, tbc, startTplLevel, nextTplLevel);
        deltaHeight = math_logNCeil( tree_barrier->radix, activeNodesNum);
        tbc->reachHeight [ tbc->curTplLevel ] = tbc->curHeight [ tbc->curTplLevel ] + deltaHeight;
        if ( deltaHeight > 0)
        {
            tbc->tplLevelLeavesToConstruct [ tbc->curTplLevel ] = activeNodesNum;
        } else
        {
            tbc->tplLevelLeavesToConstruct [ tbc->curTplLevel ] = 0;
        }
    }

    /* special case for experimenting with one thread */
    if ( (tbc->curTplLevel >= TPL_NODE_PU) && (tree_barrier->threadsNum == 1) &&
         (tbc->reachHeight [ tbc->curTplLevel ] == 0) )
    {
        tbc->reachHeight [ tbc->curTplLevel ] ++;
        tbc->tplLevelLeavesToConstruct [ tbc->curTplLevel ] = 1;
    }
}

static inline void
tree_barrier_action_on_node_finish( tree_build_context_t * tbc)
{
    if ( TOPOLOGY_AWARE_MAPPING == TRUE )
    {
        bar_Assert( (tbc->curTplLevel > TPL_NODE_MACHINE) && (tbc->curTplLevel < TPL_NODE_UNDEFINED));
        while ( tbc->curHeight [ tbc->curTplLevel ] == tbc->curHeight [ tbc->curTplLevel - 1 ] )
        {
            tbc->curTplLevel--;
            if ( tbc->curTplLevel == TPL_NODE_MACHINE )
            {
                bar_Assert( tbc->curHeight [ TPL_NODE_MACHINE ] == 0);
                return;
            }
        }
    }
}

static bool
tree_barrier_is_leaf_to_be_constructed( tree_build_context_t * tbc)
{
    return (tbc->curHeight [ tbc->curTplLevel ] == tbc->reachHeight [ tbc->curTplLevel ]);
}

static void
tree_barrier_build_tree( tree_barrier_t * tree_barrier,
                         tree_build_context_t * tbc)
{
    int i, leavesNumBEC;
    tree_node_t * child;
    
    tree_barrier_action_on_node_start( tree_barrier, tbc);
    if ( tree_barrier_is_leaf_to_be_constructed( tbc) )
    {
        if ( tree_barrier->leavesNum < tree_barrier->threadsNum )
        {
            tree_barrier->leaves [ tree_barrier->leavesNum++ ] = tbc->parent;
        }
    } else
    {
        int threadAllocId, threadId = tree_barrier->inodesNum;

        threadAllocId = tree_inode_construct( tree_barrier, tbc, & child);
        tree_barrier_action_on_inode_construction( tbc);
        for (
#   ifdef TRNM_STAT_WIN
              i = TRNM_STAT_WIN_ID;
              i < tree_barrier->radix - 1;
#   else
              i = 0;
              i < tree_barrier->radix;
#   endif
              i++ )
        {
            if ( tbc->tplLevelLeavesToConstruct [ tbc->curTplLevel ] )
            {
                tbc->parentEdgeId = i;
                tree_barrier_action_before_edge_construction( tree_barrier, tbc, i, child, &leavesNumBEC);
                tree_barrier_build_tree( tree_barrier, tbc);
                tree_barrier_action_after_edge_construction( tree_barrier, tbc, i, child, &leavesNumBEC);
            }
        }
    }
    tree_barrier_action_on_node_finish( tbc);
}

static void
tree_InitBuildContext( tree_barrier_t * tree_barrier,
                       tree_build_context_t * tbc, 
                       unsigned radix, 
                       unsigned barrierCount)
{
    tbc->parent = NULL;
    if ( TOPOLOGY_AWARE_MAPPING == FALSE )
    {
        int height = 1;

        if ( barrierCount > 1 )
        {
            height = math_logNCeil( radix, barrierCount);
        }
        tbc->curTplLevel = TPL_NODE_UNDEFINED;
        tbc->curHeight [ TPL_NODE_UNDEFINED ] = 0;
        tbc->reachHeight [ TPL_NODE_UNDEFINED ] = height;
        tbc->tplLevelLeavesToConstruct [ TPL_NODE_UNDEFINED ] = barrierCount;
    } else
    {
        tbc->curTplLevel = TPL_NODE_MACHINE;
        tbc->curHeight [ TPL_NODE_MACHINE ] = 0;
        tbc->reachHeight [ TPL_NODE_MACHINE ] = 0;
        tbc->tplLevelLeavesToConstruct [ TPL_NODE_MACHINE ] = 0;
    }
    tbc->root = tree_barrier->inodes [ 0 ];
}

static void
tree_barrier_init( tree_barrier_t * tree_barrier,
                   tls_DataSet_t * tlsDataSet,
                   int barrier_count,
                   int radix,
                   int barrierId,
                   int threadBaseId)
{
    tree_build_context_t tbc;
    int nodes_count = 0;
    int i = 0;

    bar_Assert( barrier_count > 0 && radix > 1);
    tree_barrier->radix = radix;
    tree_barrier->leavesNum = 0;
    tree_barrier->inodesNum = 0;
    tree_barrier->threadsNum = barrier_count;
    tree_barrier->barrierId = barrierId;
    for ( i = 0; i < barrier_count; i++ )
    {
        tlsDataSet->tlsData [ threadBaseId + i ]->leafId = i;
    }
    tree_InitBuildContext( tree_barrier, & tbc, radix, barrier_count);
    tree_barrier_build_tree( tree_barrier, & tbc);
}

static inline void
tree_barrier_set_sense( volatile bool * addr,
                        bool sense)
{
#if defined( ARCH_STORE_NR_NGO)
    store_nr_ngo_bool( (void *) addr, sense);
#else
#   ifdef ARCH_STORE_NR
    store_nr_bool( (void *) addr, sense);
#   else
    (* addr) = sense;
#   endif 
#endif
}

static inline void
tree_barrier_set_count( volatile int * addr,
                        int count)
{
#ifdef ARCH_STORE_NR
    store_nr_int( (void *) addr, count);
#else
    (* addr) = count;
#endif 
}

static inline void
tree_barrier_set_full( int_max_ma_vol_t * addr,
                       int_max_ma_t full)
{
#ifdef ARCH_STORE_NR
    store_nr_int_max_ma( (void *) addr, full);
#else
    (* addr) = full;
#endif 
}

static inline int_max_ma_t
tree_barrier_load_full( int_max_ma_vol_t * addr)
{
#ifdef ARCH_LOAD_NC
    return load_nc_int_max_ma( (void *) addr);
#else
    return (* addr);
#endif
}

static inline bool
tree_barrier_load_sense( volatile bool * addr)
{
#ifdef ARCH_LOAD_NC
    return load_nc_bool( (void *) addr);
#else
    return (* addr);
#endif
}

static inline void
tree_barrier_wait( tree_barrier_t * tree_barrier,
                   tls_Data_t * tlsData,
                   tree_node_t * node)
{
    int * senseP = & (tlsData->sense[ tree_barrier->barrierId ].data);
#ifdef COMBINED_BARRIER
    int currCount = fetch_and_add( & (node->count), -1);
#endif
    int currSense = *senseP;

#ifdef TRNM_BARRIER
    int partId = tree_barrier->partIdMap [ tlsData->leafId ] [ node->tier ];
    int_min_ma_vol_t * partP = & (node->trnmDataCurr.part [ partId ]);
    int_max_ma_vol_t * fullP = & (node->trnmDataCurr.full);
#   ifdef TRNM_STAT_WIN
    bool isWinner = (partId == TRNM_STAT_WIN_ID);
    if ( !isWinner )
    {
        (*partP) = currSense;
    }
#   endif
#   ifdef TRNM_DYNM_WIN
    (*partP) = currSense;
#ifdef WFE_SPINNING
    spinning_thread_wfe_send( );
#endif
    /* In case when Intra-Processor Forwarding Is Allowed (as in X86 ISA 8.2.3.5) the load above
       and store below in if statement may lead to all threads going to busy-waiting. */
#ifdef INTRA_PROCESSOR_FORWARDING_ALLOWED
    memory_barrier( );
#endif
#   endif
#endif
    if ( 
#ifdef COMBINED_BARRIER
         currCount == 1
#endif
#ifdef TRNM_BARRIER
#   ifdef TRNM_STAT_WIN
         isWinner == TRUE
#   endif
#   ifdef TRNM_DYNM_WIN
         tree_barrier_load_full( fullP) == node->trnmDataInit [ currSense ].full
#   endif
#endif
       )
    {
#ifdef TRNM_BARRIER
#   ifdef TRNM_STAT_WIN
#       ifdef WFE_SPINNING
        if ( tree_barrier_load_full( fullP) != node->trnmDataInit [ currSense ].full ) 
        {
            spinning_thread_wfe_init( (void *) fullP);
        }
#       endif
        while ( tree_barrier_load_full( fullP) != node->trnmDataInit [ currSense ].full )
        {
#       ifdef YIELD_SPINNING
            spinning_thread_yield( );
#       endif
#       ifdef PAUSE_SPINNING
            spinning_pause( );
#       endif
#       ifdef WFE_SPINNING
            spinning_thread_wfe_wait( );
#       endif
        }
#   endif
#endif /* TRNM_BARRIER */
#ifdef T_GLOBAL_SENSE
#   ifdef COMBINED_BARRIER
        node->count = node->threadsNum;
#   endif
#endif
        if ( node->parent )
        {
            tree_barrier_wait( tree_barrier, tlsData, node->parent);
        }
#ifdef T_GLOBAL_SENSE
        tree_barrier_set_sense( tree_barrier->sense, currSense);
#endif
#ifdef T_LOCAL_SENSE
#   ifdef COMBINED_BARRIER
        tree_barrier_set_count( & (node->count), node->threadsNum);
#   endif
        tree_barrier_set_sense( node->sense, currSense);
#endif
#ifdef WFE_SPINNING
        spinning_thread_wfe_send( );
#endif
    } else
    {
#ifdef T_GLOBAL_SENSE
#   ifdef WFE_SPINNING
        if ( currSense != tree_barrier_load_sense( tree_barrier->sense) ) 
        {
            spinning_thread_wfe_init( (void *) tree_barrier->sense);
        }
#   endif
        while ( currSense != tree_barrier_load_sense( tree_barrier->sense) ) 
#endif
#ifdef T_LOCAL_SENSE
#   ifdef WFE_SPINNING
        if ( currSense != tree_barrier_load_sense( node->sense) ) 
        {
            spinning_thread_wfe_init( (void *) node->sense);
        }
#   endif
        while ( currSense != tree_barrier_load_sense( node->sense) )
#endif
        {
#ifdef YIELD_SPINNING
            spinning_thread_yield( );
#endif
#ifdef PAUSE_SPINNING
            spinning_pause( );
#endif
#ifdef WFE_SPINNING
            spinning_thread_wfe_wait( );
#endif
        };
    }
    tree_barrier_set_sense( senseP, !currSense);
}
#endif /* TREE_BARRIER */

#if defined( DSMN_BARRIER) || defined( DSMNH_BARRIER)
static void
dsmn_barrier_init( dsmn_barrier_t * dsmn_barrier,
                   tls_DataSet_t * dsmn_barrier_tls_data_set,
                   int threadsNum,
                   int barrierId)
{
    int id;
    int l;
    int ceilLog2ThreadsNum = (threadsNum > 1) ? math_log2Ceil( threadsNum) : 1;

    dsmn_barrier->barrierId = barrierId;
    dsmn_barrier->ceilLog2ThreadsNum = ceilLog2ThreadsNum;
    for ( id = 0; id < threadsNum; id++ )
    {
        for ( l = 0; l < ceilLog2ThreadsNum; l++ )
        {
            int threadAllocId;
            parity_t p;
            int partnerId = (id + (1 << l)) % threadsNum;

            for ( p = PARITY_EVEN; p < PARITY_NUM; p++ )
            {
                dsmn_barrier_tls_data_set->tlsData [ id ]->my_flags [ p ] [ l ] [ barrierId ] =
                    bar_MemAlloc( sizeof( tls_Sense_t), id, partnerId, & threadAllocId);
                dsmn_barrier_tls_data_set->tlsData [ id ]->my_flags [ p ] [ l ] [ barrierId ]->data = FALSE;
            }
        }
        dsmn_barrier_tls_data_set->tlsData [ id ]->parity [ barrierId ].data = PARITY_EVEN;
        dsmn_barrier_tls_data_set->tlsData [ id ]->sense [ barrierId ].data = TRUE;
    }

    for ( id = 0; id < threadsNum; id++ )
    {
        for ( l = 0; l < ceilLog2ThreadsNum; l++ )
        {
            parity_t p;
            int partnerId = (id + (1 << l)) % threadsNum;

            for ( p = PARITY_EVEN; p < PARITY_NUM; p++ )
            {
                dsmn_barrier_tls_data_set->tlsData [ id ]->partner_flags [ p ] [ l ] [ barrierId ] = 
                    dsmn_barrier_tls_data_set->tlsData [ partnerId ]->my_flags [ p ] [ l ] [ barrierId ];
            }
        }
    }
}

static inline void
dsmn_barrier_store_bool( volatile bool * addr,
                         bool data)
{
#ifdef ARCH_STORE_NR_NGO
    store_nr_ngo_bool( (void *) addr, data);
#else
#   ifdef ARCH_STORE_NR
    store_nr_bool( (void *) addr, data);
#   else
    (* addr) = data;
#   endif 
#endif
}

static inline bool
dsmn_barrier_load_bool( volatile bool * addr)
{
#ifdef ARCH_LOAD_NC
    return load_nc_bool( (void *) addr);
#else
    return (* addr);
#endif
}


static inline void
dsmn_barrier_store_sense( volatile bool * sense_addr, 
                          bool sense)
{
    dsmn_barrier_store_bool( sense_addr, sense);
}

static inline void
dsmn_barrier_set_tls_sense( volatile bool * sense_addr,
                            bool sense)
{
    dsmn_barrier_store_bool( sense_addr, sense);
}

static inline void
dsmn_barrier_set_tls_parity( volatile bool * sense_addr,
                             bool parity )
{
    dsmn_barrier_store_bool( sense_addr, parity);
}

static inline bool
dsmn_barrier_load_sense( volatile bool * sense_addr)
{
    return dsmn_barrier_load_bool( sense_addr);
}


static inline void
dsmn_barrier_wait( dsmn_barrier_t * dsmn_barrier,
                   tls_Data_t * dsmn_barrier_tls_data)
{
    int l = 0;
    int barrierId = dsmn_barrier->barrierId;
    int ceilLog2ThreadsNum = dsmn_barrier->ceilLog2ThreadsNum;
    bool s = dsmn_barrier_tls_data->sense [ barrierId ].data;
    parity_t p = dsmn_barrier_tls_data->parity [ barrierId ].data;

    for ( l = 0; l < ceilLog2ThreadsNum; l++ )
    {
        dsmn_barrier_store_sense( & (dsmn_barrier_tls_data->partner_flags [ p ] [ l ] [ barrierId ]->data), s);
#ifdef WFE_SPINNING
        spinning_thread_wfe_send( );
#endif
#ifdef WFE_SPINNING
        if ( dsmn_barrier_load_sense( & (dsmn_barrier_tls_data->my_flags [ p ] [ l ] [ barrierId ]->data)) != s )
        {
            spinning_thread_wfe_init( 
                (void *) & (dsmn_barrier_tls_data->my_flags [ p ] [ l ] [ barrierId ]->data));
        }
#endif
        while ( dsmn_barrier_load_sense( & (dsmn_barrier_tls_data->my_flags [ p ] [ l ] [ barrierId ]->data)) != s )
        {
#ifdef YIELD_SPINNING
            spinning_thread_yield( );
#endif
#ifdef PAUSE_SPINNING
            spinning_pause( );
#endif
#ifdef WFE_SPINNING
            spinning_thread_wfe_wait( );
#endif
        }
    }
    if ( p == PARITY_ODD )
    {
        dsmn_barrier_set_tls_sense( & (dsmn_barrier_tls_data->sense [ barrierId ].data), !s);
    }
    dsmn_barrier_set_tls_parity( & (dsmn_barrier_tls_data->parity [ barrierId ].data), PARITY_ODD - p);
}
#endif /* DSMN_BARRIER || DSMNH_BARRIER */

#ifdef DSMNH_BARRIER
static void
dsmnh_barrier_init( tls_DataSet_t * tlsDataSet,
                    int threadsNum,
                    int barrierId) 
{
    if ( machineDescription.machineId != UNDEFINED_MACHINE_ID )
    {
        int so, nu, co;
        tpl_NodeType_t innermostAvoidConnectionsTplLevel;
        int activeDsmnThreadsNum = 0, threadId = 0;

        for ( innermostAvoidConnectionsTplLevel = TPL_NODE_SOCKET;
              innermostAvoidConnectionsTplLevel < TPL_NODES_DEFINED_NUM;
              innermostAvoidConnectionsTplLevel ++ )
        {
            if ( !machineDescription.summary->avoidInterNodeConnections [ innermostAvoidConnectionsTplLevel ] )
                break;
        }
        innermostAvoidConnectionsTplLevel--;
        if ((innermostAvoidConnectionsTplLevel != TPL_NODE_NUMANODE) && (innermostAvoidConnectionsTplLevel != TPL_NODE_CORE)) 
        {
            /* FIXME: Not implemented */
            bar_InternalError( __FILE__, __LINE__);
        }
        for ( so = 0; 
              so < machineDescription.summary->socketsPerMachineNum;
              so++ )
        {
            for ( nu = 0;
                  nu < machineDescription.summary->numaNodesPerSocketNum; 
                  nu++ )
            {
                if (innermostAvoidConnectionsTplLevel == TPL_NODE_NUMANODE)
                {
                    int activeThreadsNum = machineDescription.sockets [ so ].numaNodes [ nu ].activeThreadsNum;
                    if ( activeThreadsNum > 0 )
                    {
                        int k;
                        sr_barrier_init( & bar_srBarrier [ barrierId ][ activeDsmnThreadsNum ], NULL, activeThreadsNum, barrierId);
                        for ( k = 0; k < activeThreadsNum; k++ )
                        {
                            tlsDataSet->tlsData [ threadId + k ]->dsmnTlsData = tlsDataSet->tlsData [ activeDsmnThreadsNum ];
                            tlsDataSet->tlsData [ threadId + k ]->groupId = activeDsmnThreadsNum;
                        }
                        activeDsmnThreadsNum ++;
                        threadId += activeThreadsNum;
                    }
                } else  
                {
                    bar_Assert( innermostAvoidConnectionsTplLevel == TPL_NODE_CORE);
                    for ( co = 0;
                          co < machineDescription.summary->coresPerNumaNodeNum; 
                          co++ )
                    {
                        int activeThreadsNum = machineDescription.sockets [ so ].numaNodes [ nu ].cores [ co ].activeThreadsNum;
                        if ( activeThreadsNum > 0 )
                        {
                            int k;
                            sr_barrier_init( & bar_srBarrier [ barrierId ][ activeDsmnThreadsNum ], NULL, activeThreadsNum, barrierId);
                            for ( k = 0; k < activeThreadsNum; k++ )
                            {
                                tlsDataSet->tlsData [ threadId + k ]->dsmnTlsData = tlsDataSet->tlsData [ activeDsmnThreadsNum ];
                                tlsDataSet->tlsData [ threadId + k ]->groupId = activeDsmnThreadsNum;
                            }
                            activeDsmnThreadsNum ++;
                            threadId += activeThreadsNum;
                        }
                    }
                }
            }
        }
        dsmn_barrier_init( & bar_dsmnBarrier [ barrierId ], tlsDataSet, activeDsmnThreadsNum, barrierId);
    } else {
        int i;
        for ( i = 0; i < threadsNum; i++) 
        {
            sr_barrier_init( & bar_srBarrier [ barrierId ][ i ], NULL, 1, barrierId);
            tlsDataSet->tlsData [ i ]->dsmnTlsData = tlsDataSet->tlsData [ i ];
            tlsDataSet->tlsData [ i ]->groupId = i;
        }
        dsmn_barrier_init( & bar_dsmnBarrier [ barrierId ], tlsDataSet, threadsNum, barrierId);
    }
}
#endif /* DSMNH_BARRIER */

static inline void
bar_BarrierTlsDataInit( int barrierId,
                        tls_Data_t * tlsData)
{
#ifdef SR_BARRIER
    tlsData->sense [ barrierId ].data = !bar_srBarrier [ barrierId ].sense;
#endif
#ifdef TREE_BARRIER
#   ifdef T_GLOBAL_SENSE
    tlsData->sense [ barrierId ].data = !(* bar_treeBarrier [ barrierId ].sense);
#   endif
#   ifdef T_LOCAL_SENSE
    tlsData->sense [ barrierId ].data = !(* bar_treeBarrier [ barrierId ].inodes [ 0 ]->sense);
#   endif
#endif
#ifdef DSMNH_BARRIER
    tlsData->senseSR [ barrierId ][ tlsData->groupId ].data = !bar_srBarrier [ barrierId ][ tlsData->groupId ].sense;
#endif
}

static inline void
bar_BarrierWait( int barrierId,
                 tls_Data_t * tlsData)
{
    exp_Stage_t expStage = tlsData->expInfo->expStage;
        
    if ( expStage == EXP_STAGE_REF )
        return;
#ifdef OMP_BARRIER
#pragma omp barrier
#endif
#ifdef PTHREAD_BARRIER
    pthread_barrier_wait( & bar_pthreadBarrier [ barrierId ]);
#endif
#ifdef SR_BARRIER
    sr_barrier_wait( & bar_srBarrier [ barrierId ], tlsData);
#endif
#ifdef TREE_BARRIER
    tree_barrier_wait( & bar_treeBarrier [ barrierId ], tlsData, 
                       bar_treeBarrier [ barrierId ].leaves [ tlsData->leafId ]);
#endif
#ifdef DSMN_BARRIER
    dsmn_barrier_wait( & bar_dsmnBarrier [ barrierId ], tlsData);
#endif
#ifdef DSMNH_BARRIER
    sr_barrier_wait( & bar_srBarrier [ barrierId ][ tlsData->groupId ], tlsData);
#endif
}

static void
bar_StartTimer( tls_Data_t * tlsData)
{
    if ( tlsData->threadId != 0 )
        return;

    exp_Timer_t * timer = & tlsData->expInfo->timer [ tlsData->expInfo->expStage ];
    while ( clock_gettime( timer->clockId, & timer->startTime) )
    {
        ;
    }
}

static void
bar_StopTimer( tls_Data_t * tlsData)
{
    if ( tlsData->threadId != 0 )
        return;

    exp_Timer_t * timer = & tlsData->expInfo->timer [ tlsData->expInfo->expStage ];
    while ( clock_gettime( timer->clockId, & timer->stopTime) )
    {
        ;
    }
    timer->deltaTime = ((long long int)(timer->stopTime.tv_sec - timer->startTime.tv_sec)) * NANOSEC_IN_SEC +
        (long long int)(timer->stopTime.tv_nsec - timer->startTime.tv_nsec);
}

#ifdef TMPL_BENCHMARK
static void *
test_barrier_tmpl( tls_Data_t * tlsData)
{
    bar_BarrierTlsDataInit( 0, tlsData);

    bar_BarrierWait( 0, tlsData);
    bar_StartTimer( tlsData);

    bar_StopTimer( tlsData);

    return NULL;
}
#endif /* TMPL_BENCHMARK */

#ifdef PURE_BENCHMARK
/* 'volatile' is used to prevent compiler from elimination of the loop */
static inline void
#define DUMMY_LOOP_ITERATIONS 16
test_Delay( )
{
    volatile int i;

    for ( i = 0; i < DUMMY_LOOP_ITERATIONS; i++ )
    {
        ;
    }
}
#endif /* PURE_BENCHMARK */

#ifdef LDIMBL_BENCHMARK
#define IMBALANCE_FACTOR 25
/* 'volatile' is used to prevent compiler from elimination of the loop */
static inline void
test_LoadImbalance( volatile int threadId)
{
    volatile int i;

    for ( i = 0; i < IMBALANCE_FACTOR * threadId; i++ )
    {
        ;
    }
}
#endif /* LDIMBL_BENCHMARK */

#if defined( PURE_BENCHMARK) || defined( LDIMBL_BENCHMARK)
static void *
test_barrier_pure( tls_Data_t * tlsData)
{
    int loBarNum = tlsData->expInfo->loBarNum;
    int hiBarNum = tlsData->expInfo->hiBarNum;
#ifdef LDIMBL_BENCHMARK
    volatile int i = tlsData->threadId;
#endif
    int j;

    bar_BarrierTlsDataInit( 0, tlsData);
    
    bar_BarrierWait( 0, tlsData);
    bar_StartTimer( tlsData);

    for ( j = loBarNum; j <= hiBarNum; j = j + 1 )
    {
#ifdef PURE_BENCHMARK
        test_Delay( );
#endif
#ifdef LDIMBL_BENCHMARK
        test_LoadImbalance( i);
#endif
        bar_BarrierWait( 0, tlsData);
    }

    bar_StopTimer( tlsData);

    return NULL;
}
#endif /* PURE_BENCHMARK */

#ifdef NBODY_BENCHMARK
static void
bar_InitNbody( exp_Info_t * expInfo)
{
    int i;
    int particlesNum = expInfo->curThreadsNum;

    for ( i = 0; i < particlesNum; i ++ )
    {
        bar_ParticlesBuf [ i ].x = ((float) rand( ) / (float)(RAND_MAX)) * 2.0 - 1.0;
        bar_ParticlesBuf [ i ].y = ((float) rand( ) / (float)(RAND_MAX)) * 2.0 - 1.0;
        bar_ParticlesBuf [ i ].z = ((float) rand( ) / (float)(RAND_MAX)) * 2.0 - 1.0;
        bar_ParticlesBuf [ i ].Vx = ((float) rand( ) / (float)(RAND_MAX)) * 2.0 - 1.0;
        bar_ParticlesBuf [ i ].Vy = ((float) rand( ) / (float)(RAND_MAX)) * 2.0 - 1.0;
        bar_ParticlesBuf [ i ].Vz = ((float) rand( ) / (float)(RAND_MAX)) * 2.0 - 1.0;
    }
}

static void *
test_barrier_nbody( tls_Data_t * tlsData)
{
    int loBarNum = tlsData->expInfo->loBarNum;
    int hiBarNum = tlsData->expInfo->hiBarNum;
    int threadsNum = tlsData->expInfo->curThreadsNum;
    int deltaBarNum = 2;
    const float dt = 0.01f;
    int i = tlsData->threadId;
    int j;
    int k;

    bar_BarrierTlsDataInit( 0, tlsData);
    bar_BarrierTlsDataInit( 1, tlsData);

    bar_BarrierWait( 0, tlsData);
    bar_StartTimer( tlsData);

    for ( j = 0; j <= (hiBarNum - loBarNum); j = j + deltaBarNum )
    {
        float Fx = 0.0f; 
        float Fy = 0.0f; 
        float Fz = 0.0f;
        for ( k = 0; k < threadsNum; k ++ ) 
        {
            if ( k != i ) 
            {
                const float dx = bar_ParticlesBuf [ k ].x - bar_ParticlesBuf [ i ].x;
                const float dy = bar_ParticlesBuf [ k ].y - bar_ParticlesBuf [ i ].y;
                const float dz = bar_ParticlesBuf [ k ].z - bar_ParticlesBuf [ i ].z;
                const float dr_pow_plus_2 = dx * dx + dy * dy + dz * dz;
                const float dr_pow_min_3_2 = NBODY_CONST / ( dr_pow_plus_2 * sqrtf( dr_pow_plus_2));

                Fx += dx * dr_pow_min_3_2;
                Fy += dy * dr_pow_min_3_2;
                Fz += dz * dr_pow_min_3_2;
            }
        }
        bar_ParticlesBuf [ i ].Vx += dt * Fx;
        bar_ParticlesBuf [ i ].Vy += dt * Fy;
        bar_ParticlesBuf [ i ].Vz += dt * Fz;
        bar_BarrierWait( 0, tlsData);

        bar_ParticlesBuf [ i ].x += bar_ParticlesBuf [ i ].Vx * dt; 
        bar_ParticlesBuf [ i ].y += bar_ParticlesBuf [ i ].Vy * dt;
        bar_ParticlesBuf [ i ].z += bar_ParticlesBuf [ i ].Vz * dt;
        bar_BarrierWait( 1, tlsData);
    }

    bar_StopTimer( tlsData);

    return NULL;
}
#endif /* NBODY_BENCHMARK */

#ifdef SANITY_BENCHMARK
static void
bar_InitTestArray( exp_Info_t * expInfo)
{
    int i;
    int threadsNum = expInfo->curThreadsNum;
    
    for ( i = 0; i < threadsNum; i ++)
    {
        bar_TestArray [ i ] = bar_MemAlloc( sizeof( int), i, i, NULL);
    }
}

static void *
test_barrier_sanity( tls_Data_t * tlsData)
{
    exp_Stage_t expStage =  tlsData->expInfo->expStage;
    int loBarNum = tlsData->expInfo->loBarNum;
    int hiBarNum = tlsData->expInfo->hiBarNum;
    int threadsNum = tlsData->expInfo->curThreadsNum;
    int deltaBarNum = threadsNum * 2 + 1;
    int i = tlsData->threadId;
    int j;
    int k;

    bar_BarrierTlsDataInit( 0, tlsData);
    bar_BarrierTlsDataInit( 1, tlsData);
    bar_BarrierTlsDataInit( 2, tlsData);

    bar_BarrierWait( 0, tlsData);
    bar_StartTimer( tlsData);

    for ( j = 0; j <= (hiBarNum - loBarNum); j = j + deltaBarNum )
    {

        (* bar_TestArray [ i ]) = 0;

        for ( k = 0; k < threadsNum; k++ )
        {
            int t;

            bar_BarrierWait( 0, tlsData);
            t = (* bar_TestArray [ (i + j + k) % threadsNum ]);
            bar_BarrierWait( 1, tlsData);
#ifndef NDEBUG
            printf( " [%d] -> [%d] val: %d\n", i, (i + j + k) % threadsNum, t);
#endif
            (* bar_TestArray [ i ]) = t + 1;
        }
        bar_BarrierWait( 2, tlsData);
#ifndef NDEBUG
        printf( "res id: %d val: %d\n", i, (* bar_TestArray [ i ]));
#endif
        if ( (* bar_TestArray [ i ]) != threadsNum )
        {
            if ( tlsData->expInfo->expStage == EXP_STAGE_EXP )
            {
                bar_InternalError( __FILE__, __LINE__);
            }
        }
    }

    bar_StopTimer( tlsData);

    return NULL;
}
#endif /* SANITY_BENCHMARK */

static inline void
bar_BarriersInit( exp_Info_t * expInfo,
                  tls_DataSet_t * tlsDataSet)
{
    int j;
#if defined( TREE_BARRIER)
    int radix = expInfo->curRadixNum;
#endif
    int threadsNum = expInfo->curThreadsNum;

    for ( j = 0; j < BARRIERS_MAX_NUM; j++ )
    {
#ifdef PTHREAD_BARRIER
        pthread_barrier_init( & bar_pthreadBarrier [ j ], NULL, threadsNum);
#endif

#ifdef SR_BARRIER
        sr_barrier_init( & bar_srBarrier [ j ], NULL, threadsNum, j);
#endif

#ifdef TREE_BARRIER
        tree_barrier_init( & bar_treeBarrier [ j ], tlsDataSet, threadsNum, radix, j, 0);
#endif

#ifdef DSMN_BARRIER
        dsmn_barrier_init( & bar_dsmnBarrier [ j ], tlsDataSet, threadsNum, j);
#endif

#ifdef DSMNH_BARRIER
        dsmnh_barrier_init( tlsDataSet, threadsNum, j);
#endif
    }
}

static inline void
bar_InitMachineActivity( )
{
    int so, nu, co, pu;

    machineDescription.activeThreadsNum = 0;
    for ( so = 0;
          so < machineDescription.summary->socketsPerMachineNum;
          so ++ )
    {
        machineDescription.sockets [ so ].activeThreadsNum = 0;
        for ( nu = 0;
              nu < machineDescription.summary->numaNodesPerSocketNum;
              nu ++ )
        {
            machineDescription.sockets [ so ].numaNodes [ nu ].activeThreadsNum = 0;
            machineDescription.sockets [ so ].numaNodes [ nu ].allocatedInodesNum = 0;
            for ( co = 0;
                  co < machineDescription.summary->coresPerNumaNodeNum;
                  co ++ )
            {
                machineDescription.sockets [ so ].numaNodes [ nu ].cores [ co ].activeThreadsNum = 0;
                for ( pu = 0;
                      pu < machineDescription.summary->pusPerCoreNum;
                      pu ++ )
                {
                    machineDescription.sockets [ so ].numaNodes [ nu ].cores [ co ].pus [ pu ]->activeThreadsNum = 0;
                    machineDescription.sockets [ so ].numaNodes [ nu ].cores [ co ].pus [ pu ]->firstThreadData = 0;
                    machineDescription.sockets [ so ].numaNodes [ nu ].cores [ co ].pus [ pu ]->lastThreadData = 0;
                }
            }
        }
    }
}

static inline void
bar_IncrementMachineActivity( unsigned so, 
                              unsigned nu,
                              unsigned co,
                              unsigned pu,
                              unsigned activeThreadsNum)
{
    machineDescription.activeThreadsNum += activeThreadsNum;
    machineDescription.sockets [ so ].activeThreadsNum += activeThreadsNum;
    machineDescription.sockets [ so ].numaNodes [ nu ].activeThreadsNum += activeThreadsNum;
    machineDescription.sockets [ so ].numaNodes [ nu ].cores [ co ].activeThreadsNum += activeThreadsNum;
    machineDescription.sockets [ so ].numaNodes [ nu ].cores [ co ].pus [ pu ]->activeThreadsNum += activeThreadsNum;
}

static void
tpl_AddThreadDataToPU( tls_Data_t * addedData,
                       tpl_PU_t * pu)
{
    if ( pu->lastThreadData == NULL )
    {
        pu->firstThreadData = addedData;
        pu->lastThreadData = addedData;
    } else
    {
        pu->lastThreadData->nextThreadData = addedData;
        pu->lastThreadData = addedData;
    }
}

static inline void
bar_TlsDataFini( void)
{
}

static inline void
bar_TlsDataAlloc( tls_DataSet_t * tlsDataSet, 
                  int threadId,
                  tpl_PU_t * curPU)
{
    tlsDataSet->tlsData [ threadId ] = (tls_Data_t *) bar_MemAlloc( sizeof( tls_Data_t), threadId, threadId, NULL);
    tlsDataSet->tlsData [ threadId ]->curPU = curPU;
}

static inline void
bar_TlsDataNew( exp_Info_t * expInfo,
                tls_DataSet_t * tlsDataSet,
                int threadId,
                tpl_PU_t * curPU)
{
    bar_ThreadIdToPUMap [ threadId ] = curPU;
    bar_TlsDataAlloc( tlsDataSet, threadId, curPU);
    tlsDataSet->tlsData [ threadId ]->threadId = threadId;
    tlsDataSet->tlsData [ threadId ]->expInfo = expInfo;
    tlsDataSet->tlsData [ threadId ]->nextThreadData = NULL;
    if ( curPU != NULL )
    {
        tpl_AddThreadDataToPU( tlsDataSet->tlsData [ threadId ], curPU);
    }
}

static inline void
bar_TlsDataInit( exp_Info_t * expInfo,
                 tls_DataSet_t * tlsDataSet)
{
    int i, so, nu, co, pu;
    int puMax, coresNum, pusNum, pusPerCoreNum, coresPassed;
    int threadsNum = expInfo->curThreadsNum;
    unsigned machineId = machineDescription.machineId;
    unsigned osIdActivity [ PUS_PER_MACHINE_MAX_NUM ];

    if ( machineId == UNDEFINED_MACHINE_ID )
    {
        for ( i = 0; i < threadsNum; i++ )
        {
            bar_TlsDataNew( expInfo, tlsDataSet, i, NULL);
        }
        return;
    }

    bar_InitMachineActivity( );
    for ( i = 0; i < PUS_PER_MACHINE_MAX_NUM; i++ )
    {
        osIdActivity [ i ] = 0;
    }
    if ( USER_DEFINED_ACTIVE_PUS_SELECTION == TRUE )
    {
        int j;
        int threadId = 0;
        int threadsNum = expInfo->curThreadsNum;
        cpu_set_t onlineCpuSet;

        CPU_ZERO( &onlineCpuSet);
        sys_SetOnlineCpuSet( &onlineCpuSet);

        while( threadId < threadsNum )
        {
            for ( j = 0; j < CPU_MAP_PRIORITY_DELTA; j++ )
            {
                for ( i = j; 
                      (i < sizeof( cpu_set_t) * BITS_IN_BYTE) && threadId < threadsNum;
                      i = i + CPU_MAP_PRIORITY_DELTA )
                {
                    tpl_PU_t * curPU;

                    if ( !CPU_ISSET( i, &onlineCpuSet) )
                        continue;

                    curPU = bar_OsIdToPUMap [ i ];
                    bar_IncrementMachineActivity( curPU->socketId, curPU->numaNodeId, curPU->coreId, curPU->puId, 1);
                    if ( TOPOLOGY_AWARE_MAPPING == TRUE )
                    {
                        osIdActivity [ i ] ++;
                    } else
                    {
                        bar_TlsDataNew( expInfo, tlsDataSet, threadId, curPU);
                    }
                    threadId ++;
                }
            }
        }
        threadId = 0;
        if ( TOPOLOGY_AWARE_MAPPING == TRUE )
        {
            for ( so = 0;
                  so < machineDescription.summary->socketsPerMachineNum;
                  so ++ )
            {
                for ( nu = 0;
                      nu < machineDescription.summary->numaNodesPerSocketNum;
                      nu ++ )
                {
                    for ( co = 0;
                          co < machineDescription.summary->coresPerNumaNodeNum;
                          co ++ )
                    {
                        for ( pu = 0;
                              pu < machineDescription.summary->pusPerCoreNum;
                              pu ++ )
                        {
                            tpl_PU_t * curPU = & tpl_PUDescriptionsSet [ machineId ] [ so ] [ nu ] [ co ] [ pu ];
                        
                            while ( osIdActivity [ curPU->osId ] -- )
                            {
                                bar_TlsDataNew( expInfo, tlsDataSet, threadId, curPU);
                                threadId ++;
                            }
                        }
                    }
                }
            }
        }
    } else
    {
        /* automatic threads-to-PUs mapping */
        switch ( machineDescription.summary->topologyType )
        {
        case TPL_TYPE_HOMOGENEOUS_SYMMETRIC:
        {
            coresNum = machineDescription.summary->socketsPerMachineNum *
                       machineDescription.summary->numaNodesPerSocketNum *  
                       machineDescription.summary->coresPerNumaNodeNum;
            pusPerCoreNum = machineDescription.summary->pusPerCoreNum;
            pusNum = coresNum * pusPerCoreNum;
            i = 0;
            coresPassed = 0;
            while ( i < threadsNum )
            {
                for ( so = 0;
                      so < machineDescription.summary->socketsPerMachineNum;
                      so ++ )
                {
                    for ( nu = 0;
                          nu < machineDescription.summary->numaNodesPerSocketNum;
                          nu ++ )
                    {
                        for ( co = 0;
                              co < machineDescription.summary->coresPerNumaNodeNum;
                              co ++ )
                        {
                            int pusPerCoreMax = (((machineDescription.summary->smtType == TPL_SMT_HT) || 
                                                  (machineDescription.summary->smtType == TPL_SMT_MIC)) && (threadsNum <= pusNum)) ?
                                                    ((threadsNum / coresNum) + (((threadsNum - i) % (coresNum - coresPassed)) > 0)) :
                                                    machineDescription.summary->pusPerCoreNum;
                            coresPassed ++;
                            for ( pu = 0;
                                  pu < pusPerCoreMax;
                                  pu ++ )
                            {
                                int repPu = (threadsNum > pusNum) * (((threadsNum - pusNum) / pusNum) + (i < (threadsNum % pusNum)));
                               
                                bar_IncrementMachineActivity( so, nu, co, pu, repPu + 1);
                                do 
                                {
                                    bar_TlsDataNew( expInfo, tlsDataSet, i, 
                                        & tpl_PUDescriptionsSet [ machineId ] [ so ] [ nu ] [ co ] [ pu ]);
                                    i++;
                                    if ( i == threadsNum )
                                    {
                                        if ( repPu ) 
                                        {
                                            bar_InternalError( __FILE__, __LINE__);
                                        }
                                        goto exitAutoMapping;
                                    }
                                } while ( repPu-- );
                            }
                        }
                    }
                }
            }
            exitAutoMapping: ;
            break;
        }
        case TPL_TYPE_HOMOGENEOUS_ASYMMETRIC:
        case TPL_TYPE_HETEROGENEOUS:
        default:
            bar_InternalError( __FILE__, __LINE__);
        }
    }
}

#ifdef OMP_BARRIER
static inline
bar_SetOMPThreadAffinity( exp_Info_t * expInfo,
                          int threadId)
                          
{
    int osId;
#   ifdef OMP_INTEL
    int ret;
    kmp_affinity_mask_t mask;

    osId = bar_ThreadIdToOsIdMap [ expInfo->curThreadsNum - 1 ] [ threadId ];
    kmp_create_affinity_mask( &mask); 
    kmp_set_affinity_mask_proc( osId, & mask);
    ret = kmp_set_affinity( & mask);
    if ( ret )
    {
        bar_InternalError( __FILE__, __LINE__);
    }
#   endif /* OMP_INTEL */
#   ifdef OMP_GOMP
    /* FIXME */
#   endif 
}
#else /* OMP_BARRIER */
static inline
bar_SetPthreadAffinity( exp_Info_t * expInfo,
                        int threadId, 
                        pthread_attr_t * pthreadAttr)
{
    int ret;
    cpu_set_t currCpuSet;
    int osId;

    osId = bar_ThreadIdToOsIdMap [ expInfo->curThreadsNum - 1 ] [ threadId ];
    CPU_ZERO( &currCpuSet);
    CPU_SET( osId, &currCpuSet);

    ret = pthread_attr_init( & pthreadAttr[ threadId ]);
    if ( ret )
    {
        bar_InternalError( __FILE__, __LINE__);
    }
    pthread_attr_setaffinity_np( & pthreadAttr[ threadId ], sizeof( currCpuSet), &currCpuSet);
}
#endif /* !OMP_BARRIER */

static inline void
bar_SetThreadAffinityHelperByThreadIdToPUMap( exp_Info_t * expInfo,
#ifdef OMP_BARRIER
                                              int ompThreadNum
#else
                                              pthread_attr_t * pthreadAttr
#endif
                                        )
{
    int threadId = 0;
    
    while( threadId < expInfo->curThreadsNum )
    {
        bar_ThreadIdToOsIdMap [ expInfo->curThreadsNum - 1 ] [ threadId ] = 
            bar_ThreadIdToPUMap [ threadId ]->osId;
#ifdef OMP_BARRIER
        bar_SetOMPThreadAffinity( expInfo, threadId);
#else
        bar_SetPthreadAffinity( expInfo, threadId, pthreadAttr);
#endif
        threadId++;
    }
}

static inline void
bar_SetThreadAffinityHelperDefault( exp_Info_t * expInfo,
#ifdef OMP_BARRIER
                                    int ompThreadNum
#else
                                    pthread_attr_t * pthreadAttr
#endif
                                  )
{
    int i, j;
    int threadId = 0;
    int threadsNum = expInfo->curThreadsNum;

    cpu_set_t onlineCpuSet;
    CPU_ZERO( &onlineCpuSet);
    sys_SetOnlineCpuSet( &onlineCpuSet);

    while( threadId < threadsNum )
    {
        for ( j = 0; j < CPU_MAP_PRIORITY_DELTA; j++ )
        {
            for ( i = j; 
                  (i < sizeof( cpu_set_t) * BITS_IN_BYTE) && threadId < threadsNum;
                  i = i + CPU_MAP_PRIORITY_DELTA )
            {
                if ( !CPU_ISSET( i, &onlineCpuSet) )
                    continue;
#ifdef OMP_BARRIER
                {
                    if ( threadId == ompThreadNum )
                    {
                        bar_ThreadIdToOsIdMap [ expInfo->curThreadsNum - 1 ] [ threadId ] = i; 
                        bar_SetOMPThreadAffinity( expInfo, threadId);
                    }
                }
#else
                {
                    bar_ThreadIdToOsIdMap [ expInfo->curThreadsNum - 1 ] [ threadId ] = i; 
                    bar_SetPthreadAffinity( expInfo, threadId, pthreadAttr);
                }
#endif
                threadId++;
            }
        }
    }
}

static inline void
bar_SetThreadAffinityHelper( exp_Info_t * expInfo,
#ifdef OMP_BARRIER
                             int ompThreadNum
#else
                             pthread_attr_t * pthreadAttr
#endif
                             )
{
    if ( machineDescription.machineId == UNDEFINED_MACHINE_ID )
    {
        bar_SetThreadAffinityHelperDefault( expInfo,
#ifdef OMP_BARRIER
            ompThreadNum
#else
            pthreadAttr
#endif
            );
    } else
    {
        bar_SetThreadAffinityHelperByThreadIdToPUMap( expInfo,
#ifdef OMP_BARRIER
            ompThreadNum
#else
            pthreadAttr
#endif
            );
    }
}

#ifdef OMP_BARRIER
static inline void
bar_OmpSetThreadAffinity( exp_Info_t * expInfo,
                          int ompThreadNum)
{
    bar_SetThreadAffinityHelper( expInfo, ompThreadNum);
}
#else /* OMP_BARRIER */
static inline void
bar_PthreadAttrsInit( exp_Info_t * expInfo,
                      pthread_attr_t * pthreadAttr)
{
    bar_SetThreadAffinityHelper( expInfo, pthreadAttr);
}
#endif /* !OMP_BARRIER */ 

static inline void
bar_BarriersFini( void)
{
    int i;

    for ( i = 0; i < BARRIERS_MAX_NUM; i++ )
    {
#if defined( PTHREAD_BARRIER)
        pthread_barrier_destroy( & bar_pthreadBarrier [ i ]);
#endif
    }
}

static inline void
bar_PthreadAttrsFini( exp_Info_t * expInfo,
                      pthread_attr_t * pthreadAttr)
{
    int i;
    int ret;
    int threadsNum = expInfo->curThreadsNum;

    for ( i = 0; i < threadsNum; i++ )
    {
        ret = pthread_attr_destroy( & pthreadAttr[ i ]);
        bar_Assert( !ret);
    }
}

static inline void
bar_PthreadsFini( exp_Info_t * expInfo,
                 pthread_t * pthread)
{
    int i;
    int ret;
    int threadsNum = expInfo->curThreadsNum;

    for ( i = 0; i < threadsNum; i++ )
    {
        pthread_join( pthread [ i ], NULL);
        bar_Assert( !ret);
    }
}

static inline void
bar_CreateThreadsAndRunTest( tls_DataSet_t * tlsDataSet,
                             exp_Info_t * expInfo,
#ifndef OMP_BARRIER
                             pthread_t * pthread,
                             pthread_attr_t * pthreadAttr,
#endif
                             void * (* testFunc)(tls_Data_t *) )
{
    int i;
    int ret;
    int threadsNum = expInfo->curThreadsNum;

#ifdef OMP_BARRIER
#   pragma omp parallel num_threads( threadsNum)
    {
        i = omp_get_thread_num( );
        bar_OmpSetThreadAffinity( expInfo, i);
        testFunc( (void * __restrict__) tlsDataSet->tlsData [ i ]);
    }
#else
    for ( i = 0; i < threadsNum; i++ )
    {
        ret = pthread_create( & pthread [ i ], 
                              & pthreadAttr [ i ], 
                              (void * (*)(void *)) testFunc, 
                              (void * __restrict__) tlsDataSet->tlsData [ i ]);
        bar_Assert( !ret);
    }
#   ifndef NDEBUG
    printf( "Created number of pthreads: %i \n", threadsNum);
#   endif
#endif
}

static void
bar_TestBarrier( exp_Info_t * expInfo)
{
#   ifndef OMP_BARRIER
    pthread_t pthread [ THREADS_MAX_NUM ];
    pthread_attr_t pthreadAttr [ THREADS_MAX_NUM ];
#   endif
    tls_DataSet_t tlsDataSet;
    int threadsNum = expInfo->curThreadsNum;
    long int i, j, ret;

    bar_MemReuse( );
    bar_TlsDataInit( expInfo, & tlsDataSet);
#   ifdef SANITY_BENCHMARK
    bar_InitTestArray( expInfo);
#   endif
#   ifdef NBODY_BENCHMARK
    bar_InitNbody( expInfo);
#   endif
    bar_BarriersInit( expInfo, & tlsDataSet);
#   ifndef OMP_BARRIER
    bar_PthreadAttrsInit( expInfo, pthreadAttr);
#   endif

#ifdef TMPL_BENCHMARK

    bar_CreateThreadsAndRunTest( & tlsDataSet, expInfo,
#   ifndef OMP_BARRIER
                                 pthread, pthreadAttr, 
#   endif
                                 & test_barrier_tmpl);

#else /* TMPL_BENCHMARK */

#   ifdef NBODY_BENCHMARK
    bar_CreateThreadsAndRunTest( & tlsDataSet, expInfo,
#       ifndef OMP_BARRIER
                                 pthread, pthreadAttr,
#       endif
                                 & test_barrier_nbody);
#   endif


#   ifdef SANITY_BENCHMARK
    bar_CreateThreadsAndRunTest( & tlsDataSet, expInfo,
#       ifndef OMP_BARRIER
                                 pthread, pthreadAttr, 
#       endif
                                 & test_barrier_sanity);
#   endif

#   if defined( PURE_BENCHMARK) || defined( LDIMBL_BENCHMARK)
    bar_CreateThreadsAndRunTest( & tlsDataSet, expInfo,
#       ifndef OMP_BARRIER
                                 pthread, pthreadAttr,
#       endif
                                 & test_barrier_pure);
#   endif

#endif /* !TMPL_BENCHMARK */

#   ifndef OMP_BARRIER
    bar_PthreadsFini( expInfo, pthread);
    bar_PthreadAttrsFini( expInfo, pthreadAttr);
#   endif
    bar_BarriersFini( );
    bar_TlsDataFini( );

}

static void
bar_CheckPreconditions( )
{
    bar_Assert( BITS_IN_BYTE * sizeof( atomic_Data_t) == HW_ATOMIC_DATA_SIZE_IN_BITS);
#if !defined( ARCH_LL_SC) && !defined( ARCH_CAS) && !defined( ARCH_FETCH_AND_ADD)
    bar_Assert( 0);
#endif
}

#ifndef NDEBUG
static void
bar_PrintExperimentInfo( exp_Info_t * expInfo)
{
    printf( "Number of logical cpus: %i \n", CPUS_NUM);
#ifdef PTHREAD_BARRIER
    printf( "Test pthread barrier...\n");
#endif
#ifdef SR_BARRIER
    printf( "Test sense reversing barrier...\n");
#endif
#ifdef TREE_BARRIER
#   ifdef T_GLOBAL_SENSE
    printf( "Test combining barrier with global sense...\n");
#   endif
#   ifdef T_LOCAL_SENSE
    printf( "Test combining barrier with local sense...\n");
#   endif
#endif
#ifdef DSMN_BARRIER
    printf( "Test dsmn barrier...\n");
#endif
#ifdef DSMNH_BARRIER
    printf( "Test dsmnh barrier...\n");
#endif
}
#endif /* !NDEBUG */

static void
bar_PrintTableHeader( )
{
    printf( "%s", TABLE_HEADER);
    printf( "\n");
}

#ifdef DELAYED_PRINT
static void
bar_SaveTableLine( exp_Info_t * expInfo)
{
    int barriersNum;
    int radix;
    double timePerBarrier;
#ifdef PRINT_SYNCH_UNSYNCH_PHASE_TIME
    double timePerUnsynchronizedPhase;
#endif
    
#if defined( PURE_BENCHMARK) || defined( LDIMBL_BENCHMARK)
    barriersNum = expInfo->hiBarNum;
#endif
#ifdef NBODY_BENCHMARK
    barriersNum = expInfo->hiBarNum;
#endif 
#ifdef SANITY_BENCHMARK
    barriersNum = ((expInfo->hiBarNum + expInfo->curThreadsNum * 2) / (expInfo->curThreadsNum * 2 + 1)) *
        (expInfo->curThreadsNum * 2 + 1);
#endif
    timePerBarrier =
        (double) (expInfo->timer [ EXP_STAGE_EXP ].deltaTime - 
                  expInfo->timer [ EXP_STAGE_REF ].deltaTime) /
        (double) (barriersNum);
#ifdef PRINT_SYNCH_UNSYNCH_PHASE_TIME
    timePerUnsynchronizedPhase =
        (double) (expInfo->timer [ EXP_STAGE_REF ].deltaTime) /
        (double) (barriersNum);
#endif

    if ( expInfo->timer [ EXP_STAGE_EXP ].deltaTime < expInfo->timer [ EXP_STAGE_REF ].deltaTime )
    {
        bar_Assert( expInfo->curThreadsNum == 1);
        timePerBarrier = 0.0;
#ifdef PRINT_SYNCH_UNSYNCH_PHASE_TIME
        timePerUnsynchronizedPhase = 0.0;
#endif
    }
    bar_Assert( expInfo->currTableLine < EXP_LINES_NUM);
#if defined( TREE_BARRIER)
    radix = expInfo->curRadixNum;
#else
    radix = UNDEFINED_RADIX;
#endif
    expInfo->tableLines [ expInfo->currTableLine ].threadsNum = expInfo->curThreadsNum;
    expInfo->tableLines [ expInfo->currTableLine ].radix = radix;
    expInfo->tableLines [ expInfo->currTableLine ].timePerBarrier = timePerBarrier;
#ifdef PRINT_SYNCH_UNSYNCH_PHASE_TIME
    expInfo->tableLines [ expInfo->currTableLine ].timePerUnsynchronizedPhase = timePerUnsynchronizedPhase;
#endif
    expInfo->currTableLine++;
    
}

static void
bar_PrintAffinity( int curThreadsNum)
{
    int i;
    for ( i = 0; i < curThreadsNum; i++ )
    {
        if ( i != 0 )
        {
            printf(" ");
        }
        printf( "%d", bar_ThreadIdToOsIdMap [ curThreadsNum - 1 ] [ i ]);
    }
    printf( ",");
}

static void
bar_PrintTableLines( exp_Info_t * expInfo)
{
    int i = 0;
    
    for ( i = 0; i < expInfo->currTableLine; i++)
    {
        printf( "%s,", HOSTNAME_STR);
        printf( "%s,", ARCH_STR);
        printf( "%s,", EXP_ID_STR);
        printf( "%s,", BENCH_STR);
        printf( "%s,", BARRIER_STR);
        printf( "%d,", expInfo->tableLines [ i ].radix);
        printf( "%s,", SPINNING_STR);
        printf( "%d,", expInfo->tableLines [ i ].threadsNum);
        bar_PrintAffinity( expInfo->tableLines [ i ].threadsNum);
#ifdef TMPL_BENCHMARK
#   ifdef PRINT_SYNCH_UNSYNCH_PHASE_TIME
        printf( "%s,", TMPL_TIME);
        printf( "%8.2f,", 0.0);
#   endif
        printf( "%s\n", TMPL_TIME);
#else
#   ifdef PRINT_SYNCH_UNSYNCH_PHASE_TIME
        printf( "%8.2f,", expInfo->tableLines [ i ].timePerUnsynchronizedPhase + 
                          expInfo->tableLines [ i ].timePerBarrier);
        printf( "%8.2f,", expInfo->tableLines [ i ].timePerUnsynchronizedPhase);
#   endif
        printf( "  %8.2f\n", expInfo->tableLines [ i ].timePerBarrier);
#endif
    }
}
#endif /* DELAYED_PRINT */

#ifndef DELAYED_PRINT
static void
bar_PrintTableLine( exp_Info_t * expInfo)
{
    int barriersNum;
    double barOverhead;
#ifdef PRINT_SYNCH_UNSYNCH_PHASE_TIME
    double barUnsynchronziedPhaseTime;
#endif
    
#if defined( PURE_BENCHMARK) || defined( LDIMBL_BENCHMARK)
    barriersNum = expInfo->hiBarNum;
#endif
#ifdef SANITY_BENCHMARK
    barriersNum = ((expInfo->hiBarNum + expInfo->curThreadsNum * 2) / (expInfo->curThreadsNum * 2 + 1)) *
        (expInfo->curThreadsNum * 2 + 1);
#endif

    barOverhead = 
        (double) (expInfo->timer [ EXP_STAGE_EXP ].deltaTime - 
                  expInfo->timer [ EXP_STAGE_REF ].deltaTime) /
        (double) (barriersNum);
#ifdef PRINT_SYNCH_UNSYNCH_PHASE_TIME
    barUnsynchronziedPhaseTime = 
        (double) (expInfo->timer [ EXP_STAGE_REF ].deltaTime) /
        (double) (barriersNum);
#endif
    printf( "%s,", HOSTNAME_STR);
    printf( "%s,", ARCH_STR);
    printf( "%s,", EXP_ID_STR);
    printf( "%s,", BENCH_STR);
    printf( "%s,", BARRIER_STR);
#if defined( TREE_BARRIER)
    printf( "%d,", expInfo->curRadixNum);
#else
    printf( "%d,", UNDEFINED_RADIX);
#endif
    printf( "%s,", SPINNING_STR);
    printf( "%d,", expInfo->curThreadsNum);
    bar_PrintAffinity( expInfo->curThreadsNum);
#ifdef TMPL_BENCHMARK
#   ifdef PRINT_SYNCH_UNSYNCH_PHASE_TIME
    printf( "%s,", TMPL_TIME);
    printf( "%8.2f,", 0.0);
#   endif
    printf( "%s\n", TMPL_TIME);
#else
#   ifdef PRINT_SYNCH_UNSYNCH_PHASE_TIME
    printf( "%8.2f,", barOverhead + barUnsynchronziedPhaseTime);
    printf( "%8.2f,", barUnsynchronziedPhaseTime);
#   endif
    printf( "  %8.2f\n", barOverhead);
#endif
}
#endif /* !DELAYED_PRINT */

static void
bar_SetParentThreadAffinity( bar_ParentAffinity_t affinity)
{
    int i;
    int ret = 0;

    cpu_set_t onlineCpuSet;
    CPU_ZERO( &onlineCpuSet);
    sys_SetOnlineCpuSet( &onlineCpuSet);

    switch ( affinity )
    {
    case BAR_PARENT_AFFINITY_ONE:
    {
        i = sizeof( cpu_set_t) * BITS_IN_BYTE - 1;
        for ( ; ;)
        {
            cpu_set_t currCpuSet;
            
            if ( !CPU_ISSET( i, &onlineCpuSet) )
            {
                if ( i == 0 )
                {
                    bar_InternalError( __FILE__, __LINE__);
                }
                i--;
                continue;
            }
            CPU_ZERO( &currCpuSet);
            CPU_SET( i, &currCpuSet);
            ret = pthread_setaffinity_np( pthread_self( ), sizeof( cpu_set_t), &currCpuSet);
            break;
        }
        break;
    }
    case BAR_PARENT_AFFINITY_ALL:
        ret = pthread_setaffinity_np( pthread_self( ), sizeof( cpu_set_t), &onlineCpuSet);
        break;
    default:
        bar_Assert( 0);
    }
    if ( ret )
    {
        bar_InternalError( __FILE__, __LINE__);
    }
}

static void
bar_SetOnlineCpuSet( )
{
    CPU_ZERO( & bar_onlineCpuSet);
    /* FIXME: Get online cpu set properly using lsproc or its code */
    sched_getaffinity( 0, sizeof( cpu_set_t), & bar_onlineCpuSet);
}

static unsigned
bar_ResolveMachineIdByHostname( const char * hostname)
{
    int i;

    for ( i = 0; i < MACHINES_MAX_NUM; i++ )
    {
        if ( !strcmp( hostname, bar_MachineIdToHostnameMap [ i ]) )
        {
            return i;
        }
    }

    return UNDEFINED_MACHINE_ID;
}

static void
tpl_InitMachineTopology( )
{
    unsigned machineId = machineDescription.machineId;
    int so, nu, co, pu;

    if ( machineId == UNDEFINED_MACHINE_ID )
        return;

    switch ( machineDescription.summary->topologyType )
    {
    case TPL_TYPE_HOMOGENEOUS_SYMMETRIC:
    {
        tpl_SMT_t smtType = machineDescription.summary->smtType;

        for ( so = 0; 
              so < machineDescription.summary->socketsPerMachineNum;
              so++ )
        {
            for ( nu = 0;
                  nu < machineDescription.summary->numaNodesPerSocketNum; 
                  nu++ )
            {
                machineDescription.sockets [ so ].numaNodes [ nu ].osId = 
                    (so * machineDescription.summary->numaNodesPerSocketNum) + nu;
                for ( co = 0;
                      co < machineDescription.summary->coresPerNumaNodeNum; 
                      co++ )
                {
                    for ( pu = 0;
                          pu < machineDescription.summary->pusPerCoreNum; 
                          pu++ )
                    {
                        tpl_PUDescriptionsSet [ machineId ] [ so ] [ nu ] [ co ] [ pu ].smtType = smtType;
                        tpl_PUDescriptionsSet [ machineId ] [ so ] [ nu ] [ co ] [ pu ].machineId = machineId;
                        tpl_PUDescriptionsSet [ machineId ] [ so ] [ nu ] [ co ] [ pu ].socketId = so;
                        tpl_PUDescriptionsSet [ machineId ] [ so ] [ nu ] [ co ] [ pu ].numaNodeId = nu;
                        tpl_PUDescriptionsSet [ machineId ] [ so ] [ nu ] [ co ] [ pu ].coreId = co;
                        tpl_PUDescriptionsSet [ machineId ] [ so ] [ nu ] [ co ] [ pu ].puId = pu;
        
                        tpl_PUDescriptionsSet [ machineId ] [ so ] [ nu ] [ co ] [ pu ].numaOsId =
                            machineDescription.sockets [ so ].numaNodes [ nu ].osId;

                        machineDescription.sockets [ so ].numaNodes [ nu ].cores [ co ]. pus [ pu ] =
                            & tpl_PUDescriptionsSet [ machineId ] [ so ] [ nu ] [ co ] [ pu ];
                
                        bar_OsIdToPUMap [ tpl_PUDescriptionsSet [ machineId ] [ so ] [ nu ] [ co ] [ pu ].osId ] = 
                            & tpl_PUDescriptionsSet [ machineId ] [ so ] [ nu ] [ co ] [ pu ];
                    }
                }
            }
        }
        break;
    }
    case TPL_TYPE_HOMOGENEOUS_ASYMMETRIC:
    case TPL_TYPE_HETEROGENEOUS:
    default:
        bar_InternalError( __FILE__, __LINE__);
    }
    
}

static void
bar_InitExperiment( exp_Info_t * expInfo)
{
    expInfo->loExpNum = 1;
    expInfo->hiExpNum = EXPERIMENTS_NUM;

    expInfo->loBarNum = 1;
    expInfo->hiBarNum = BARRIERS_NUM;

    expInfo->loThreadsNum = THREADS_MIN_NUM;
    expInfo->hiThreadsNum = THREADS_MAX_NUM;

    expInfo->timer [ EXP_STAGE_REF ].clockId = EXP_CLOCK_ID;
    expInfo->timer [ EXP_STAGE_EXP ].clockId = EXP_CLOCK_ID;

#ifdef DELAYED_PRINT
    expInfo->currTableLine = 0;
#endif
    bar_SetOnlineCpuSet( );
#ifndef OMP_BARRIER
    bar_SetParentThreadAffinity( BAR_PARENT_AFFINITY_ONE);
#endif
    machineDescription.machineId = bar_ResolveMachineIdByHostname( HOSTNAME_STR);
    if ( machineDescription.machineId != UNDEFINED_MACHINE_ID )
    {
        machineDescription.summary = & tpl_MachineSummariesSet [ machineDescription.machineId ];
    } else
    {
        TOPOLOGY_AWARE_MAPPING = FALSE;
    }
    tpl_InitMachineTopology( );
    bar_MemInit( );

#if defined( TREE_BARRIER)
    expInfo->loRadixNum = 2;
    expInfo->hiRadixNum = RADIX_MAX;

    if ( (machineDescription.machineId != UNDEFINED_MACHINE_ID) &&
         (machineDescription.summary->maxRadix < expInfo->hiRadixNum) )
    {
        expInfo->hiRadixNum = machineDescription.summary->maxRadix;
    }
#endif
}

static void
bar_PrintUnsupportedConfiguration( )
{
    fprintf( stderr, "unsupported configuration!\n");
}

static inline bool
bar_IsUnsupportedConfiguration( )
{
#if defined( ARCH_X86_FAMILY) && defined( HWYIELD_SPINNING)
    if ( sys_GetPrivilegeLevel( ) != X86_RING_0 )
    {
        fprintf( stderr, "HLT requires ring 0 access!\n");
    }
    return TRUE;
#endif
#if defined( ARCH_X86_FAMILY) && defined( WFE_SPINNING)
    {
        int in_eax = 1;
        int eax;
        int ebx;
        int ecx;
        int edx;
    
        x86_Cpuid( in_eax, & eax, & ebx, & ecx, & edx);
        if ( ecx & (1 << MONITOR_CPUID_ECX_BIT) )
        {
            if ( sys_GetPrivilegeLevel( ) != X86_RING_0 )
            {
                /* FIXME Too conservative deicision. Need to do more precise check.
                   X86 ISA 8.10.3 The instructions are conditionally available at levels greater than 0.
                   Use the following steps to detect the availability of MONITOR and MWAIT: ... */
                fprintf( stderr, "MONITOR/MWAIT requires RING 0 access!\n");

                return TRUE;
            }
        } else
        {
            fprintf( stderr, "Processor does not MONITOR/MWAIT!\n");

            return TRUE;
        }
    }
#endif
    return FALSE;
}

static void
bar_ReadArgs( int argc,
              const char * argv [ ])
{
    if ( argc > 1 )
    {
        HOSTNAME_STR = argv [ 1 ];
    } else
    {
        HOSTNAME_STR = "unknown-host";
    }
    if ( argc > 2 )
    {
        EXP_ID_STR = argv [ 2 ];
    } else
    {
        EXP_ID_STR = "0";
    }
    if ( argc > 3 )
    {
        if ( !strcmp( argv [ 3 ], "yes") )
        {
            INTERPOLATE_RADIX = TRUE;
        } else if ( !strcmp( argv [ 3 ], "no") )
        {
            INTERPOLATE_RADIX = FALSE;
        } else
        {
            bar_InternalError( __FILE__, __LINE__);
        }
    } else
    {
        INTERPOLATE_RADIX = DEF_INTERPOLATE_RADIX;
    }
    if ( argc > 4 )
    {
        if ( !strcmp( argv [ 4 ], "yes") )
        {
            TOPOLOGY_AWARE_MAPPING = TRUE;
        } else if ( !strcmp( argv [ 4 ], "no") )
        {
            TOPOLOGY_AWARE_MAPPING = FALSE;
        } else
        {
            bar_InternalError( __FILE__, __LINE__);
        }
    } else
    {
        TOPOLOGY_AWARE_MAPPING = DEF_TOPOLOGY_AWARE_MAPPING;
    }
    if ( argc > 5 )
    {
        if ( !strcmp( argv [ 5 ], "yes") )
        {
            TOPOLOGY_NUMA_AWARE_ALLOC = TRUE;
        } else if ( !strcmp( argv [ 5 ], "no") )
        {
            TOPOLOGY_NUMA_AWARE_ALLOC = FALSE;
        } else
        {
            bar_InternalError( __FILE__, __LINE__);
        }
    } else
    {
        TOPOLOGY_NUMA_AWARE_ALLOC = DEF_TOPOLOGY_NUMA_AWARE_ALLOC;
    }
    if ( argc > 6 )
    {
        if ( !strcmp( argv [ 6 ], "yes") )
        {
            USER_DEFINED_ACTIVE_PUS_SELECTION = TRUE;
        } else if ( !strcmp( argv [ 6 ], "no") )
        {
            USER_DEFINED_ACTIVE_PUS_SELECTION = FALSE;
        } else
        {
            bar_InternalError( __FILE__, __LINE__);
        }
    } else
    {
        USER_DEFINED_ACTIVE_PUS_SELECTION = DEF_USER_DEFINED_ACTIVE_PUS_SELECTION;
    }
}

static void
bar_ThreadsCpuOverloadAdjustment( exp_Info_t * expInfo)
{
#if !defined( PTHREAD_BARRIER) && !defined( PTYIELD_SPINNING)
    if ( expInfo->curThreadsNum > CPUS_NUM)
    {
        expInfo->hiBarNum = BUSY_WAIT_THREAD_CPU_OVERLOAD_BARRIERS_NUM;
    }
#endif
#if defined( ARCH_ARM_FAMILY) && defined( PAUSE_SPINNING)
    expInfo->hiBarNum = BUSY_WAIT_THREAD_CPU_OVERLOAD_BARRIERS_NUM;
#endif
#if defined( PTHREAD_BARRIER)
    expInfo->hiBarNum = BARRIERS_NUM / 10;
#endif
}

#if defined( TREE_BARRIER)
static void
bar_InitRadix( exp_Info_t * expInfo)
{
    expInfo->curRadixNum = expInfo->loRadixNum;
}

static void
bar_IncRadix( exp_Info_t * expInfo)
{
#ifdef TRNM_BARRIER
    expInfo->curRadixNum += 1;
#else /* TRNM_BARRIER */
    if ( expInfo->curRadixNum < RADIX_CONTINIOUS_BOUND )
    {
        expInfo->curRadixNum += 1;
    } else
    {
        expInfo->curRadixNum RADIX_INC;
    }
#endif /* !TRNM_BARRIER */
}

static bool
bar_ProceedRadix( exp_Info_t * expInfo)
{
    int hiRadixNum = expInfo->hiRadixNum;

    if ( INTERPOLATE_RADIX &&
         (expInfo->curThreadsNum < expInfo->hiRadixNum) )
    {
        hiRadixNum = expInfo->curThreadsNum;
    }
    return  (
#   ifdef TRNM_BARRIER
#       ifdef TRNM_STAT_WIN
              (expInfo->curRadixNum <= (MA_GRANULARITY + 1)) &&
#       else
              (expInfo->curRadixNum <= MA_GRANULARITY) &&
#       endif
#   endif
              (expInfo->curRadixNum <= hiRadixNum)) ||
              ((expInfo->curThreadsNum == 1) && (expInfo->curRadixNum == 2));
}
#endif

int
main( int argc, 
      const char * argv [ ])
{
    exp_Info_t expInfo;
   
    bar_ReadArgs( argc, argv);
     
    if ( bar_IsUnsupportedConfiguration( ) )
    {
        return 0;
    }

    bar_CheckPreconditions( );
    bar_InitExperiment( & expInfo);
#ifndef NDEBUG
    bar_PrintExperimentInfo( & expInfo);
#endif
#ifndef DELAYED_PRINT
    bar_PrintTableHeader( );
#endif

    int lim;
    for ( expInfo.curThreadsNum = expInfo.loThreadsNum;
          expInfo.curThreadsNum <= expInfo.hiThreadsNum; 
          lim = expInfo.curThreadsNum, 
          lim THREADS_INC, 
          (lim <= expInfo.hiThreadsNum) ? 
              expInfo.curThreadsNum THREADS_INC :
              expInfo.curThreadsNum ++
        )
    {
        bar_ThreadsCpuOverloadAdjustment( &expInfo);
#if defined( TREE_BARRIER)
        for ( bar_InitRadix( & expInfo);
              bar_ProceedRadix( & expInfo);
              bar_IncRadix( & expInfo) )
#endif
        {
            /* warming up */
            expInfo.expStage = EXP_STAGE_REF;
            bar_TestBarrier( & expInfo);
            
            expInfo.expStage = EXP_STAGE_EXP;
            bar_TestBarrier( & expInfo);

            for ( expInfo.curExpNum = expInfo.loExpNum;
                  expInfo.curExpNum <= expInfo.hiExpNum ;
                  expInfo.curExpNum++ )
            {
                expInfo.expStage = EXP_STAGE_REF;
                bar_TestBarrier( & expInfo);

                expInfo.expStage = EXP_STAGE_EXP;
                bar_TestBarrier( & expInfo);

#ifdef DELAYED_PRINT
                bar_SaveTableLine( & expInfo);
#else
                bar_PrintTableLine( & expInfo);
#endif
            }
        }
    }
#ifdef DELAYED_PRINT
    bar_PrintTableHeader( );
    bar_PrintTableLines( & expInfo);
#endif
    bar_MemFini( );

    return 0;
}

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

#ifndef BARRIER_H
#define BARRIER_H


/**
 * Debug and assert switches
 */
#define NDEBUG
#define NASSERT

#define TRUE          1
#define FALSE         0

#define RETURN_OK     FALSE
#define RETURN_FAIL   TRUE

/**
 * Number of logical CPUs
 */
#ifndef CPUS_NUM
#   define CPUS_NUM 2
#endif


/**
 * Number fo threads and experiments
 */
#ifndef THREADS_MAX_NUM
#   define THREADS_MAX_NUM CPUS_NUM
#endif
#ifndef CEIL_LOG2_THREADS_MAX_NUM
/* Rough approximation given another constraint that it should be greater than zero */
#   define CEIL_LOG2_THREADS_MAX_NUM THREADS_MAX_NUM
#endif
#ifndef BARRIERS_NUM
#   define BARRIERS_NUM 1000000
#endif

#ifndef EXPERIMENTS_NUM
#   define EXPERIMENTS_NUM 10
#endif

#ifndef CPU_MAP_PRIORITY_DELTA
#   define CPU_MAP_PRIORITY_DELTA 1
#endif

#ifndef THREADS_MIN_NUM
#   define THREADS_MIN_NUM 1
#endif

#ifndef THREADS_INC
#   define THREADS_INC +=1
#endif

#ifndef RADIX_INC
#   define RADIX_INC +=1
#endif

#ifndef RADIX_MAX
#   define RADIX_MAX THREADS_MAX_NUM
#endif

#define RADIX_CONTINIOUS_BOUND 4

#define BARRIERS_MAX_NUM 4

/**
 * 'clock_gettime' clock type
 */
#ifndef EXP_CLOCK_ID
#   define EXP_CLOCK_ID CLOCK_MONOTONIC
#endif

#ifdef ARCH_MIC
#   define ARCH_X86_FAMILY
#   define ARCH_STORE_NR
/* Turns out to be less favorable decision in comparision with ARCH_STORE_NR */
//#   define ARCH_STORE_NR_NGO
/* Turns out to be more favorable decision in comparision with ARCH_STORE_NR */
//#   define ARCH_STORE_NR_NGO_REFINED
#   ifdef ARCH_STORE_NR_NGO_REFINED
#       define ARCH_STORE_NR_NGO
#   endif
#   define ARCH_MIC_DELAY 64
#endif
#ifdef ARCH_X86_64
#   define ARCH_X86_FAMILY
#   define ARCH_X86_MONITOR_MWAIT
/* The architecture supports both defines, but they are not beneficial, therefore, switched off. */
/*#   define ARCH_STORE_NR */
/*#   define ARCH_LOAD_NC */
#endif
#ifdef ARCH_ARMV7L
#   define ARCH_ARM_FAMILY
#endif

/**
 * Architecture type
 */
#if !defined( ARCH_ARM_FAMILY) && !defined ( ARCH_X86_FAMILY)
/* #define ARCH_ARM_FAMILY */
#   define ARCH_X86_FAMILY
#endif

/**
 * Spinning amortization
 */
#if !defined( SPIN_SPINNING) && !defined( HWYIELD_SPINNING) && !defined( PTYIELD_SPINNING) && \
    !defined( PAUSE_SPINNING) && !defined( WFE_SPINNING)
#define SPIN_SPINNING
/* #define HWYIELD_SPINNING */
/* #define PTYIELD_SPINNING */
/* #define PAUSE_SPINNING */
/* #define WFE_SPINNING */
#endif

#if defined( PTYIELD_SPINNING) || defined( HWYIELD_SPINNING)
#   define YIELD_SPINNING
#endif

#ifndef BUSY_WAIT_THREAD_CPU_OVERLOAD_BARRIERS_NUM
#define BUSY_WAIT_THREAD_CPU_OVERLOAD_BARRIERS_NUM (THREADS_MAX_NUM * 2 + 1)
#endif

/**
 * Coherency line size
 */
#ifndef CCL_SIZE
#   define CCL_SIZE 128
#endif

/**
 * Maximum atomic data size in bits
 * FIXME: Maximum atomic data size should be defined more precisely depending on MP architecture 
 */
#define HW_ATOMIC_DATA_SIZE_IN_BITS 32

/**
 * Max memory access size in bits.
 */
#define HW_MAX_MA_SIZE_IN_BITS 32

/**
 * Min memory access size in bits.
 */
#define HW_MIN_MA_SIZE_IN_BITS 8

/**
 * Memory access granularity.
 */
#define MA_GRANULARITY (HW_MAX_MA_SIZE_IN_BITS / HW_MIN_MA_SIZE_IN_BITS)

/**
 * Using libnuma for NUMA-aware memory allocation.
 */
/* #define LIB_NUMA */

/**
 * Memory access min/max size volatile types.
 */
#define DEFINE_MAX_MA_VOLATILE_TYPE_H(max_ma_size) typedef volatile int ## max_ma_size ## _t int_max_ma_vol_t
#define DEFINE_MIN_MA_VOLATILE_TYPE_H(min_ma_size) typedef volatile int ## min_ma_size ## _t int_min_ma_vol_t
#define DEFINE_MAX_MA_VOLATILE_TYPE(max_ma_size) DEFINE_MAX_MA_VOLATILE_TYPE_H(max_ma_size)
#define DEFINE_MIN_MA_VOLATILE_TYPE(min_ma_size) DEFINE_MIN_MA_VOLATILE_TYPE_H(min_ma_size)
DEFINE_MAX_MA_VOLATILE_TYPE( HW_MAX_MA_SIZE_IN_BITS);
DEFINE_MIN_MA_VOLATILE_TYPE( HW_MIN_MA_SIZE_IN_BITS) ;

/**
 * Memory access min/max size types.
 */
#define DEFINE_MAX_MA_TYPE_H(max_ma_size) typedef int ## max_ma_size ## _t int_max_ma_t
#define DEFINE_MIN_MA_TYPE_H(min_ma_size) typedef int ## min_ma_size ## _t int_min_ma_t
#define DEFINE_MAX_MA_TYPE(max_ma_size) DEFINE_MAX_MA_TYPE_H(max_ma_size)
#define DEFINE_MIN_MA_TYPE(min_ma_size) DEFINE_MIN_MA_TYPE_H(min_ma_size)
DEFINE_MAX_MA_TYPE( HW_MAX_MA_SIZE_IN_BITS);
DEFINE_MIN_MA_TYPE( HW_MIN_MA_SIZE_IN_BITS) ;

/**
 * Bytes in byte
 */
#define BITS_IN_BYTE 8


/**
 * Atomic type.
 * FIXME: Atomic data type should be defined more precisely depending on MP architecture 
 */
typedef unsigned int atomic_Data_t;


/**
 * Barrier test type: calc (with some calculations to check correctness) or pure
 */
#if !defined( SANITY_BENCHMARK) && !defined( PURE_BENCHMARK) && !defined( LDIMBL_BENCHMARK) &&\
    !defined( TMPL_BENCHMARK) && !defined( NBODY_BENCHMARK)
/* #   define TMPL_BENCHMARK */
/* #   define SANITY_BENCHMARK */
/* #   define LDIMBL_BENCHMARK */
#   define PURE_BENCHMARK
#endif

#ifdef NBODY_BENCHMARK
/**
 * NBODY interaction constant.
 */
# define NBODY_CONST (6.674E-11F)

/**
 * Particle.
 */
typedef struct bar_Particle_s
{
    float x;
    float y;
    float z;
    float Vx;
    float Vy;
    float Vz;
    char padding [ CCL_SIZE - (sizeof( float) * 6) ];
} bar_Particle_t;
#endif /* NBODY_BENCHMARK */


#define TMPL_TIME "   time"

/**
 * Defualt value for INTERPOLATE_RADIX
 */
#if !defined( DEF_INTERPOLATE_RADIX)
#   define DEF_INTERPOLATE_RADIX FALSE
#endif

/**
 * Defualt value for TOPOLOGY_AWARE_MAPPING
 */
#if !defined( DEF_TOPOLOGY_AWARE_MAPPING)
#   define DEF_TOPOLOGY_AWARE_MAPPING TRUE
#endif

/**
 * Defualt value for USER_DEFINED_ACTIVE_PUS_SELECTION
 */
#if !defined( DEF_USER_DEFINED_ACTIVE_PUS_SELECTION)
#   define DEF_USER_DEFINED_ACTIVE_PUS_SELECTION FALSE
#endif


/**
 * Barrier type
 */
#if !defined( PTHREAD_BARRIER) && !defined ( SR_BARRIER) &&\
    !defined( CTRGS_BARRIER) && !defined( CTRLS_BARRIER) &&\
    !defined( DSMN_BARRIER) && !defined( DSMNH_BARRIER) &&\
    !defined( STNGS_BARRIER) && !defined( STNLS_BARRIER) &&\
    !defined( DTNGS_BARRIER) && !defined( DTNLS_BARRIER) &&\
    !defined( STRGS_BARRIER) && !defined( STRLS_BARRIER) &&\
    !defined( OMP_BARRIER)
/* #   define PTHREAD_BARRIER */
#   define SR_BARRIER
/* #   define CTRGS_BARRIER */
/* #   define CTRLS_BARRIER */
/* #   define STRGS_BARRIER */
/* #   define STRLS_BARRIER */
/* #   define STNGS_BARRIER */
/* #   define STNLS_BARRIER */
/* #   define DTNGS_BARRIER */
/* #   define DTNLS_BARRIER */
/* #   define DSMN_BARRIER */
/* #   define OMP_BARRIER */
/* #   define DSMNH_BARRIER */
#endif


/**
 * Different types of tree barriers where threads are asigned to leaves only
 * with global or local sense
 */
#ifdef CTRGS_BARRIER
#   define COMBINED_BARRIER
#   define TREE_BARRIER
#   define T_GLOBAL_SENSE
#endif
#ifdef CTRLS_BARRIER
#   define COMBINED_BARRIER
#   define TREE_BARRIER
#   define T_LOCAL_SENSE
#endif
#ifdef DTNGS_BARRIER
#   define TRNM_BARRIER
#   define TREE_BARRIER
#   define T_GLOBAL_SENSE
#endif
#ifdef DTNLS_BARRIER
#   define TRNM_BARRIER
#   define TREE_BARRIER
#   define T_LOCAL_SENSE
#endif
#ifdef STNGS_BARRIER
#   define TRNM_BARRIER
#   define TREE_BARRIER
#   define T_GLOBAL_SENSE
#endif
#ifdef STNLS_BARRIER
#   define TRNM_BARRIER
#   define TREE_BARRIER
#   define T_LOCAL_SENSE
#endif

#ifdef TRNM_BARRIER
#   if defined( DTNGS_BARRIER) || defined( DTNLS_BARRIER)
#       define TRNM_DYNM_WIN
#   endif
#   if defined( STNGS_BARRIER) || defined( STNLS_BARRIER)
#       define TRNM_STAT_WIN
#       define TRNM_STAT_WIN_ID (-1)
#   endif
#   define TRNM_TRUE TRUE
#   define TRNM_FALSE FALSE
#endif

#ifdef ARCH_X86_FAMILY
#   define ARCH_CAS
#   define ARCH_FETCH_AND_ADD
#   define INTRA_PROCESSOR_FORWARDING_ALLOWED
#endif
#ifdef ARCH_ARM_FAMILY
#   define ARCH_LL_SC
#   define INTRA_PROCESSOR_FORWARDING_ALLOWED
#endif


#ifdef ARCH_X86_FAMILY
typedef enum x86_Ring_e
{
    X86_RING_0 = 0,
    X86_RING_1 = 1,
    X86_RING_2 = 2,
    X86_RING_3 = 3,
    x86_RINGS_NUM,
    X86_RINGS_MASK = 3
} x86_Ring_t;
#endif

#define MONITOR_CPUID_IN_EAX 1
#define MONITOR_CPUID_ECX_BIT 3

typedef unsigned int bool;

#define MACHINES_MAX_NUM 9
#define SOCKETS_PER_MACHINE_MAX_NUM 4
#define NUMANODES_PER_SOCKET_MAX_NUM 2
#define CORES_PER_NUMANODE_MAX_NUM 60
#define PUS_PER_CORE_MAX_NUM 4

#define NUMANODES_PER_MACHINE_MAX_NUM (SOCKETS_PER_MACHINE_MAX_NUM * NUMANODES_PER_SOCKET_MAX_NUM)
#define PUS_PER_MACHINE_MAX_NUM (NUMANODES_PER_MACHINE_MAX_NUM * CORES_PER_NUMANODE_MAX_NUM * PUS_PER_CORE_MAX_NUM)

#define UNDEFINED_MACHINE_ID (-1)
#define UNDEFINED_THREAD_ID (-1)
#define UNDEFINED_NUMANODE_ID (-1)


#define MACHINE_SUMMARY_INIT( TPL_TYPE, SMT_TYPE, SO_PER_MA_NUM, NU_PER_SO_NUM, CO_PER_NU_NUM, PU_PER_CO_NUM,\
                              NUMA_MALLOC_POLICY, AVOID_CONNECTIONS, MAX_RADIX)\
    { (TPL_TYPE), (SMT_TYPE), (SO_PER_MA_NUM), (NU_PER_SO_NUM), (CO_PER_NU_NUM), (PU_PER_CO_NUM), NUMA_MALLOC_POLICY, AVOID_CONNECTIONS, (MAX_RADIX)}

#define MACHINE_TOPOLOGY_INIT( ...) \
    { __VA_ARGS__ }
#define SO( ...) \
    { __VA_ARGS__ }
#define NU( ...) \
    { __VA_ARGS__ }
#define CO( ...) \
    { __VA_ARGS__ }
#define PU( PU_NUM) \
    { PU_NUM, 1 }

#define AVOID_INTERCORE_CONNECTIONS { TRUE, TRUE, TRUE, TRUE, FALSE}
#define AVOID_INTERNUMANODE_CONNECTIONS { TRUE, TRUE, TRUE, FALSE, FALSE}

/**
 * Policy for memory allocation.
 */
typedef enum mem_NumaMallocPolicy_e
{
    MALLOC_ON_R = 0,
    MALLOC_ON_W = 1,
    MALLOC_ANY = MALLOC_ON_R,
    MALLOC_POLICIES_NUM
} mem_NumaMallocPolicy_t;

/**
 * Type of SMT implementastion.
 */
typedef enum tpl_SMT_e
{
    TPL_SMT_NONE,/* No SMT */
    TPL_SMT_HT,  /* Hyper-Threading Technology */
    TPL_SMT_CMT, /* Clustered Multi-Threading Technology */
    TPL_SMT_MIC, /* MIC Multi-Threading */
    TPL_SMT_MIXED, /* Several types of SMT for heterogeneous machine. */
    TPL_SMT_NUM
} tpl_SMT_t;

/**
 * Machine topology type.
 *
 * TODO Support TPL_TYPE_HOMOGENEOUS_ASYMMETRIC and TPL_TYPE_HETEROGENEOUS topologies.
 */
typedef enum tpl_MachineTopologyType_e
{
    TPL_TYPE_HOMOGENEOUS_SYMMETRIC,
    TPL_TYPE_HOMOGENEOUS_ASYMMETRIC,
    TPL_TYPE_HETEROGENEOUS,
    TPL_TYPES_NUM
} tpl_MachineTopologyType_t;

/**
 * Machine topology node type.
 */
typedef enum tpl_NodeType_e
{
    TPL_NODE_MACHINE,
    TPL_NODE_SOCKET,
    TPL_NODE_NUMANODE,
    TPL_NODE_CORE,
    TPL_NODE_PU,
    TPL_NODE_UNDEFINED,
    TPL_NODES_DEFINED_NUM = TPL_NODE_UNDEFINED,
    TPL_NODES_ALL_NUM
} tpl_NodeType_t;

/**
 * Machine topology summary.
 */
typedef struct tpl_MachineSummary_t
{
    tpl_MachineTopologyType_t topologyType;
    tpl_SMT_t smtType;

    /* fields below are related to TPL_TYPE_HOMOGENEOUS_SYMMETRIC topologies only. */
    unsigned socketsPerMachineNum;
    unsigned numaNodesPerSocketNum;
    unsigned coresPerNumaNodeNum;
    unsigned pusPerCoreNum;
    mem_NumaMallocPolicy_t numaMallocPolicy;
    
    /* fields below are related to barrier synchronization.  */
    unsigned avoidInterNodeConnections [ TPL_NODES_DEFINED_NUM ];
    unsigned maxRadix;
} tpl_MachineSummary_t;

/**
 * Processor Unit (PU) topology description.
 */
struct tls_Data_t;
typedef struct tpl_PU_t
{
    unsigned osId;
    unsigned numaOsId;

    unsigned puId;
    unsigned coreId;
    unsigned numaNodeId;
    unsigned socketId;
    unsigned machineId;
    tpl_SMT_t smtType;

    unsigned activeThreadsNum;
    struct tls_Data_t * firstThreadData;
    struct tls_Data_t * lastThreadData;
} tpl_PU_t;

/**
 * Core (CO) topology description.
 */
typedef struct tpl_CO_t
{
    tpl_PU_t * pus [ PUS_PER_CORE_MAX_NUM ];
    
    unsigned activeThreadsNum;
} tpl_CO_t;

/**
 * Numa Node (NU) topology description.
 */
typedef struct tpl_NU_t
{
    tpl_CO_t cores [ CORES_PER_NUMANODE_MAX_NUM ]; 
    unsigned osId;

    unsigned activeThreadsNum;
    unsigned allocatedInodesNum;
    void * allocatedInodes [ CEIL_LOG2_THREADS_MAX_NUM * THREADS_MAX_NUM ];
} tpl_NU_t;

/**
 * Socket (SO) topology description.
 */
typedef struct tpl_SO_t
{
    tpl_NU_t numaNodes [ NUMANODES_PER_SOCKET_MAX_NUM ];
    
    unsigned activeThreadsNum;
} tpl_SO_t;

/**
 * Machine topology description
 */
typedef struct tpl_MachineDescription_t
{
    unsigned machineId;
    tpl_MachineSummary_t * summary;
    tpl_SO_t sockets [ SOCKETS_PER_MACHINE_MAX_NUM ];

    unsigned activeThreadsNum;
} tpl_MachineDescription_t;


#define UNDEFINED_RADIX 1

typedef enum parity_e
{
    PARITY_EVEN,
    PARITY_GDL = PARITY_EVEN,
    PARITY_ODD,
    PARITY_NUM
} parity_t;

#ifdef PTHREAD_BARRIER
typedef pthread_barrier_t bar_Barrier_t;
#endif

#if defined ( SR_BARRIER) || defined ( DSMNH_BARRIER)
typedef struct sr_barrier_t
{
    volatile __attribute__ ( ( aligned( CCL_SIZE))) int sense;
    volatile __attribute__ ( ( aligned( CCL_SIZE))) int count;
    char padding [ CCL_SIZE - sizeof( int) ];
    int threadsNum;
    int barrierId;
} __attribute__ ( ( aligned( CCL_SIZE))) sr_barrier_t; 
#endif

#if defined ( DSMN_BARRIER) || defined ( DSMNH_BARRIER)
typedef struct dsmn_barrier_t
{
    int barrierId;
    int ceilLog2ThreadsNum;
} __attribute__ ( ( aligned( CCL_SIZE))) dsmn_barrier_t; 
#endif

#ifdef TREE_BARRIER
#ifdef TRNM_BARRIER
typedef union trnm_Data_t
{
     int_min_ma_vol_t part [ MA_GRANULARITY ];
     int_max_ma_vol_t full;
} __attribute__ ( ( aligned( CCL_SIZE))) trnm_Data_t;
#endif
typedef struct tree_node_t
{
#ifdef T_LOCAL_SENSE
    volatile __attribute__ ( ( aligned( CCL_SIZE))) bool * sense;
    char paddingS [ CCL_SIZE - sizeof( bool) ];
#endif
#ifdef COMBINED_BARRIER
    volatile __attribute__ ( ( aligned( CCL_SIZE))) int count;
    char paddingC [ CCL_SIZE - sizeof( int) ];
    int threadsNum;
#endif
#ifdef TRNM_BARRIER
    trnm_Data_t trnmDataCurr;
    volatile trnm_Data_t trnmDataInit [ PARITY_NUM ];
    /* numeration start from 0 and increases from leaves to root */
    int tier;
#endif
    struct tree_node_t * parent;
} __attribute__ ( ( aligned( CCL_SIZE))) tree_node_t;

typedef struct tree_barrier_t
{
#ifdef T_GLOBAL_SENSE
    volatile __attribute__ ( ( aligned( CCL_SIZE))) bool * sense;
    char padding [ CCL_SIZE - sizeof( bool) ];
#endif
    int radix;
    int leavesNum;
    int inodesNum;
    int threadsNum;
    struct tree_node_t * leaves [ 1 << CEIL_LOG2_THREADS_MAX_NUM ];
    struct tree_node_t * inodes [ 1 << CEIL_LOG2_THREADS_MAX_NUM ];
#ifdef TRNM_BARRIER
    int partIdMap [ THREADS_MAX_NUM ] [ CEIL_LOG2_THREADS_MAX_NUM ];
#endif
    int barrierId;
} __attribute__ ( ( aligned( CCL_SIZE))) tree_barrier_t;

typedef struct tree_build_context_t
{
    tpl_NodeType_t curTplLevel;
    unsigned curHeight [ TPL_NODES_ALL_NUM ];
    unsigned reachHeight [ TPL_NODES_ALL_NUM ];
    unsigned tplLevelLeavesToConstruct [ TPL_NODES_ALL_NUM ];
    tree_node_t * parent;
    tree_node_t * root;
    int parentEdgeId;
} tree_build_context_t;

#endif /* TREE_BARRIER */

#define NANOSEC_IN_SEC 1000000000

/**
 * Timer info.
 */
typedef struct exp_Timer_t
{
    clockid_t clockId;
    struct timespec startTime;
    struct timespec stopTime;
    unsigned long long int deltaTime;
} exp_Timer_t;

/**
 * Timer info.
 */
typedef struct exp_ClockCounter_t
{
    unsigned long long int startClock;
    unsigned long long int stopClock;
    unsigned long long int deltaClock;;
} exp_ClockCounter_t;

/**
 * Experiment stage.
 */
typedef enum exp_Stage_e
{
    EXP_STAGE_REF,
    EXP_STAGE_EXP,
    EXP_STAGE_NUM
} exp_Stage_t;

/**
 * Affinity type.
 */
typedef enum bar_ParentAffinity_e
{
    BAR_PARENT_AFFINITY_ONE,
    BAR_PARENT_AFFINITY_ALL,
    BAR_PARENT_AFFINITY_NUM
} bar_ParentAffinity_t;

#define DELAYED_PRINT
#ifdef DELAYED_PRINT

/* FIXME Too rough apprximation. */
#define EXP_LINES_NUM ((THREADS_MAX_NUM * THREADS_MAX_NUM * EXPERIMENTS_NUM))

/**
 * Experiment table line.
 */
typedef struct exp_TableLine_t
{
    int threadsNum;
    int radix;
#ifdef PRINT_SYNCH_UNSYNCH_PHASE_TIME
    double timePerUnsynchronizedPhase;
#endif
    double timePerBarrier;
} exp_TableLine_t;
#endif

/**
 * Experiment info.
 */
typedef struct exp_Info_t
{
    int loExpNum;
    int hiExpNum;
    int curExpNum;
    int loThreadsNum;
    int hiThreadsNum;
    int curThreadsNum;
#if defined ( TREE_BARRIER)
    int loRadixNum;
    int hiRadixNum;
    int curRadixNum;
#endif
    int loBarNum;
    int hiBarNum;
    exp_Stage_t expStage;
    exp_Timer_t timer [ EXP_STAGE_NUM ];
    exp_ClockCounter_t clockCounter [ EXP_STAGE_NUM ];
    
#ifdef DELAYED_PRINT
    int currTableLine;
    exp_TableLine_t tableLines [ EXP_LINES_NUM ];
#endif
} exp_Info_t;

typedef struct tls_Sense_t
{
    bool data;
    char padding [ CCL_SIZE - sizeof( bool) ];
} __attribute__ ( ( aligned( CCL_SIZE), packed)) tls_Sense_t;

typedef struct tls_Parity_t
{
    parity_t data;
    char padding [ CCL_SIZE - sizeof( parity_t) ];
} __attribute__ ( ( aligned( CCL_SIZE))) tls_Parity_t;

/**
 * Thread local data.
 */
typedef struct tls_Data_t
{
#if defined( DSMN_BARRIER) || defined( DSMNH_BARRIER)
    volatile tls_Sense_t * my_flags [ PARITY_NUM ] [ CEIL_LOG2_THREADS_MAX_NUM ] [ BARRIERS_MAX_NUM ];
    volatile tls_Sense_t * partner_flags [ PARITY_NUM ] [ CEIL_LOG2_THREADS_MAX_NUM ] [ BARRIERS_MAX_NUM ];
    tls_Parity_t parity [ BARRIERS_MAX_NUM ];
    tls_Sense_t sense [ BARRIERS_MAX_NUM ];
#   ifdef DSMNH_BARRIER
    tls_Sense_t senseSR [ BARRIERS_MAX_NUM ][ THREADS_MAX_NUM ];
    struct tls_Data_t * dsmnTlsData;
    bool groupId;
#   endif
#endif
#if defined( SR_BARRIER) || defined( TREE_BARRIER)
    tls_Sense_t sense [ BARRIERS_MAX_NUM ];
#endif
#ifdef TREE_BARRIER
    int leafId;
#endif
    int threadId;
    exp_Info_t * expInfo;
    struct tls_Data_t * nextThreadData;
    tpl_PU_t * curPU;
} __attribute__ ( ( aligned( CCL_SIZE))) tls_Data_t;

/**
 * Thread local datas set.
 */
typedef struct tls_DataSet_t
{
    tls_Data_t * tlsData [ THREADS_MAX_NUM ];
} tls_DataSet_t;

#endif /* !BARRIER_H */

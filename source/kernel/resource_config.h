/* Independent current resource controls. Historical compositions ignore them. */
#ifndef QDOS386_RESOURCE_CONFIG_H
#define QDOS386_RESOURCE_CONFIG_H
#ifndef QDOS_CFG_MAX_JOBS
#define QDOS_CFG_MAX_JOBS 24
#endif
#ifndef QDOS_CFG_MAX_CPUS
#define QDOS_CFG_MAX_CPUS 8
#endif
#ifndef QDOS_CFG_MAX_SESSIONS
#define QDOS_CFG_MAX_SESSIONS 20
#endif
#ifdef QDOS_R4_RESOURCES
#if QDOS_CFG_MAX_JOBS != 24 && QDOS_CFG_MAX_JOBS != 64
#error "R5: job capacity must be 24 or 64; 256 requires ABI/budget review"
#endif
#if QDOS_CFG_MAX_CPUS != 1 && QDOS_CFG_MAX_CPUS != 4 && QDOS_CFG_MAX_CPUS != 8
#error "R4: CPU contexts must be 1, 4 or 8; 32 needs SMP wire-ABI/topology review"
#endif
#if QDOS_CFG_MAX_SESSIONS < 4 || QDOS_CFG_MAX_SESSIONS > 20
#error "R4: sessions must be 4..20; sessions are independent of job capacity"
#endif
#if defined(R17_JOB_MAX) && R17_JOB_MAX != QDOS_CFG_MAX_JOBS
#error "R4: R17_JOB_MAX conflicts with QDOS_CFG_MAX_JOBS"
#endif
#endif
#endif

#ifndef NBNXM_KERNEL_MT3000_H
#define NBNXM_KERNEL_MT3000_H

#include "gromacs/nbnxm/constants.h"
/* Grouped pair-list i-unit */
typedef struct
{
    // /* Returns the number of j-cluster groups in this entry */
    // int numJClusterGroups() const { return cj4_ind_end - cj4_ind_start; }

    int sci;           /* i-super-cluster       */
    int shift;         /* Shift vector index plus possible flags */
    int cj4_ind_start; /* Start index into cj4  */
    int cj4_ind_end;   /* End index into cj4    */
} mt_nbnxn_sci_t;

#ifndef M_1_SQRTPI
/* 1.0 / sqrt(M_PI) */
#    define M_1_SQRTPI 0.564189583547756
#endif

#define c_nbnxnGpuClusterSize 8

#define c_nbnxnGpuClusterpairSplit 1

//! The fixed size of the exclusion mask array for a half GPU cluster pair
#define c_nbnxnGpuExclSize  \
        (c_nbnxnGpuClusterSize * c_nbnxnGpuClusterSize / c_nbnxnGpuClusterpairSplit)

//nbnxn_cj4_t
typedef struct
{
    // The i-cluster interactions mask for 1 warp
    unsigned int imask;
    // Index into the exclusion array for 1 warp, default index 0 which means no exclusions
    int excl_ind;
} mt_nbnxn_im_ei_t;

typedef struct
{
    int           cj[c_nbnxnGpuJgroupSize];         /* The 4 j-clusters */
    mt_nbnxn_im_ei_t imei[c_nbnxnGpuClusterpairSplit]; /* The i-cluster mask data       for 2 warps */
} mt_nbnxn_cj4_t;

typedef struct
{
    /* Topology exclusion interaction bits per warp */
    unsigned int pair[c_nbnxnGpuExclSize];
}mt_nbnxn_excl_t;

typedef struct
{
    int cj4_ind;
    int im;
    int ic;
    int jm;
    int jc;
    int status;
    int npair;
    int fix;
    int fiy;
    int fiz;
    //cache
    mt_nbnxn_cj4_t cj4;
    int cj4_valid;
    mt_nbnxn_excl_t excl;
    int excl_valid;
    real ix;
    real iy;
    real iz;
    real iq;
    int iq_valid;
    real jx;
    real jy;
    real jz;
    real jq;
    int jq_valid;
    int typei;
    int typei_valid;
    int typej;
    int typej_valid;
    real fexcl;
    int fexcl_valid;

} status_vars_calculateOp_t;

#endif
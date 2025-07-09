// #include "gromacs/utility/real.h"
#include<stdio.h>
#include<stdlib.h>
#include <math.h>       /* erff sqrtf*/
#include "nbnxm_kernel_mt3000.h"
#include "common_objs.h"
#include "gromacs/pbcutil/ishift.h"
#include "gromacs/nbnxm/constants.h"
#include    "hthread_device.h"
#include "string.h"

#include	"task_types.h"
#include "../op_cache_tianhe/op_cache.h"

typedef float real;
const int dsp_count = 1;
static const int c_numClPerSupercl = c_nbnxnGpuNumClusterPerSupercluster;
static const int c_clSize          = 8;

status_vars_calculateOp_t status_vars;

void updateStatusVars(
                        int cj4_ind,
                        int im,
                        int ic,
                        int jm,
                        int jc,
                        int status,
                        int npair,
                        int fix,
                        int fiy,
                        int fiz,
                        mt_nbnxn_cj4_t cj4,
                        int cj4_valid,
                        mt_nbnxn_excl_t excl,
                        int excl_valid,
                        real ix,
                        real iy,
                        real iz,
                        real iq,
                        int iq_valid,
                        real jx,
                        real jy,
                        real jz,
                        real jq,
                        int jq_valid,
                        int typei,
                        int typei_valid,
                        int typej,
                        int typej_valid,
                        real fexcl,
                        int fexcl_valid
                     )
{
    int cj4_ind = status_vars.cj4_ind = cj4_ind;
    int im = status_vars.im = im;
    int ic = status_vars.ic = ic;
    int jm = status_vars.jm = jm;
    int jc = status_vars.jc =jc;
    int status = status_vars.status =status;
    int npair = status_vars.npair = npair;
    int fix = status_vars.fix = fix;
    int fiy = status_vars.fiy = fiy;
    int fiz = status_vars.fiz = fiz;

    status_vars.cj4 = cj4;
    status_vars.cj4_valid = cj4_valid;
    status_vars.excl = excl;
    status_vars.excl_valid = excl_valid;
    status_vars.ix = ix;
    status_vars.iy = iy;
    status_vars.iz = iz;
    status_vars.iq = iq;
    status_vars.iq_valid = iq_valid;
    status_vars.jx = jx;
    status_vars.jy = jy;
    status_vars.jz =jz;
    status_vars.jq =jq;
    status_vars.jq_valid =jq_valid;
    status_vars.typei =typei;
    status_vars.typei_valid =typei_valid;
    status_vars.typej =typej;
    status_vars.typej_valid =typej_valid;
    status_vars.fexcl =fexcl;
    status_vars.fexcl_valid =fexcl_valid;
}

task_ret calculateFOp1(void** inputs, void** outputs){
    mt_nbnxn_sci_t *nbln = ((obj_mem)inputs[0])->p;
    int sci = nbln->sci;
    int cj4_ind0 = nbln->cj4_ind_start;
    int cj4_ind1 = nbln->cj4_ind_end;
    int shift = nbln->shift;
    int* sh = ((obj_mem)inputs[1])->p;
    int shX = sh[0];
    int shY = sh[1];
    int shZ = sh[2];

    int ish3 = 3* nbln->shift;

    cache_info_t* cache_info_x = (cache_info_t*)inputs[2];
    cache_info_t* cache_info_cj4 = (cache_info_t*)inputs[3];
    cache_info_t* cache_info_type = (cache_info_t*)inputs[4];
    cache_info_t* cache_info_Ftab = (cache_info_t*)inputs[5];
    cache_info_t* cache_info_excl = (cache_info_t*)inputs[6];

    real* f = inputs[7];
    real* fshift = inputs[8];
    real* Vc = inputs[9];
    real* Vvdw = inputs[10];

    int cj4_ind = status_vars.cj4_ind;
    int im = status_vars.im;
    int ic = status_vars.ic;
    int jm = status_vars.jm;
    int jc = status_vars.jc;
    int status = status_vars.status;
    int npair = status_vars.npair;
    int fix = status_vars.fix;
    int fiy = status_vars.fiy;
    int fiz = status_vars.fiz;

    mt_nbnxn_cj4_t cj4 = status_vars.cj4;
    int cj4_valid = status_vars.cj4_valid;
    mt_nbnxn_excl_t excl = status_vars.excl;
    int excl_valid = status_vars.excl_valid;
    real ix = status_vars.ix;
    real iy = status_vars.iy;
    real iz = status_vars.iz;
    real iq = status_vars.iq;
    int iq_valid = status_vars.iq_valid;
    real jx = status_vars.jx;
    real jy = status_vars.jy;
    real jz = status_vars.jz;
    real jq = status_vars.jq;
    int jq_valid = status_vars.jq_valid;
    int typei = status_vars.typei;
    int typei_valid = status_vars.typei_valid;
    int typej = status_vars.typej;
    int typej_valid = status_vars.typej_valid;
    real fexcl = status_vars.fexcl;
    int fexcl_valid = status_vars.fexcl_valid;


    int                 ci, cj;
    int                 ia, ja, is, ifs, js, jfs;
    int                 n0;
    real                fscal, tx, ty, tz;
    int                 ggid;
    int within_rlist; 
    real                ix, iy, iz, fix, fiy, fiz;
    real                jx, jy, jz;
    real                dx, dy, dz, rsq, rinv;
    real                iq;
    real                qq, vcoul = 0, krsq, vctot = 0;
    int                 nti;
    int                 tj;
    real                rinvsq;
    real                rt, r, eps; 
    real                rinvsix; 
    real                Vvdwtot; 
    real                Vvdw_rep, Vvdw_disp; 
    real                c6, c12; 
    real                int_bit;
    int npair_tot, npair;
    int nhwu, nhwu_pruned;

    real *x_ptr, *Ftab_ptr;

    //im = 0, ic = 0
    //cache:cj4, 
    switch (status)
    {
    case 0:
        if(!cj4_valid){
            if((cj4 = *(mt_nbnxn_cj4_t *)(cache_info_cj4->loadDataFromBufOnlyReadAsync_ptr(cache_info_cj4, cj4_ind))) == NULL){
                updateStatusVars(cj4_ind, im,ic,jm,jc,status,npair,fix,fiy,fiz,
                        cj4,cj4_valid,excl,excl_valid,ix,iy,iz,iq,iq_valid,jx,jy,jz,jq,jq_valid,
                        typei,typei_valid,typej,typej_valid,fexcl,fexcl_valid);
                return task_ret_again;
            }
            cj4_valid = 1;
        }
        if(shift == CENTRAL && cj4.cj[0] == sci * c_numClPerSupercl)
        {
            /* we have the diagonal:
            * add the charge self interaction energy term
            */
            for(;im < c_numClPerSupercl;im++)
            {
                ci = sci * c_numClPerSupercl + im;
                for (;ic < c_clSize; ic++)
                {
                    ia = ci * c_clSize + ic;
                    if(!iq_valid){
                        if((iq = *(real*)(cache_info_x->loadDataFromBufOnlyReadAsync_ptr(cache_info_x, ia * xstride + 3))) == NULL){
                            updateStatusVars(cj4_ind, im,ic,jm,jc,status,npair,fix,fiy,fiz,
                                cj4,cj4_valid,excl,excl_valid,ix,iy,iz,iq,iq_valid,jx,jy,jz,jq,jq_valid,
                                typei,typei_valid,typej,typej_valid,fexcl,fexcl_valid);
                            return task_ret_again;
                        }
                        iq_valid = 1;
                    }
                    vctot += iq * iq;
                    iq_valid = 0;
                }
                ic = 0;
            }
            im = 0;
            if (!bEwald)
            {
                vctot *= -facel * 0.5 * c_rf;
            }
            else
            {
                /* last factor 1/sqrt(pi) */
                vctot *= -facel * ewaldcoeff_q * M_1_SQRTPI;
            }
        }
        cj4_valid = 0;
    // cj4_ind = cj4_ind0,jm = 0,im = 0;ic = 0;jc = 0;npair = 0;fix = 0;fiy = 0;fiz = 0;
    case 1:
        for(;cj4_ind < (cj4_ind0 + cj4_ind1)/2; cj4_ind++)
        {
            if(!cj4_valid){
                if((cj4 = *(mt_nbnxn_cj4_t *)(cache_info_cj4->loadDataFromBufOnlyReadAsync_ptr(cache_info_cj4, cj4_ind))) == NULL){
                    updateStatusVars(cj4_ind, im,ic,jm,jc,status,npair,fix,fiy,fiz,
                        cj4,cj4_valid,excl,excl_valid,ix,iy,iz,iq,iq_valid,jx,jy,jz,jq,jq_valid,
                        typei,typei_valid,typej,typej_valid,fexcl,fexcl_valid);
                    return task_ret_again;
                }
                cj4_valid = 1;
            }
            if(!excl_valid){
                if((excl = *(mt_nbnxn_excl_t*)(cache_info_excl->loadDataFromBufOnlyReadAsync_ptr(cache_info_excl, cj4.imei[0].excl_ind))) == NULL){
                    updateStatusVars(cj4_ind, im,ic,jm,jc,status,npair,fix,fiy,fiz,
                        cj4,cj4_valid,excl,excl_valid,ix,iy,iz,iq,iq_valid,jx,jy,jz,jq,jq_valid,
                        typei,typei_valid,typej,typej_valid,fexcl,fexcl_valid);
                    return task_ret_again;
                }
                excl_valid = 1;
            }

            for(;jm < c_nbnxnGpuJgroupSize; jm++)
            {
                cj = cj4.cj[jm];
                for(;im < c_numClPerSupercl; im++){
                    if((cj4.imei[0].imask >> (jm * c_numClPerSupercl + im)) & 1)
                    {
                        within_rlist = 0;
                        ci = sci * c_numClPerSupercl + im;

                        for(;ic < c_clSize; ic++){
                            ia = ci * c_clSize + ic;

                            is = ia * xstride;
                            ifs = ia * fstride;
                            if(!iq_valid){
                                if((x_ptr = cache_info_x->loadDataFromBufOnlyReadAsync_ptr(cache_info_x, is)) == NULL){
                                    updateStatusVars(cj4_ind, im,ic,jm,jc,status,npair,fix,fiy,fiz,
                                        cj4,cj4_valid,excl,excl_valid,ix,iy,iz,iq,iq_valid,jx,jy,jz,jq,jq_valid,
                                        typei,typei_valid,typej,typej_valid,fexcl,fexcl_valid);
                                    return task_ret_again;
                                }
                                ix = shX + *x_ptr;
                                iy = shY + *(x_ptr + 1);
                                iz = shZ + *(x_ptr + 2);
                                iq = facel + *(x_ptr + 3);
                                iq_valid = 1;
                            }
                            if(!typei_valid){
                                if((typei = *(int *)(cache_info_type->loadDataFromBufOnlyReadAsync_ptr(cache_info_type, ia))) == NULL){
                                    updateStatusVars(cj4_ind, im,ic,jm,jc,status,npair,fix,fiy,fiz,
                                        cj4,cj4_valid,excl,excl_valid,ix,iy,iz,iq,iq_valid,jx,jy,jz,jq,jq_valid,
                                        typei,typei_valid,typej,typej_valid,fexcl,fexcl_valid);
                                    return task_ret_again;
                                }
                                typei_valid = 1;
                            }
                            nti = ntype * 2 * typei;

                            for(;jc < c_clSize; jc++)
                            {
                                ja = cj * c_clSize + jc;
                                if(shift == CENTRAL && ci == cj && ja<=ia)
                                {
                                    continue;
                                }
                                int_bit = (real)(
                                        (excl.pair[(jc & (c_nbnxnGpuClusterSize - 1)) * c_clSize + ic]
                                            >> (jm * c_numClPerSupercl + im))
                                        & 1);
                                js  = ja * xstride;
                                jfs = ja * fstride;
                                if(!jq_valid){
                                    if((x_ptr = cache_info_x->loadDataFromBufOnlyReadAsync_ptr(cache_info_x, js)) == NULL){
                                        updateStatusVars(cj4_ind, im,ic,jm,jc,status,npair,fix,fiy,fiz,
                                            cj4,cj4_valid,excl,excl_valid,ix,iy,iz,iq,iq_valid,jx,jy,jz,jq,jq_valid,
                                            typei,typei_valid,typej,typej_valid,fexcl,fexcl_valid);
                                        return task_ret_again;
                                    }
                                    jx = *x_ptr;
                                    jy = *(x_ptr + 1);
                                    jz = *(x_ptr + 2);
                                    jq = *(x_ptr + 3);
                                    jq_valid = 1;
                                }

                                dx  = ix - jx;
                                dy  = iy - jy;
                                dz  = iz - jz;
                                rsq = dx * dx + dy * dy + dz * dz;
                                if (rsq < rlist2)
                                {
                                    within_rlist = true;
                                }
                                if (rsq >= rcut2)
                                {
                                    continue;
                                }

                                if(!typej_valid){
                                    if((typej = *(int *)(cache_info_type->loadDataFromBufOnlyReadAsync_ptr(cache_info_type, ja))) == NULL){
                                        updateStatusVars(cj4_ind, im,ic,jm,jc,status,npair,fix,fiy,fiz,
                                            cj4,cj4_valid,excl,excl_valid,ix,iy,iz,iq,iq_valid,jx,jy,jz,jq,jq_valid,
                                            typei,typei_valid,typej,typej_valid,fexcl,fexcl_valid);
                                        return task_ret_again;
                                    }
                                    typej_valid = 1;
                                }

                                if (typei != ntype - 1 && typej != ntype - 1)
                                {
                                    npair++;
                                }

                                // Ensure distance do not become so small that r^-12 overflows
                                rsq = (rsq > NBNXN_MIN_RSQ) ? rsq : NBNXN_MIN_RSQ;

                                rinv   = 1.0F / sqrtf(rsq);
                                rinvsq = rinv * rinv;
                                qq = iq * jq;
                                if (!bEwald)
                                {
                                    /* Reaction-field */
                                    krsq  = k_rf * rsq;
                                    fscal = qq * (int_bit * rinv - 2 * krsq) * rinvsq;
                                    if (computeEnergy)
                                    {
                                        vcoul = qq * (int_bit * rinv + krsq - c_rf);
                                    }
                                }
                                else
                                {
                                    r   = rsq * rinv;
                                    rt  = r * coulombEwaldTables_scale;
                                    n0  = (int)(rt);
                                    eps = rt - (real)(n0);

                                    if(!fexcl_valid){
                                        if((Ftab_ptr = cache_info_Ftab->loadDataFromBufOnlyReadAsync_ptr(cache_info_Ftab, n0)) == NULL){
                                            updateStatusVars(cj4_ind, im,ic,jm,jc,status,npair,fix,fiy,fiz,
                                                cj4,cj4_valid,excl,excl_valid,ix,iy,iz,iq,iq_valid,jx,jy,jz,jq,jq_valid,
                                                typei,typei_valid,typej,typej_valid,fexcl,fexcl_valid);
                                            return task_ret_again;
                                        }
                                        fexcl = (1 - eps) * *Ftab_ptr + eps * *(Ftab_ptr+1);
                                        fexcl_valid = 1;
                                    }

                                    fscal = qq * (int_bit * rinvsq - fexcl) * rinv;

                                    if (computeEnergy)
                                    {
                                        vcoul = qq
                                                * ((int_bit - erff(ewaldcoeff_q * r)) * rinv//TODO
                                                    - int_bit * sh_ewald);
                                    }
                                }

                                if (rsq < rvdw2)
                                {
                                    tj = nti + 2 * typej;

                                    /* Vanilla Lennard-Jones cutoff */
                                    c6  = vdwparam[tj];
                                    c12 = vdwparam[tj + 1];

                                    rinvsix   = int_bit * rinvsq * rinvsq * rinvsq;
                                    Vvdw_disp = c6 * rinvsix;
                                    Vvdw_rep  = c12 * rinvsix * rinvsix;
                                    fscal += (Vvdw_rep - Vvdw_disp) * rinvsq;

                                    if (computeEnergy)
                                    {
                                        vctot += vcoul;

                                        Vvdwtot +=
                                                (Vvdw_rep + int_bit * c12 * repulsion_shift_cpot) / 12
                                                - (Vvdw_disp
                                                    + int_bit * c6 * dispersion_shift_cpot)
                                                            / 6;
                                    }
                                }

                                tx  = fscal * dx;
                                ty  = fscal * dy;
                                tz  = fscal * dz;
                                fix = fix + tx;
                                fiy = fiy + ty;
                                fiz = fiz + tz;
                                f[jfs + 0] -= tx;
                                f[jfs + 1] -= ty;
                                f[jfs + 2] -= tz;

                                jq_valid = 0;
                                typej_valid = 0;
                                fexcl_valid = 0;
                            }
                            jc = 0;
                            
                            f[ifs + 0] += fix;
                            f[ifs + 1] += fiy;
                            f[ifs + 2] += fiz;
                            fshift[ish3]     = fshift[ish3] + fix;
                            fshift[ish3 + 1] = fshift[ish3 + 1] + fiy;
                            fshift[ish3 + 2] = fshift[ish3 + 2] + fiz;

                            /* Count in half work-units.
                                * In CUDA one work-unit is 2 warps.
                                */
                            if ((ic + 1) % (c_clSize / c_nbnxnGpuClusterpairSplit) == 0)
                            {
                                npair_tot += npair;

                                nhwu++;
                                if (within_rlist)
                                {
                                    nhwu_pruned++;
                                }

                                within_rlist = 0;
                                npair        = 0;
                            }

                            iq_valid = 0;
                            typei_valid = 0;
                            fix = 0;
                            fiy = 0;
                            fiz = 0;
                        }
                        ic = 0;
                        npair = 0;
                    }
                }
                im = 0;
            }
            jm = 0;
            cj4_valid = 0;
            excl_valid = 0;
        }
        if (computeEnergy)
        {
            ggid       = 0;
            Vc[ggid]   = Vc[ggid] + vctot;
            Vvdw[ggid] = Vvdw[ggid] + Vvdwtot;
        }

    default:
        break;
    }
    
    hthread_printf("calculateFOp finish!\n");
    return task_ret_ok;
}
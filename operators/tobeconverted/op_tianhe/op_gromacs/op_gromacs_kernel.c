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

//floatConst
real                rcut2, rvdw2, rlist2;
real                facel;
real                c_rf, k_rf;
real                coulombEwaldTables_scale;
real                ewaldcoeff_q;
real                sh_ewald;
real                repulsion_shift_cpot, dispersion_shift_cpot;
int xstride, fstride;
int ntype;
int               computeEnergy;
int               bEwald;
int sci_list_size = 8;
real*             vdwparam;

__global__ task_ret initialOp(void** inputs, void** outputs){
    real* vdwparam_ = inputs[0];
    mt_nbnxn_sci_t*  sci_list_src = inputs[1];
    mt_nbnxn_sci_t*  sci_list_buffer = scalar_malloc(sci_list_size * sizeof(mt_nbnxn_sci_t));

    vdwparam = scalar_malloc(4 * sizeof(real));
    int chl1 = dma_p2p(vdwparam_, 1, 4*sizeof(real), 0, vdwparam, 1, 4*sizeof(real), 0, 0, 0);
    int chl2 = dma_p2p(sci_list_src, 1, sci_list_size * sizeof(mt_nbnxn_sci_t), 0,
                        sci_list_buffer, 1, sci_list_size * sizeof(mt_nbnxn_sci_t), 0, 0, 0);
    real* floatConst = inputs[2];
    int ntype_ = inputs[3];
    int bEwald_ = inputs[4];
    int xstride_ = inputs[5];
    int fstride_ = inputs[6];
    int computeEnergy_ = inputs[7];
    int na_sc = inputs[8];
    cache_info_t* cache_info_cj4 = inputs[9];
    cache_info_t* cache_info_excls = inputs[10];
    cache_info_t* cache_info_type = inputs[11];
    cache_info_t* cache_info_x = inputs[12];
    cache_info_t* cache_info_Ftab = inputs[13];
    cache_info_t* cache_info_shiftvec = inputs[14];
    cache_info_cj4->autoPrefetch_ptr(cache_info_cj4);
    cache_info_excls->autoPrefetch_ptr(cache_info_excls);
    cache_info_type->autoPrefetch_ptr(cache_info_type);
    cache_info_x->autoPrefetch_ptr(cache_info_x);
    cache_info_Ftab->autoPrefetch_ptr(cache_info_Ftab);
    cache_info_shiftvec->autoPrefetch_ptr(cache_info_shiftvec);
    
    //floatConst decode
    rcut2 = floatConst[0];
    rvdw2 = floatConst[1];
    rlist2 = floatConst[2];
    facel = floatConst[3];
    c_rf = floatConst[4];
    ewaldcoeff_q = floatConst[5];
    k_rf = floatConst[6];
    coulombEwaldTables_scale = floatConst[7];
    sh_ewald = floatConst[8];
    repulsion_shift_cpot = floatConst[9];
    dispersion_shift_cpot = floatConst[10];

    xstride = xstride_;
    fstride = fstride_;
    ntype = ntype_;
    computeEnergy = computeEnergy_;
    bEwald = bEwald_;

    int thread_id = get_thread_id();
    unsigned long start, end;
    if(thread_id < na_sc%dsp_count){
        start = thread_id*na_sc/dsp_count + thread_id;
        end = start + na_sc/dsp_count + 1;
    }else{
        start = thread_id*na_sc/dsp_count + na_sc%dsp_count;
        end = start + na_sc/dsp_count;
    }

    dma_wait(chl1);
    dma_wait(chl2);

    obj_mem mem_sci_list_buffer = (obj_mem)outputs[0];
    mem_sci_list_buffer->p = (void*)sci_list_buffer;
    mem_sci_list_buffer->size = sizeof(mt_nbnxn_sci_t);
    mem_sci_list_buffer->bufsize  = sci_list_size;
    obj_var var_start = (obj_var)outputs[1];
    obj_var var_end = (obj_var)outputs[2];
    var_start->t_uint = start;
    var_end->t_uint = end;

    return task_ret_done;
}

__global__ task_ret getSciOp(void** inputs, void** outputs){
    unsigned long cur = ((obj_var)inputs[0])->t_uint;
    unsigned long end = ((obj_var)inputs[1])->t_uint;
    if(cur >= end){
        return task_ret_done;
    }
    mt_nbnxn_sci_t* sci_list_src = inputs[2];
    mt_nbnxn_sci_t* sci_list_buffer = ((obj_mem)inputs[3])->p;
    cache_info_t* cache_info_shiftvec = inputs[4];
    mt_nbnxn_sci_t nbln = sci_list_buffer[cur];
    real* sh_ptr = (real*)(cache_info_shiftvec->loadDataFromBufOnlyRead_ptr(cache_info_shiftvec, 3* nbln.shift));
    if(cur%sci_list_size == 0){
        int chl = dma_p2p(sci_list_src, 1, sci_list_size * sizeof(mt_nbnxn_sci_t), 0, 
            sci_list_buffer, 1, sci_list_size * sizeof(mt_nbnxn_sci_t), 0, 0, 0);//dma copy
        //需要非阻塞
        dma_wait(chl);
    }
    
    obj_mem mem_nbnxn_sci = (obj_mem)outputs[0];
    if(mem_nbnxn_sci->p == NULL){
        mem_nbnxn_sci->p = (void*)scalar_malloc(sizeof(mt_nbnxn_sci_t));
        mem_nbnxn_sci->size = sizeof(mt_nbnxn_sci_t);
        mem_nbnxn_sci->bufsize  = 1;
    }
    ((mt_nbnxn_sci_t*)mem_nbnxn_sci->p)[0] = nbln;

    obj_mem mem_3shift_values = (obj_mem)outputs[1];
    if(mem_3shift_values->p == NULL){
        mem_3shift_values->p = (void*)scalar_malloc( 3*sizeof(real));
        mem_3shift_values->size = sizeof(real);
        mem_3shift_values->bufsize  = 3;
    }
    memcpy(mem_3shift_values->p, sh_ptr, 3*sizeof(real));
    inputs[0] = cur+1;

    return task_ret_ok;

}

__global__ task_ret calculateFOp1(void** inputs, void** outputs){
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

    int cj4_ind;
    int                 ci, cj;
    int                 ic, jc, ia, ja, is, ifs, js, jfs, im, jm;
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
    real                fexcl; 
    real                c6, c12; 
    real                int_bit;
    int npair_tot, npair;
    int nhwu, nhwu_pruned;

    real *x_ptr, *Ftab_ptr;
    int typei, typej;

    mt_nbnxn_cj4_t cj4;
    mt_nbnxn_excl_t*       excl;
    cj4 = *(mt_nbnxn_cj4_t *)(cache_info_cj4->loadDataFromBufOnlyRead_ptr(cache_info_cj4, cj4_ind));

    if(shift == CENTRAL && cj4.cj[0] == sci * c_numClPerSupercl)
    {
        /* we have the diagonal:
        * add the charge self interaction energy term
        */
        for (im = 0; im < c_numClPerSupercl; im++)
        {
            ci = sci * c_numClPerSupercl + im;
            for (ic = 0; ic < c_clSize; ic++)
            {
                ia = ci * c_clSize + ic;
                x_ptr = cache_info_x->loadDataFromBufOnlyRead_ptr(cache_info_x, ia * xstride + 3);
                iq = *x_ptr;
                vctot += iq * iq;
            }
        }
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
    for(cj4_ind = cj4_ind0; cj4_ind < (cj4_ind0 + cj4_ind1)/2; cj4_ind++)
    {
        cj4 = *(mt_nbnxn_cj4_t *)(cache_info_cj4->loadDataFromBufOnlyRead_ptr(cache_info_cj4, cj4_ind));
        excl = cache_info_excl->loadDataFromBufOnlyRead_ptr(cache_info_excl, cj4.imei[0].excl_ind);
        for(jm = 0; jm < c_nbnxnGpuJgroupSize; jm++)
        {
            cj = cj4.cj[jm];
            for(im = 0; im < c_numClPerSupercl; im++){
                if((cj4.imei[0].imask >> (jm * c_numClPerSupercl + im)) & 1)
                {
                    within_rlist = 0;
                    npair = 0;
                    ci = sci * c_numClPerSupercl + im;

                    for(ic = 0; ic < c_clSize; ic++){
                        ia = ci * c_clSize + ic;

                        is = ia * xstride;
                        ifs = ia * fstride;
                        x_ptr = cache_info_x->loadDataFromBufOnlyRead_ptr(cache_info_x, is);
                        ix = shX + *x_ptr;
                        iy = shY + *(x_ptr + 1);
                        iz = shZ + *(x_ptr + 2);
                        iq = facel + *(x_ptr + 3);
                        typei = *(int *)(cache_info_type->loadDataFromBufOnlyRead_ptr(cache_info_type, ia));
                        nti = ntype * 2 * typei;

                        fix = 0;
                        fiy = 0;
                        fiz = 0;

                        for(jc = 0; jc < c_clSize; jc++)
                        {
                            ja = cj * c_clSize + jc;
                            if(shift == CENTRAL && ci == cj && ja<=ia)
                            {
                                continue;
                            }
                            int_bit = (real)(
                                    (excl->pair[(jc & (c_nbnxnGpuClusterSize - 1)) * c_clSize + ic]
                                        >> (jm * c_numClPerSupercl + im))
                                    & 1);
                            js  = ja * xstride;
                            jfs = ja * fstride;
                            x_ptr = cache_info_x->loadDataFromBufOnlyRead_ptr(cache_info_x, js);
                            jx = *x_ptr;
                            jy = *(x_ptr + 1);
                            jz = *(x_ptr + 2);
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

                            typej = *(int *)(cache_info_type->loadDataFromBufOnlyRead_ptr(cache_info_type, ja));
                            if (typei != ntype - 1 && typej != ntype - 1)
                            {
                                npair++;
                            }

                            // Ensure distance do not become so small that r^-12 overflows
                            rsq = (rsq > NBNXN_MIN_RSQ) ? rsq : NBNXN_MIN_RSQ;

                            rinv   = 1.0F / sqrtf(rsq);
                            rinvsq = rinv * rinv;
                            qq = iq * *(x_ptr + 3);
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
                                Ftab_ptr = cache_info_Ftab->loadDataFromBufOnlyRead_ptr(cache_info_Ftab, n0);
                                fexcl = (1 - eps) * *Ftab_ptr + eps * *(Ftab_ptr+1);

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
                        }

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
                    }
                }


            }
        }
    }
    if (computeEnergy)
    {
        ggid       = 0;
        Vc[ggid]   = Vc[ggid] + vctot;
        Vvdw[ggid] = Vvdw[ggid] + Vvdwtot;
    }
    hthread_printf("calculateFOp finish!\n");
    return task_ret_ok;
}



__global__ task_ret calculateFOp2(void** inputs, void** outputs){
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

    int cj4_ind;
    int                 ci, cj;
    int                 ic, jc, ia, ja, is, ifs, js, jfs, im, jm;
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
    real                fexcl; 
    real                c6, c12; 
    real                int_bit;
    int npair_tot, npair;
    int nhwu, nhwu_pruned;

    real *x_ptr, *Ftab_ptr;
    int typei, typej;

    mt_nbnxn_cj4_t cj4;
    mt_nbnxn_excl_t*       excl;
    cj4 = *(mt_nbnxn_cj4_t *)(cache_info_cj4->loadDataFromBufOnlyRead_ptr(cache_info_cj4, cj4_ind));
    for(cj4_ind = (cj4_ind0 + cj4_ind1)/2; cj4_ind < cj4_ind1; cj4_ind++)
    {
        cj4 = *(mt_nbnxn_cj4_t *)(cache_info_cj4->loadDataFromBufOnlyRead_ptr(cache_info_cj4, cj4_ind));
        excl = cache_info_excl->loadDataFromBufOnlyRead_ptr(cache_info_excl, cj4.imei[0].excl_ind);
        for(jm = 0; jm < c_nbnxnGpuJgroupSize; jm++)
        {
            cj = cj4.cj[jm];
            for(im = 0; im < c_numClPerSupercl; im++){
                if((cj4.imei[0].imask >> (jm * c_numClPerSupercl + im)) & 1)
                {
                    within_rlist = 0;
                    npair = 0;
                    ci = sci * c_numClPerSupercl + im;

                    for(ic = 0; ic < c_clSize; ic++){
                        ia = ci * c_clSize + ic;

                        is = ia * xstride;
                        ifs = ia * fstride;
                        x_ptr = cache_info_x->loadDataFromBufOnlyRead_ptr(cache_info_x, is);
                        return task_ret_again;
                        ix = shX + *x_ptr;
                        iy = shY + *(x_ptr + 1);
                        iz = shZ + *(x_ptr + 2);
                        iq = facel + *(x_ptr + 3);
                        typei = *(int *)(cache_info_type->loadDataFromBufOnlyRead_ptr(cache_info_type, ia));
                        nti = ntype * 2 * typei;

                        fix = 0;
                        fiy = 0;
                        fiz = 0;

                        for(jc = 0; jc < c_clSize; jc++)
                        {
                            ja = cj * c_clSize + jc;
                            if(shift == CENTRAL && ci == cj && ja<=ia)
                            {
                                continue;
                            }
                            int_bit = (real)(
                                    (excl->pair[(jc & (c_nbnxnGpuClusterSize - 1)) * c_clSize + ic]
                                        >> (jm * c_numClPerSupercl + im))
                                    & 1);
                            js  = ja * xstride;
                            jfs = ja * fstride;
                            x_ptr = cache_info_x->loadDataFromBufOnlyRead_ptr(cache_info_x, js);
                            jx = *x_ptr;
                            jy = *(x_ptr + 1);
                            jz = *(x_ptr + 2);
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

                            typej = *(int *)(cache_info_type->loadDataFromBufOnlyRead_ptr(cache_info_type, ja));
                            if (typei != ntype - 1 && typej != ntype - 1)
                            {
                                npair++;
                            }

                            // Ensure distance do not become so small that r^-12 overflows
                            rsq = (rsq > NBNXN_MIN_RSQ) ? rsq : NBNXN_MIN_RSQ;

                            rinv   = 1.0F / sqrtf(rsq);
                            rinvsq = rinv * rinv;
                            qq = iq * *(x_ptr + 3);
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
                                Ftab_ptr = cache_info_Ftab->loadDataFromBufOnlyRead_ptr(cache_info_Ftab, n0);
                                fexcl = (1 - eps) * *Ftab_ptr + eps * *(Ftab_ptr+1);

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
                        }

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
                    }
                }


            }
        }
    }
    if (computeEnergy)
    {
        ggid       = 0;
        Vc[ggid]   = Vc[ggid] + vctot;
        Vvdw[ggid] = Vvdw[ggid] + Vvdwtot;
    }
    hthread_printf("calculateFOp finish!\n");
    return task_ret_ok;
}


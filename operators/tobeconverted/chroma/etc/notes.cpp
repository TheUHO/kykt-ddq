/**
 * @file notes.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-27
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "chroma_operators.h"

/////////////////// MACRO DEFINATION //////////////////////
/**
 * @brief _CASE_LATTICE_CLASS
 * @brief SWITCH_LATTICE_CLASS_REF
 */
#define _CASE_LATTICE_CLASS(T, phi, input_i, ...)                                                  \
    case e##T: {                                                                                   \
        DEF_REF(T, phi, input_i);                                                                  \
        __VA_ARGS__;                                                                               \
        break;                                                                                     \
    }

#define SWITCH_LATTICE_CLASS_REF(_etype, phi, input_i, ...)                                        \
    do {                                                                                           \
        switch (_etype) {                                                                          \
            _CASE_LATTICE_CLASS(LatticeColorVector, phi, input_i, __VA_ARGS__);                    \
            _CASE_LATTICE_CLASS(LatticeFermion, phi, input_i, __VA_ARGS__);                        \
            _CASE_LATTICE_CLASS(LatticePropagator, phi, input_i, __VA_ARGS__);                     \
            _CASE_LATTICE_CLASS(LatticeStaggeredFermion, phi, input_i, __VA_ARGS__);               \
            _CASE_LATTICE_CLASS(LatticeStaggeredPropagator, phi, input_i, __VA_ARGS__);            \
        default: {                                                                                 \
            printf("datatype error!\n");                                                           \
        }                                                                                          \
        }                                                                                          \
    } while (0)

SWITCH_LATTICE_CLASS_REF(type, phi, input[3], RunQuarkDisplacement(xml_param, u, eisign, phi));

///////////////////// LAYOUT ///////////////////////
#if CMakeLists
/**
 * @file src/qdpxx/CMakeLists.txt
 */
// line: 131-149
if( ${qdpLayout} STREQUAL "lexico") 
  message( STATUS "QDP++: Enabling Lexicographic Layout" )
  set(QDP_USE_LEXICO_LAYOUT ON)
  
elseif( ${qdpLayout} STREQUAL "cb2")
  message( STATUS "QDP++: Enabling CB2 (4D Checkerboard) Layout" )
  set(QDP_USE_CB2_LAYOUT ON)
  
elseif( ${qdpLayout} STREQUAL "cb32") 
  message( STATUS "QDP++: Enabling CB32 (32 colored ) Layout" )
  set(QDP_USE_CB32_LAYOUT ON)

elseif( ${qdpLayout} STREQUAL "cb3d")
  message( STATUS "QDP++: Enabling CB3D (3D checkerboarded ) Layout" )
  set(QDP_USE_CB3D_LAYOUT ON)

else()
  message( ERROR "QDP++: Unknown layout. Please set either QDP_LAYOUT_LEXICO, QDP_LAYOUT_CB2, QDP_LAYOUT_CB3D or QDP_LAYOUT_CB32")
endif()

// line: 24
set(QDP_LAYOUT "cb2" CACHE STRING "The QDP layout: lexico, cb2, cb32, cb3d (default is cb2)")

/**
 * @file build/qdpxx/CMakeCache.txt
 */
// line: 479-480
//The QDP layout: lexico, cb2, cb32, cb3d (default is cb2)
QDP_LAYOUT:STRING=cb2

/**
 * @file build/qdpxx/QDPXXConfig.cmake
 */
// line: 51
set(QDP_LAYOUT    cb2)

#endif

/**
 * @file qdpxx/lib/qdp_parscalar_layout.cc
 * 
 */
// line: 649-685
#if QDP_USE_CB2_LAYOUT == 1

QDPXX_MESSAGE("Using a 2 checkerboard (red/black) layout")

namespace Layout {
//! The linearized site index for the corresponding coordinate
/*! This layout is appropriate for a 2 checkerboard (red/black) lattice */
int linearSiteIndex(const multi1d<int> &coord)
{
    // 单节点上的格子体积的一半, 即奇或偶格子的体积： subgrid_vol_cb = Subgrid lattice volume / 2
    int subgrid_vol_cb = Layout::sitesOnNode() >> 1;

    // 单节点上的格子的
    // Layout::subgridLattSize()= _layout.subgrid_nrow[i] = _layout.nrow[i] / _layout.logical_size[i];
    // _layout.logical_size[i] =  QMP_get_logical_dimensions();
    multi1d<int> subgrid_cb_nrow = Layout::subgridLattSize();
    subgrid_cb_nrow[0] >>= 1; // subgrid_cb_nrow /=2, 因此 nrow[:]中，至少nrow[0] 须为偶数

    // 判读输入global坐标 corrd[] 的奇偶性:cb=0,偶；cb=1,奇
    // cb = corrd[x] + corrd[y] + corrd[z] + corrd[t];
    int cb = 0;
    for (int m = 0; m < Nd; ++m)
        cb += coord[m];
    cb &= 1; //按位与: cb = cb & 0b0001

    // 计算节点内的 local 坐标
    multi1d<int> subgrid_cb_coord(Nd);
    subgrid_cb_coord[0] = (coord[0] >> 1) % subgrid_cb_nrow[0];
    for (int i = 1; i < Nd; ++i)
        subgrid_cb_coord[i] = coord[i] % subgrid_cb_nrow[i];
    
    // 由此可见 偶数坐标部分放前边，奇数坐标部分放后边
    return local_site(subgrid_cb_coord, subgrid_cb_nrow) + cb * subgrid_vol_cb;
}
}

/**
 * @file qdpxx/lib/qdp_layout.cc 
 * 
 */

// line: 148-163
//! Calculates the lexicographic site index from the coordinate of a site
/*! 
 * Nothing specific about the actual lattice size, can be used for any kind of latt size 
 */
// coord[0] goes fastest; coord[1] goes slowest.
// 如坐标 (x,y,z,t)， siteindex =  x+(y+(z+t*LZ)*LY)*LX
int local_site(const multi1d<int> &coord, const multi1d<int> &latt_size)
{
    int order = 0;
    // order =  x+(y+(z+t*LZ)*LY)*LX, if coord = {x,y,z,t};
    for (int mmu = latt_size.size() - 1; mmu >= 1; --mmu)
        order = latt_size[mmu - 1] * (coord[mmu] + order);
    order += coord[0];
    return order;
}

#endif


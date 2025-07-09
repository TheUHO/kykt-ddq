#ifndef OP_CHROMA_H
#define OP_CHROMA_H

#include "task_types.h"
#include "ddq.h"
#include "chroma_types.h"

#define DELETE_CLASS_HEADER(T) void delete_##T(void *ptr)

#define NEW_DELETE_CLASS_HEADER(T)                                                                 \
    void *new_##T();                                                                               \
    void delete_##T(void *ptr)

#define NEW_DELETE_TCLASS_HEADER(T1, T2, ...)                                                      \
    void *new_##T1##T2();                                                                          \
    void delete_##T1##T2(void *ptr);                                                               \
    task_ret op_Copy##T1##T2(void **input, void **output)

#define OBJ_IMPORT_CLASS_HEADER(T)                                                                 \
    void delete_##T(void *ptr);                                                                    \
    obj obj_import_##T(T t, enum_obj_prop prop)

#define NEW_DELETE_SOBJ_CLASS_HEADER(T)                                                            \
    obj *sobj_new_##T(int n_obj, enum_obj_prop prop);                                              \
    void sobj_delete_##T(obj *ptr)

#define INIT_GAUSS_CLASS_HEADER(T) task_ret op_InitGaussian##T(void **input, void **output)

#define WRAP_LATTICE_CLASS_HEADER(T)                                                               \
    NEW_DELETE_CLASS_HEADER(T);                                                                    \
    INIT_GAUSS_CLASS_HEADER(T)

#ifdef __cplusplus
extern "C"
{
#endif

    NEW_DELETE_CLASS_HEADER(GroupXML_t);
    NEW_DELETE_CLASS_HEADER(ChromaProp_t);

    NEW_DELETE_TCLASS_HEADER(multi2d, DComplex);
    NEW_DELETE_TCLASS_HEADER(multi2d, Real);
    NEW_DELETE_TCLASS_HEADER(multi1d, LatticeColorMatrix);

    DELETE_CLASS_HEADER(SftMom);

    OBJ_IMPORT_CLASS_HEADER(int);
    OBJ_IMPORT_CLASS_HEADER(uint_t);

    NEW_DELETE_SOBJ_CLASS_HEADER(LatticePropagator);
    NEW_DELETE_SOBJ_CLASS_HEADER(LatticeGauge);

    WRAP_LATTICE_CLASS_HEADER(LatticeColorVector);
    WRAP_LATTICE_CLASS_HEADER(LatticePropagator);
    WRAP_LATTICE_CLASS_HEADER(LatticeFermion);
    WRAP_LATTICE_CLASS_HEADER(LatticeStaggeredFermion);
    WRAP_LATTICE_CLASS_HEADER(LatticeStaggeredPropagator);
    WRAP_LATTICE_CLASS_HEADER(LatticeGauge);

    task_ret op_ChromaInitialize(void *input[4], void **output);
    task_ret op_ChromaFinalize(void **input, void **output);

    task_ret op_InitRNG(void **input, void **output);
    task_ret op_CreateLayout(void **input, void **output);

    task_ret op_LoadConfig(void **input, void **output);
    task_ret op_MesPlq(void **input, void **output);

    task_ret op_RunQuarkSourceConstruction(void **input, void **output);

    task_ret op_GetNamedGauge(void **input, void **output);
    task_ret op_GetNamedLatticePropagator(void **input, void **output);
    task_ret op_SetNamedLatticePropagator(void **input, void **output);

    task_ret op_DoInlinePropagatorInverter(void **input, void **output);

    task_ret op_ComputeMesons2(void **input, void **output);
    task_ret op_ComputeCurcor2(void **input, void **output);
    task_ret op_ComputeBarhqlq(void **input, void **output);

    task_ret op_Displacement(void **input, void **output);

    task_ret op_RunQuarkDisplacement(void **input, void **output);
    task_ret op_RunLinkSmearing(void **input, void **output);
    task_ret op_RunSinkSmearing(void **input, void **output);
    task_ret op_RunQuarkSmearing(void **input, void **output);
    task_ret op_RunInlineMeasurement(void **input, void **output);

    task_ret op_WriteGauge(void **input, void **output);
    task_ret op_ReadGauge(void **input, void **output);
    task_ret op_WriteLatticePropagator(void **input, void **output);
    task_ret op_ReadLatticePropagator(void **input, void **output);
    task_ret op_WriteLatticeFermion(void **input, void **output);
    task_ret op_ReadLatticeFermion(void **input, void **output);
    task_ret op_WriteMulti2d(void **input, void **output);

    obj obj_import_GroupXML_t(const char *id, const char *path, const char *xml,
                              enum_obj_prop prop);
    obj obj_import_ChromaProp_t(const char *fermact_id, const char *fermact_path,
                                const char *fermact_xml, const char *invParam_id,
                                const char *invParam_path, const char *invParam_xml,
                                int quarkSpinType, int obsvP, enum_obj_prop prop);
    obj obj_import_SftMom(int mom2_max, int x0, int y0, int z0, int t0, int avg_equiv_mom,
                          int j_decay, enum_obj_prop prop);
    obj obj_import_XMLOutputInstance(enum_obj_prop prop);

#ifdef __cplusplus
}
#endif
#endif

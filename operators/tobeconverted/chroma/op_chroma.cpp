#include "op_chroma.h"
#include "chroma.h"
#include "chroma_operators.h"
#include "task_types.h"
using namespace std;

#ifdef NDEBUG
#define log_printf(...)
#else
#define log_printf(...) printf(__VA_ARGS__)
#endif

#define DEF_VAL(type, var, input_i) type var = *(type *) input_i
#define DEF_REF(type, var, input_i) type &var = *(type *) input_i
#define DEF_PTR(type, var, input_i) type *var = (type *) input_i
#define DEF_CPTR(type, var, input_i) const type *var = (const type *) input_i

#define SET_VAL(type, output_i, var) (*(type *) output_i) = (type) var

#define AS_VAL(type, input_i) (*(type *) input_i)
#define AS_PTR(type, input_i) ((type *) input_i)
#define AS_CPTR(type, input_i) ((const type *) input_i)

#define STR_MSG(T) #T

#define DELETE_CLASS(T)                                                                            \
    void delete_##T(void *ptr)                                                                     \
    {                                                                                              \
        DEF_PTR(T, var, ptr);                                                                      \
        log_printf("delete_" STR_MSG(T) ": %p\n", var);                                            \
        delete (var);                                                                              \
    }

#define NEW_DELETE_CLASS(T, ...)                                                                   \
    void *new_##T()                                                                                \
    {                                                                                              \
        T *var = new T(__VA_ARGS__);                                                               \
        log_printf("new_" STR_MSG(T) ": %p\n", var);                                               \
        return var;                                                                                \
    }                                                                                              \
    void delete_##T(void *ptr)                                                                     \
    {                                                                                              \
        DEF_PTR(T, var, ptr);                                                                      \
        log_printf("delete_" STR_MSG(T) ": %p\n", var);                                            \
        delete (var);                                                                              \
    }

#define NEW_DELETE_TCLASS(T1, T2, ...)                                                             \
    void *new_##T1##T2()                                                                           \
    {                                                                                              \
        T1<T2> *var = new T1<T2>(__VA_ARGS__);                                                     \
        log_printf("new_" STR_MSG(T1) "<" STR_MSG(T2) ">: %p\n", var);                             \
        return var;                                                                                \
    }                                                                                              \
    void delete_##T1##T2(void *ptr)                                                                \
    {                                                                                              \
        DEF_PTR(T1<T2>, var, ptr);                                                                 \
        log_printf("delete_" STR_MSG(T1) "<" STR_MSG(T2) ">: %p\n", var);                          \
        delete (var);                                                                              \
    }                                                                                              \
    task_ret op_Copy##T1##T2(void **input, void **output)                                          \
    {                                                                                              \
        DEF_REF(T1<T2>, src, input[0]);                                                            \
        DEF_REF(T1<T2>, dest, output[0]);                                                          \
        dest = src;                                                                                \
        log_printf("op_Copy_" STR_MSG(T1) "<" STR_MSG(T2) ">: %p -> %p\n", input[0], output[0]);   \
        return task_ret_done;                                                                      \
    }

#define INIT_GAUSS_CLASS(T)                                                                        \
    task_ret op_InitGaussian##T(void **input, void **output)                                       \
    {                                                                                              \
        T *var = (T *) output[0];                                                                  \
        log_printf("op_CreateGaussian" STR_MSG(T) ": %p\n", var);                                  \
        gaussian((*var));                                                                          \
        return task_ret_done;                                                                      \
    }

#define WRAP_LATTICE_CLASS(T)                                                                      \
    NEW_DELETE_CLASS(T)                                                                            \
    INIT_GAUSS_CLASS(T)

#define OBJ_IMPORT_CLASS(T)                                                                        \
    DELETE_CLASS(T)                                                                                \
    obj obj_import_##T(T t, enum_obj_prop prop)                                                    \
    {                                                                                              \
        log_printf("op_import_" STR_MSG(T) ": %d\n", prop);                                        \
        T *ptr = new T(t);                                                                         \
        return obj_import(ptr, delete_##T, prop);                                                  \
    }

#define NEW_DELETE_SOBJ_CLASS(T, ...)                                                              \
    void *sobj_access_##T(int flag)                                                                \
    {                                                                                              \
        static int n_ref = 0;                                                                      \
        static void *p = NULL;                                                                     \
        if (flag > 0)                                                                              \
            n_ref = flag;                                                                          \
        else if (flag == 0 && p == NULL) {                                                         \
            p = new T;                                                                             \
            log_printf("sobj_access new " STR_MSG(T) ": %p\n", p);                                 \
        } else if (flag == -1) {                                                                   \
            n_ref--;                                                                               \
            if (n_ref == 0) {                                                                      \
                log_printf("sobj_access delete " STR_MSG(T) ": %p\n", p);                          \
                delete (T *) p;                                                                    \
            }                                                                                      \
        }                                                                                          \
        return p;                                                                                  \
    }                                                                                              \
    void *sobj_construct_##T()                                                                     \
    {                                                                                              \
        return sobj_access_##T(0);                                                                 \
    }                                                                                              \
    void sobj_destruct_##T(void *p)                                                                \
    {                                                                                              \
        sobj_access_##T(-1);                                                                       \
    }                                                                                              \
    obj *sobj_new_##T(int n_obj, enum_obj_prop prop)                                               \
    {                                                                                              \
        obj *res = new obj[n_obj];                                                                 \
        log_printf("sobj_new_" STR_MSG(T) " []%p\n", res);                                         \
        for (int i = 0; i < n_obj; i++)                                                            \
            res[i] = obj_new(sobj_construct_##T, sobj_destruct_##T, prop);                         \
        sobj_access_##T(n_obj);                                                                    \
        return res;                                                                                \
    }                                                                                              \
    void sobj_delete_##T(obj *ptr)                                                                 \
    {                                                                                              \
        log_printf("sobj_delete_" STR_MSG(T) " []%p\n", ptr);                                      \
        delete[] ptr;                                                                              \
    }

NEW_DELETE_CLASS(LatticeGauge, 4)
NEW_DELETE_CLASS(GroupXML_t)
NEW_DELETE_CLASS(ChromaProp_t)

NEW_DELETE_TCLASS(multi2d, DComplex)
NEW_DELETE_TCLASS(multi2d, Real)
NEW_DELETE_TCLASS(multi1d, LatticeColorMatrix)

DELETE_CLASS(SftMom)

NEW_DELETE_SOBJ_CLASS(LatticePropagator)
NEW_DELETE_SOBJ_CLASS(LatticeGauge)

OBJ_IMPORT_CLASS(int)
OBJ_IMPORT_CLASS(uint_t)

WRAP_LATTICE_CLASS(LatticeColorVector)
WRAP_LATTICE_CLASS(LatticePropagator)
WRAP_LATTICE_CLASS(LatticeFermion)
WRAP_LATTICE_CLASS(LatticeStaggeredFermion)
WRAP_LATTICE_CLASS(LatticeStaggeredPropagator)

/** Chroma Environment initialization
input:
  int *argc, char **argv,
  int *ndim, int *lattdims
*/
task_ret op_ChromaInitialize(void *input[4], void **output)
{

    DEF_REF(int, argc, input[0]);     //int *argc = (int *) input[0];
    DEF_PTR(char *, argv, input[1]);  //  char **argv = (char **) input[1];
    DEF_VAL(int, ndim, input[2]);     // int _ndim = *(int *) input[2];
    DEF_PTR(int, lattdims, input[3]); // int *_lattdims = (int *) input[3];
    log_printf("op_ChromaInitialize: argc=%d, ndim=%d, lattice=[%d %d %d %d]\n", argc, ndim,
               lattdims[0], lattdims[1], lattdims[2], lattdims[3]);
    Chroma::initialize(&argc, &argv);
    multi1d<int> nrow(lattdims, ndim);
    Layout::setLattSize(nrow);
    Layout::create();
    XMLFileWriter &xml_out = Chroma::getXMLOutputInstance();
    push(xml_out, "chroma"); //the follows calls need the primary node ready
    proginfo(xml_out);       // Print out basic program info
    return task_ret_done;
}
/** Chroma Environment finalization
*/
task_ret op_ChromaFinalize(void **input, void **output)
{
    log_printf("op_ChromaFinalize\n");
    XMLFileWriter &xml_out = Chroma::getXMLOutputInstance();
    pop(xml_out);
    Chroma::finalize();
    return task_ret_done;
}

/** Init Gaussian LatticeGauge
output:
  LatticeGauge *u
*/
task_ret op_InitGaussianLatticeGauge(void **input, void **output)
{
    LatticeGauge *U4 = (LatticeGauge *) output[0];
    log_printf("op_CreateGaussianLatticeGauge: %p\n", U4);
    gaussian((*U4)[0]);
    gaussian((*U4)[1]);
    gaussian((*U4)[2]);
    gaussian((*U4)[3]);
    return task_ret_done;
}

/** Init RNG
input:
  const GroupXML_t *param_rng,
  XMLFileWriter *xml_out
*/
task_ret op_InitRNG(void **input, void **output)
{
    DEF_REF(GroupXML_t, param_rng, input[0]);
    DEF_REF(XMLWriter, xml_out, input[1]);
    log_printf("op_InitRNG: %s %s %s\n", param_rng.id.c_str(), param_rng.path.c_str(),
               param_rng.xml.c_str());
    InitRNG(param_rng, xml_out);
    return task_ret_done;
}
/** Create Layout
input:
  const GroupXML_t *param_nrow;
*/
task_ret op_CreateLayout(void **input, void **output)
{
    DEF_REF(GroupXML_t, param_nrow, input[0]);
    log_printf("op_CreateLayout: %s %s\n", param_nrow.path.c_str(), param_nrow.xml.c_str());
    CreateLayout(param_nrow);
    return task_ret_done;
}

/** Load Config
input:
  const GroupXML_t *param_cfg,
  XMLFileWriter *xml_out
output:
  multi1d<LatticeColorMatrix> *u
*/
task_ret op_LoadConfig(void **input, void **output)
{
    DEF_REF(GroupXML_t, param_cfg, input[0]);
    DEF_REF(XMLWriter, xml_out, input[1]);
    DEF_REF(multi1d<LatticeColorMatrix>, u, output[0]);
    XMLBufferWriter config_xml;
    log_printf("op_LoadConfig: id=%s, path=%s\n", param_cfg.id.c_str(), param_cfg.path.c_str());
    LoadConfig(param_cfg, u, config_xml, xml_out);
    return task_ret_done;
}

/** Calculate some gauge invariant observables
input:
  const multi1d<LatticeColorMatrix> *u,
  XMLFileWriter *xml_out
*/
task_ret op_MesPlq(void **input, void **output)
{
    DEF_REF(multi1d<LatticeColorMatrix>, u, input[0]);
    DEF_REF(XMLWriter, xml_out, input[1]);
    XMLBufferWriter config_xml;
    log_printf("op_MesPlqg\n");
    MesPlq(xml_out, "Observables", u);
    return task_ret_done;
}
/** RunQuarkSourceConstruction
input:
  const GroupXML_t *xml_param,
  const multi1d<LatticeColorMatrix> *u,
  int *type
output:
  LatticePropagator|LatticeStaggeredPropagator|LatticeFermion *phi
*/
task_ret op_RunQuarkSourceConstruction(void **input, void **output)
{
    DEF_REF(GroupXML_t, xml_param, input[0]);
    DEF_REF(multi1d<LatticeColorMatrix>, u, input[1]);
    DEF_VAL(int, type, input[2]);
    log_printf("op_RunQuarkSourceConstruction: %p, %p, type= %d\n", input[0], input[1], type);
    switch (type) {
#define case_type(T)                                                                               \
    case e##T: {                                                                                   \
        DEF_REF(T, phi, output[0]);                                                                \
        RunQuarkSourceConstruction(xml_param, u, phi);                                             \
        break;                                                                                     \
    }
        case_type(LatticePropagator);
        case_type(LatticeStaggeredPropagator);
        case_type(LatticeFermion);
#undef case_type
    default:
        QDPIO::cerr << "datatype error!" << std::endl;
        QDP_abort(1);
    }

    return task_ret_done;
}

/** Get the named Gauge by id
input:
  const char *gauge_id,
  XMLFileWriter *xml_out
output:
  multi1d<LatticeColorMatrix> *u
*/
task_ret op_GetNamedGauge(void **input, void **output)
{
    std::string gauge_id = AS_CPTR(char, input[0]);
    DEF_REF(XMLWriter, xml_out, input[1]);
    DEF_REF(multi1d<LatticeColorMatrix>, u, output[0]);
    log_printf("op_GetNamedGauge: id=%s\n", gauge_id.c_str());
    GetNamedGauge(gauge_id, u, xml_out);
    return task_ret_ok;
}

/** GetNamedLatticePropagator
input:
  const char *prop_id,
  XMLFileWriter *xml_out
output:
  LatticePropagator *quark_propagator,
  int *j_decay
*/
task_ret op_GetNamedLatticePropagator(void **input, void **output)
{
    std::string prop_id = AS_CPTR(char, input[0]);
    DEF_REF(XMLWriter, xml_out, input[1]);
    DEF_REF(LatticePropagator, quark_propagator, output[0]);
    DEF_REF(int, j_decay, output[1]);
    log_printf("op_GetNamedLatticePropagator: id=%s\n", prop_id.c_str());
    GetNamedLatticePropagator(prop_id, quark_propagator, j_decay, xml_out);
    return task_ret_ok;
}

/** SetNamedLatticePropagator
input:
  const char *prop_id,
  const LatticePropagator *quark_propagator,
  int *j_decay,
  XMLFileWriter *xml_out
*/
task_ret op_SetNamedLatticePropagator(void **input, void **output)
{
    std::string prop_id = AS_CPTR(char, input[0]);
    DEF_REF(LatticePropagator, quark_propagator, input[1]);
    DEF_REF(int, j_decay, input[2]);
    DEF_REF(XMLWriter, xml_out, input[3]);
    log_printf("op_SetNamedLatticePropagator: id=%s\n", prop_id.c_str());
    SetNamedLatticePropagator(prop_id, quark_propagator, j_decay, xml_out);
    return task_ret_ok;
}

/** Displacement
input:
  multi1d<LatticeColorMatrix> *u,
  LatticeColorVector|LatticePropagator|LatticeFermion|LatticeStaggeredFermion|LatticeStaggeredPropagator *phi,
  int *length, int *dir, const int *type;
*/
task_ret op_Displacement(void **input, void **output)
{
    DEF_REF(multi1d<LatticeColorMatrix>, U4, input[0]);
    DEF_REF(int, length, input[2]);
    DEF_REF(int, dir, input[3]);
    DEF_REF(int, type, input[4]);
    log_printf("op_Displacement: %p, %p %d %d %d\n", input[0], input[1], length, dir, type);
    switch (type) {
#define case_type(T)                                                                               \
    case e##T: {                                                                                   \
        DEF_REF(T, phi, input[1]);                                                                 \
        displacement(U4, phi, length, dir);                                                        \
        break;                                                                                     \
    }
        case_type(LatticeColorVector);
        case_type(LatticePropagator);
        case_type(LatticeFermion);
        case_type(LatticeStaggeredFermion);
        case_type(LatticeStaggeredPropagator);
#undef case_type
    default:
        QDPIO::cerr << "datatype error!" << std::endl;
        QDP_abort(1);
    }
    return task_ret_ok;
}

/** RunQuarkDisplacement
input:
  const GroupXML_t *xml_param,
  multi1d<LatticeColorMatrix> *u,
  int *isign,
  LatticeColorVector|LatticePropagator|LatticeFermion|LatticeStaggeredPropagator *phi,
  int *type;
output:
  LatticeColorVector|LatticePropagator|LatticeFermion|LatticeStaggeredPropagator *phi,
*/
task_ret op_RunQuarkDisplacement(void *input[6], void **output)
{
    DEF_REF(GroupXML_t, xml_param, input[0]);
    DEF_REF(multi1d<LatticeColorMatrix>, u, input[1]);
    DEF_REF(int, isign, input[2]);
    enum PlusMinus eisign = (PlusMinus) isign;
    DEF_REF(int, type, input[4]);
    log_printf("op_RunQuarkDisplacement: %s %s %s, %p\n", xml_param.id.c_str(),
               xml_param.path.c_str(), xml_param.xml.c_str(), input[1]);
    switch (type) {
#define case_type(T)                                                                               \
    case e##T: {                                                                                   \
        DEF_REF(T, phi, input[3]);                                                                 \
        RunQuarkDisplacement(xml_param, u, eisign, phi);                                           \
        break;                                                                                     \
    }
        case_type(LatticeColorVector);
        case_type(LatticePropagator);
        case_type(LatticeFermion);
        case_type(LatticeStaggeredPropagator);
#undef case_type
    default:
        QDPIO::cerr << "datatype error!" << std::endl;
        QDP_abort(1);
    }
    return task_ret_ok;
}
/** RunLinkSmearing
input:
  const GroupXML_t *xml_param,
  multi1d<LatticeColorMatrix> *u
output:
  multi1d<LatticeColorMatrix> *u
*/
task_ret op_RunLinkSmearing(void **input, void **output)
{
    DEF_REF(GroupXML_t, xml_param, input[0]);
    DEF_REF(multi1d<LatticeColorMatrix>, u, input[1]);
    log_printf("op_RunLinkSmearing: %s %s %s, %p\n", xml_param.id.c_str(), xml_param.path.c_str(),
               xml_param.xml.c_str(), input[1]);
    if (input[1] != output[0])
        log_printf("Error! op_RunLinkSmearing assumes input[1] == output[0]\n");
    RunLinkSmearing(xml_param, u);
    return task_ret_ok;
}
/** RunSinkSmearing
input:
  const GroupXML_t *xml_param,
  const multi1d<LatticeColorMatrix> *u,
  LatticePropagator|LatticeStaggeredPropagator|LatticeFermion *phi,
  int *type;
*/
task_ret op_RunSinkSmearing(void **input, void **output)
{
    DEF_REF(GroupXML_t, xml_param, input[0]);
    DEF_REF(multi1d<LatticeColorMatrix>, u, input[1]);
    DEF_REF(int, type, input[3]);
    log_printf("op_RunSinkSmearing: %s %s %s, type=%d\n", xml_param.id.c_str(),
               xml_param.path.c_str(), xml_param.xml.c_str(), type);
    switch (type) {
#define case_type(T)                                                                               \
    case e##T: {                                                                                   \
        DEF_REF(T, phi, input[2]);                                                                 \
        RunSinkSmearing(xml_param, u, phi);                                                        \
        break;                                                                                     \
    }
        case_type(LatticePropagator);
        case_type(LatticeFermion);
        case_type(LatticeStaggeredPropagator);
#undef case_type
    default:
        QDPIO::cerr << "datatype error!" << std::endl;
        QDP_abort(1);
    }
    return task_ret_ok;
}
/** RunQuarkSmearing
input:
  const GroupXML_t *xml_param,
  const multi1d<LatticeColorMatrix> *u,
  LatticePropagator|LatticeStaggeredPropagator|LatticeFermion|LatticeColorVector *phi,
  int *type
*/
task_ret op_RunQuarkSmearing(void **input, void **output)
{
    DEF_REF(GroupXML_t, xml_param, input[0]);
    DEF_REF(multi1d<LatticeColorMatrix>, u, input[1]);
    DEF_REF(int, type, input[3]);
    log_printf("op_RunQuarkSmearing: %s %s %s, %p\n", xml_param.id.c_str(), xml_param.path.c_str(),
               xml_param.xml.c_str(), input[1]);
    switch (type) {
#define case_type(T)                                                                               \
    case e##T: {                                                                                   \
        DEF_REF(T, phi, input[2]);                                                                 \
        RunQuarkSmearing(xml_param, u, phi);                                                       \
        break;                                                                                     \
    }
        case_type(LatticeColorVector);
        case_type(LatticePropagator);
        case_type(LatticeFermion);
        case_type(LatticeStaggeredPropagator);
#undef case_type
    default:
        QDPIO::cerr << "datatype error!" << std::endl;
        QDP_abort(1);
    }
    return task_ret_ok;
}

/** DoInlinePropagatorInverter
input:
  const multi1d<LatticeColorMatrix> *u,
  const LatticePropagator|LatticeStaggeredPropagator *quark_prop_source,
  int * type, //eLatticePropagator=wislon, eLatticeStaggeredPropagator=stagger
  const ChromaProp_t *params_prop, const int *t0, const int *j_decay,
  XMLWriter *xml_out
output:
  LatticePropagator|LatticeStaggeredPropagator *quark_propagator
*/
task_ret op_DoInlinePropagatorInverter(void **input, void **output)
{
    DEF_REF(multi1d<LatticeColorMatrix>, u, input[0]);
    DEF_REF(int, type, input[2]);
    DEF_REF(ChromaProp_t, params_prop, input[3]);
    DEF_VAL(int, t0, input[4]);
    DEF_VAL(int, j_decay, input[5]);
    DEF_REF(XMLWriter, xml_out, input[6]);

    log_printf("op_DoInlinePropagatorInverter: type=%d, fermact=%s invert=%s j_decay=%d, %p\n",
               type, params_prop.fermact.id.c_str(), params_prop.invParam.id.c_str(), j_decay,
               input[1]);

    switch (type) {
#define case_type(T)                                                                               \
    case e##T: {                                                                                   \
        DEF_REF(T, quark_prop_source, input[1]);                                                   \
        DEF_REF(T, quark_propagator, output[0]);                                                   \
        DoInlinePropagatorInverter(u, quark_prop_source, params_prop, t0, j_decay,                 \
                                   quark_propagator, xml_out);                                     \
        break;                                                                                     \
    }
        case_type(LatticePropagator);
        case_type(LatticeStaggeredPropagator);
#undef case_type
    default:
        QDPIO::cerr << "datatype error!" << std::endl;
        QDP_abort(1);
    }
    return task_ret_ok;
}
/** Compute Mesons2-pt function
input:
  const LatticePropagator *quark_prop_1
  const LatticePropagator *quark_prop_2,
  const SftMom *phases, int *t0, int *gamma_value, 
  XMLFileWriter *xml_out
output:
  multi2d<DComplex> *mesprop_vec
*/
task_ret op_ComputeMesons2(void **input, void **output)
{
    DEF_REF(LatticePropagator, quark_prop_1, input[0]);
    DEF_REF(LatticePropagator, quark_prop_2, input[1]);
    DEF_REF(SftMom, phases, input[2]);
    DEF_VAL(int, t0, input[3]);
    DEF_VAL(int, gamma_value, input[4]);
    DEF_REF(XMLWriter, xml_out, input[5]);

    DEF_REF(multi2d<DComplex>, mesprop_vec, output[0]);
    log_printf("op_ComputeMesons2: t0=%d, gamma_value=%d\n", t0, gamma_value);
    ComputeMesons2(quark_prop_1, quark_prop_2, phases, t0, gamma_value, mesprop_vec, xml_out);
    return task_ret_ok;
}
/** Construct current correlators
input:
  const multi1d<LatticeColorMatrix> *u,
  const LatticePropagator *quark_prop_1,
  const LatticePropagator *quark_prop_2,
  const SftMom *phases, 
  int *t0, int *no_vec_cur,
  XMLWriter &xml_out
output:
  multi2d<Real> *vector_current,
  multi2d<Real> *axial_current
*/
task_ret op_ComputeCurcor2(void **input, void **output)
{
    DEF_REF(multi1d<LatticeColorMatrix>, u, input[0]);
    DEF_REF(LatticePropagator, quark_prop_1, input[1]);
    DEF_REF(LatticePropagator, quark_prop_2, input[2]);
    DEF_REF(SftMom, phases, input[3]);
    DEF_VAL(int, t0, input[4]);
    DEF_VAL(int, no_vec_cur, input[5]);
    DEF_REF(XMLWriter, xml_out, input[6]);
    DEF_REF(multi2d<Real>, vector_current, output[0]);
    DEF_REF(multi2d<Real>, axial_current, output[1]);

    log_printf("op_ComputeCurcor2: t0=%d, no_vec_cur=%d\n", t0, no_vec_cur);
    ComputeCurcor2(u, quark_prop_1, quark_prop_2, phases, t0, no_vec_cur, vector_current,
                   axial_current, xml_out);

    return task_ret_ok;
}

/** Compute Heavy-light baryon 2-pt functions
input:
  const LatticePropagator *propagator_1
  const LatticePropagator *propagator_2,
  const SftMom *phases, int *t0,
  int *bc_spec, bool *time_rev,
  int *baryons,
  XMLFileWriter *xml_out
output:
  multi2d<DComplex> *barprop_vec
*/
task_ret op_ComputeBarhqlq(void **input, void **output)
{
    DEF_REF(LatticePropagator, propagator_1, input[0]);
    DEF_REF(LatticePropagator, propagator_2, input[1]);
    DEF_REF(SftMom, phases, input[2]);
    DEF_VAL(int, t0, input[3]);
    DEF_VAL(int, bc_spec, input[4]);
    DEF_VAL(bool, time_rev, input[5]);
    DEF_VAL(int, baryons, input[6]);
    DEF_REF(XMLWriter, xml_out, input[7]);

    DEF_REF(multi2d<DComplex>, barprop_vec, output[0]);
    log_printf("op_ComputeBarhqlq: t0=%d, bc_spec=%d, time_rev=%d, baryons=%d\n", t0, bc_spec,
               time_rev, baryons);

    ComputeBarhqlq(propagator_1, propagator_2, phases, t0, bc_spec, time_rev, baryons, barprop_vec,
                   xml_out);

    return task_ret_ok;
}

/** RunInlineMeasurement
input:
  const GroupXML_t *xml_param,
  unsigned long *update_no,
  XMLFileWriter *xml_out
*/
task_ret op_RunInlineMeasurement(void **input, void **output)
{
    DEF_REF(GroupXML_t, xml_param, input[0]);
    DEF_VAL(unsigned long, update_no, input[1]);
    DEF_REF(XMLWriter, xml_out, input[2]);
    log_printf("op_RunInlineMeasurement: %s %s %s, %lu\n", xml_param.id.c_str(),
               xml_param.path.c_str(), xml_param.xml.c_str(), update_no);
    RunInlineMeasurement(xml_param, update_no, xml_out);
    return task_ret_done;
}
/** WriteGauge
input:
  const multi1d<LatticeColorMatrix> *u,
  const char *file
*/
task_ret op_WriteGauge(void **input, void **output)
{
    DEF_REF(multi1d<LatticeColorMatrix>, u, input[0]);
    const std::string file = AS_CPTR(char, input[1]);
    log_printf("op_WriteGauge: %p, %s\n", input[0], file.c_str());
    WriteGauge(u, file);
    return task_ret_ok;
}
/** ReadGauge
input:
  const char *file
output:
  multi1d<LatticeColorMatrix> *u
*/
task_ret op_ReadGauge(void **input, void **output)
{
    const std::string file = AS_CPTR(char, input[0]);
    DEF_REF(multi1d<LatticeColorMatrix>, u, output[0]);
    log_printf("op_ReadGauge: %s\n", file.c_str());
    ReadGauge(file, u);
    return task_ret_ok;
}
/** WriteLatticePropagator
input:
  const LatticePropagator *quark_propagator,
  const int *j_decay,
  const char *file
*/
task_ret op_WriteLatticePropagator(void **input, void **output)
{
    DEF_REF(LatticePropagator, quark_propagator, input[0]);
    DEF_VAL(int, j_decay, input[1]);
    const std::string file = AS_CPTR(char, input[2]);
    log_printf("op_WriteLatticePropagator: %p, %d, %s\n", input[0], j_decay, file.c_str());
    WriteLatticePropagator(quark_propagator, j_decay, file);
    return task_ret_ok;
}
/** ReadLatticePropagator
input:
  const char *file
output:
  LatticePropagator *quark_propagator,
  int *j_decay
*/
task_ret op_ReadLatticePropagator(void **input, void **output)
{
    const std::string file = AS_CPTR(char, input[0]);
    DEF_REF(LatticePropagator, quark_propagator, output[0]);
    DEF_REF(int, j_decay, output[1]);
    log_printf("op_WriteLatticePropagator: %p, %d, %s\n", input[0], j_decay, file.c_str());
    ReadLatticePropagator(file, quark_propagator, j_decay);
    return task_ret_ok;
}
/** WriteLatticeFermion
input:
  const LatticeFermion *fermion,
  const char *file
*/
task_ret op_WriteLatticeFermion(void **input, void **output)
{
    DEF_REF(LatticeFermion, fermion, input[0]);
    const std::string file = AS_CPTR(char, input[1]);
    log_printf("op_WriteLatticeFermion: %p, %s\n", input[0], file.c_str());
    WriteLatticeFermion(fermion, file);
    return task_ret_ok;
}
/** ReadLatticeFermion
input:
  const char *file
output:
  LatticeFermion *fermion
*/
task_ret op_ReadLatticeFermion(void **input, void **output)
{
    const std::string file = AS_CPTR(char, input[0]);
    DEF_REF(LatticeFermion, fermion, output[0]);
    log_printf("op_ReadLatticeFermion: %s\n", file.c_str());
    ReadLatticeFermion(file, fermion);
    return task_ret_ok;
}
/** WriteComplexMatrix
input:
  const multi2d<ComplexF|ComplexD|Rreal> *data,
  const int *type,
  const char *file
*/
task_ret op_WriteMulti2d(void **input, void **output)
{
    DEF_VAL(int, type, input[1]);
    const std::string file = AS_CPTR(char, input[2]);
    log_printf("op_WriteMulti2d: type=%d, file=%s\n", type, file.c_str());
    switch (type) {
#define case_type(T)                                                                               \
    case e##T: {                                                                                   \
        DEF_REF(multi2d<T>, data, input[0]);                                                       \
        WriteMulti2d(data, file);                                                                  \
        break;                                                                                     \
    }
        case_type(DComplex);
        // case_type(Complex);
        // case_type(Double);
        case_type(Real);
#undef case_type
    default:
        QDPIO::cerr << "datatype error!" << std::endl;
        QDP_abort(1);
    }
    return task_ret_done;
}

/** wrap global XMLOutputInstance to a DDQ obj */
obj obj_import_XMLOutputInstance(enum_obj_prop prop)
{
    log_printf("obj_import_XMLOutputInstance: %d\n", prop);
    XMLFileWriter &xml_out = Chroma::getXMLOutputInstance();
    return obj_import(&xml_out, nullptr, prop);
}

/** new a GroupXML_t obj */
obj obj_import_GroupXML_t(const char *id, const char *path, const char *xml, enum_obj_prop prop)
{
    log_printf("obj_import_GroupXML_t: %d\n", prop);
    GroupXML_t *gxml = new GroupXML_t;
    gxml->id = id;
    gxml->path = path;
    gxml->xml = xml;
    return obj_import(gxml, delete_GroupXML_t, prop);
}

/** new a ChromaProp_t obj */
obj obj_import_ChromaProp_t(const char *fermact_id, const char *fermact_path,
                            const char *fermact_xml, const char *invParam_id,
                            const char *invParam_path, const char *invParam_xml, int quarkSpinType,
                            int obsvP, enum_obj_prop prop)
{
    log_printf("obj_import_ChromaProp_t: %d\n", prop);
    ChromaProp_t *cprop = new ChromaProp_t;
    cprop->fermact.id = fermact_id;
    cprop->fermact.path = fermact_path;
    cprop->fermact.xml = fermact_xml;
    cprop->invParam.id = invParam_id;
    cprop->invParam.path = invParam_path;
    cprop->invParam.xml = invParam_xml;
    cprop->quarkSpinType = (QuarkSpinType) quarkSpinType;
    cprop->obsvP = (bool) obsvP;
    return obj_import(cprop, delete_ChromaProp_t, prop);
}
/** new a SftMom obj */
obj obj_import_SftMom(int mom2_max, int x0, int y0, int z0, int t0, int avg_equiv_mom, int j_decay,
                      enum_obj_prop prop)
{
    log_printf(
        "obj_import_SftMom: mom2_max=%d, t_srce=[%d %d %d %d], j_decay=%d, avg_equiv_mom=%d\n",
        mom2_max, x0, y0, z0, t0, j_decay, avg_equiv_mom);
    multi1d<int> t_srce(4);
    t_srce[0] = x0;
    t_srce[1] = y0;
    t_srce[2] = z0;
    t_srce[3] = t0;
    //assert(j_decay==3);
    //See also SftMomParams_t
    SftMom *phases = new SftMom(mom2_max, t_srce, (bool) avg_equiv_mom, j_decay);
    return obj_import(phases, delete_SftMom, prop);
}

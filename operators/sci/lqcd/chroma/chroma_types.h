
#ifndef CHROMA_TYPES_H
#define CHROMA_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

    enum eLatticeType {
        eLatticeColorVector,
        eLatticePropagator,
        eLatticeFermion,
        eLatticeStaggeredFermion,
        eLatticeStaggeredPropagator,
        eLatticeGauge
    };
    enum ePlusMinus { ePLUS = 1, eMINUS = -1 };
    enum eComplexType { eComplex, eDComplex, eComplexF, eComplexD, eReal, eDouble };

#ifdef __cplusplus
}
#endif

#endif
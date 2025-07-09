
#ifndef CHROMA_TYPES_H
#define CHROMA_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

    enum eLatticeType {
        eLatticeColorVector = 0,
        eLatticePropagator = 1,
        eLatticeFermion = 2,
        eLatticeStaggeredFermion = 3,
        eLatticeStaggeredPropagator = 4,
        eLatticeGauge = 5
    };
    enum ePlusMinus { ePLUS = 1, eMINUS = -1 };
    enum eComplexType { eComplex = 0, eDComplex = 1, eComplexF = 2, eComplexD = 3, eReal = 4, eDouble = 5 };

#ifdef __cplusplus
}
#endif

#endif

#ifndef CHROMA_OPERTATORS_H
#define CHROMA_OPERTATORS_H

#include "chroma.h"
#include "chroma_types.h"

typedef multi1d<LatticeColorMatrix> LatticeGauge;

void InitRNG(const GroupXML_t &param_rng, XMLWriter &xml_out);

void CreateLayout(const GroupXML_t &param_nrow);

void StartupConfig(const GroupXML_t &input_cfg, multi1d<LatticeColorMatrix> &u,
                   XMLReader &gauge_file_xml, XMLReader &gauge_xml);
void LoadConfig(const GroupXML_t &param_cfg, multi1d<LatticeColorMatrix> &u,
                XMLBufferWriter &config_xml, XMLWriter &xml_out);

void GetNamedGauge(const std::string &gauge_id, multi1d<LatticeColorMatrix> &u, XMLWriter &xml_out);
void GetNamedLatticePropagator(const std::string &prop_id, LatticePropagator &quark_propagator,
                               int &j_decay, XMLWriter &xml_out);
std::string SetNamedLatticePropagator(const std::string &prop_id,
                                      const LatticePropagator &quark_propagator, int j_decay,
                                      XMLWriter &xml_out);
// for debug
void CheckMesPlsq(const multi1d<LatticeColorMatrix> &u, const std::string &name,
                  XMLWriter &xml_out);
void CheckSanity(const LatticePropagator &quark_propagator, const std::string &prefix,
                 XMLWriter &xml_out);
void CheckSanity(const LatticeStaggeredPropagator &quark_propagator, const std::string &prefix,
                 XMLWriter &xml_out);

/* chroma/lib/io/gauge_io.h
void readGauge(XMLReader &file_xml, XMLReader &record_xml, multi1d<LatticeColorMatrix> &u,
               const std::string &file, QDP_serialparallel_t serpar);
void writeGauge(XMLBufferWriter &file_xml, XMLBufferWriter &record_xml,
                const multi1d<LatticeColorMatrix> &u, const std::string &file, QDP_volfmt_t volfmt,
                QDP_serialparallel_t serpar);
*/
void WriteGauge(const multi1d<LatticeColorMatrix> &u, const std::string &file);
void ReadGauge(const std::string &file, multi1d<LatticeColorMatrix> &u);

/* chroma/lib/io/qprop_io.h
//! Write/read a Chroma propagator
void writeQprop(XMLBufferWriter &file_xml, XMLBufferWriter &record_xml,
                const LatticePropagator &quark_prop, const std::string &file, QDP_volfmt_t volfmt,
                QDP_serialparallel_t serpar);
void readQprop(XMLReader &file_xml, XMLReader &record_xml, LatticePropagator &quark_prop,
               const std::string &file, QDP_serialparallel_t serpar);
*/
void WriteLatticePropagator(const LatticePropagator &quark_propagator, int j_decay,
                            const std::string &file);
void ReadLatticePropagator(const std::string &file, LatticePropagator &quark_propagator,
                           int &j_decay);

/* Write/read a Chroma Fermion Field (eg prop_component)
void writeFermion(XMLBufferWriter &file_xml, XMLBufferWriter &record_xml,
                  const LatticeFermion &fermion, const std::string &file, QDP_volfmt_t volfmt,
                  QDP_serialparallel_t serpar);
void readFermion(XMLReader &file_xml, XMLReader &record_xml, LatticeFermion &fermion,
                 const std::string &file, QDP_serialparallel_t serpar);
*/
void WriteLatticeFermion(const LatticeFermion &fermion, const std::string &file);
void ReadLatticeFermion(const std::string &file, LatticeFermion &fermion);

void WriteMulti2d(const multi2d<DComplex> &data, const std::string &file);
void WriteMulti2d(const multi2d<Real> &data, const std::string &file);

////////////////
// ref: <elem>/<Param>/<Source>
void RunQuarkSourceConstruction(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                                LatticePropagator &prop);
void RunQuarkSourceConstruction(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                                LatticeStaggeredPropagator &prop);
void RunQuarkSourceConstruction(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                                LatticeFermion &fermion);
////////////////
void RunQuarkDisplacement(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                          enum PlusMinus isign, LatticePropagator &quark);
void RunQuarkDisplacement(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                          enum PlusMinus isign, LatticeStaggeredPropagator &quark);
void RunQuarkDisplacement(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                          enum PlusMinus isign, LatticeFermion &quark);
void RunQuarkDisplacement(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                          enum PlusMinus isign, LatticeColorVector &quark);
////////////////
void RunLinkSmearing(const GroupXML_t &xml_param, multi1d<LatticeColorMatrix> &u);

////////////////
void RunSinkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                     LatticePropagator &quark_sink);
void RunSinkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                     LatticeStaggeredPropagator &quark_sink);
void RunSinkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                     LatticeFermion &quark_sink);
////////////////
void RunQuarkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                      LatticePropagator &quark);
void RunQuarkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                      LatticeStaggeredPropagator &quark);
void RunQuarkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                      LatticeFermion &quark);
void RunQuarkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                      LatticeColorVector &quark);
////////////////
void DoInlinePropagatorInverter(const multi1d<LatticeColorMatrix> &u,
                                const LatticePropagator &quark_prop_source,
                                const ChromaProp_t &params_prop, int t0, int j_decay,
                                LatticePropagator &quark_propagator, XMLWriter &xml_out);
void DoInlinePropagatorInverter(const multi1d<LatticeColorMatrix> &u,
                                const LatticeStaggeredPropagator &quark_prop_source,
                                const ChromaProp_t &params_prop, int t0, int j_decay,
                                LatticeStaggeredPropagator &quark_propagator, XMLWriter &xml_out);
// Compute hadron spectrum
void ComputeMesons2(const LatticePropagator &quark_prop_1, const LatticePropagator &quark_prop_2,
                    const SftMom &phases, int t0, int gamma_value, multi2d<DComplex> &mesprop,
                    XMLWriter &xml_out);
void ComputeCurcor2(const multi1d<LatticeColorMatrix> &u, const LatticePropagator &quark_prop_1,
                    const LatticePropagator &quark_prop_2, const SftMom &phases, int t0,
                    int no_vec_cur, multi2d<Real> &vector_current, multi2d<Real> &axial_current,
                    XMLWriter &xml_out);
void ComputeBarhqlq(const LatticePropagator &propagator_1, const LatticePropagator &propagator_2,
                    const SftMom &phases, int t0, int bc_spec, bool time_rev, int baryon_value,
                    multi2d<DComplex> &barprop, XMLWriter &xml_out);

// ref <elem>
void RunInlineMeasurement(const GroupXML_t &xml_param, unsigned long update_no, XMLWriter &xml_out);

#endif

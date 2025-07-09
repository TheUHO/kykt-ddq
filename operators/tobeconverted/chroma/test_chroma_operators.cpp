#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "chroma_operators.h"

using namespace std;
#define OP_BYSTEP 1

void load_default_cfg(multi1d<LatticeColorMatrix> &u, XMLWriter &xml_out)
{
    /*
    multi1d<LatticeColorMatrix> u(4);
    gaussian(u[0]);
    gaussian(u[1]);
    gaussian(u[2]);
    gaussian(u[3]);
    */
    GroupXML_t param_cfg;
    param_cfg.id = "Cfg";
    param_cfg.path = "/Cfg";
    param_cfg.xml = "<Cfg>"
                    " <cfg_type>WEAK_FIELD</cfg_type>"
                    " <cfg_file>dummy</cfg_file>"
                    "</Cfg>";

    XMLBufferWriter config_xml;
    LoadConfig(param_cfg, u, config_xml, xml_out);

    // Calculate some gauge invariant observables
    MesPlq(xml_out, "Observables", u);

    // Reset and set the default gauge field
    InlineDefaultGaugeField::reset();
    InlineDefaultGaugeField::set(u, config_xml);
}
void make_default_source(LatticePropagator &quark_source, int j_decay, int t0, XMLWriter &xml_out)
{
    //for each <elem>: AbsInlineMeasurement

    GroupXML_t params_xml;
    params_xml.id = "MAKE_SOURCE";
    params_xml.path = "/elem";
    params_xml.xml = "<elem>"
                     "<Name>MAKE_SOURCE</Name>"
                     " <Frequency>1</Frequency>"
                     " <Param>"
                     "  <version>6</version>"
                     "  <Source>"
                     "   <version>2</version>"
                     "   <SourceType>SHELL_SOURCE</SourceType>"
                     "   <j_decay>" +
                     std::to_string(j_decay) +
                     "</j_decay>"
                     "   <t_srce>0 0 0 " +
                     std::to_string(t0) +
                     "</t_srce>"
                     "   <SmearingParam>"
                     "     <wvf_kind>GAUGE_INV_GAUSSIAN</wvf_kind>"
                     "     <wvf_param>2.0</wvf_param>"
                     "     <wvfIntPar>5</wvfIntPar>"
                     "     <no_smear_dir>3</no_smear_dir>"
                     "   </SmearingParam>"
                     "   <Displacement>"
                     "     <version>1</version>"
                     "     <DisplacementType>NONE</DisplacementType>"
                     "   </Displacement>"
                     "   <LinkSmearing>"
                     "     <LinkSmearingType>APE_SMEAR</LinkSmearingType>"
                     "     <link_smear_fact>2.5</link_smear_fact>"
                     "     <link_smear_num>1</link_smear_num>"
                     "     <no_smear_dir>3</no_smear_dir>"
                     "   </LinkSmearing>"
                     "  </Source>"
                     "  </Param>"
                     "   <NamedObject>"
                     "    <gauge_id>default_gauge_field</gauge_id>"
                     "   <source_id>sh_source_1</source_id>"
                     "  </NamedObject>"
                     "</elem>";
    RunInlineMeasurement(params_xml, 0, xml_out);

    // gaussian(quark_source);
    int j_decay2 = 0;
    GetNamedLatticePropagator("sh_source_1", quark_source, j_decay2, xml_out);
    assert(j_decay2 == j_decay);
}
void make_default_source_bystep(LatticePropagator &quark_source, int j_decay, int t0,
                                XMLWriter &xml_out)
{
    multi1d<LatticeColorMatrix> u;
    XMLBufferWriter file_xml;
    XMLBufferWriter record_xml;
    InlineDefaultGaugeField::get(u, file_xml, record_xml);

    GroupXML_t params_xml;
    params_xml.id = "SHELL_SOURCE";
    params_xml.path = "/Source";
    params_xml.xml = "<Source>"
                     "   <version>2</version>"
                     "   <SourceType>SHELL_SOURCE</SourceType>"
                     "   <j_decay>" +
                     std::to_string(j_decay) +
                     "</j_decay>"
                     "   <t_srce>0 0 0 " +
                     std::to_string(t0) +
                     "</t_srce>"
                     "   <SmearingParam>"
                     "     <wvf_kind>GAUGE_INV_GAUSSIAN</wvf_kind>"
                     "     <wvf_param>2.0</wvf_param>"
                     "     <wvfIntPar>5</wvfIntPar>"
                     "     <no_smear_dir>3</no_smear_dir>"
                     "   </SmearingParam>"
                     "   <Displacement>"
                     "     <version>1</version>"
                     "     <DisplacementType>NONE</DisplacementType>"
                     "   </Displacement>"
                     "   <LinkSmearing>"
                     "     <LinkSmearingType>APE_SMEAR</LinkSmearingType>"
                     "     <link_smear_fact>2.5</link_smear_fact>"
                     "     <link_smear_num>1</link_smear_num>"
                     "     <no_smear_dir>3</no_smear_dir>"
                     "   </LinkSmearing>"
                     "</Source>";

    RunQuarkSourceConstruction(params_xml, u, quark_source);
}
void propagator_inverter_bystep(const multi1d<LatticeColorMatrix> &u,
                                LatticePropagator &quark_source,
                                LatticePropagator &quark_propagator, const multi1d<int> &boundary,
                                int j_decay, int t0, XMLWriter &xml_out)
{
    std::ostringstream bc_str;
    int ns = boundary.size();
    for (int i = 0; i < ns; i++) {
        bc_str << boundary[i];
        if (i < ns - 1)
            bc_str << " ";
    }
    /*!< fermion action */
    ChromaProp_t params_prop;
    params_prop.quarkSpinType = QUARK_SPIN_TYPE_FULL;
    params_prop.obsvP = false; /*!< measure any observables (like Z_V, or mresP) on 5D prop */
    params_prop.fermact.id = "WILSON";
    params_prop.fermact.path = "/FermionAction";
    params_prop.fermact.xml = "<FermionAction>"
                              " <FermAct>WILSON</FermAct> "
                              " <Kappa>0.11</Kappa>"
                              " <AnisoParam>"
                              "  <anisoP>false</anisoP>"
                              "  <t_dir>3</t_dir>"
                              "  <xi_0>1.0</xi_0>"
                              "  <nu>1.0</nu>"
                              " </AnisoParam>"
                              " <FermionBC>"
                              "  <FermBC>SIMPLE_FERMBC</FermBC>"
                              "  <boundary>" +
                              bc_str.str() +
                              "</boundary>"
                              " </FermionBC>"
                              "</FermionAction>";
    /*!< Inverter parameters */
    params_prop.invParam.id = "CG_INVERTER";
    params_prop.invParam.path = "/InvertParam";
    params_prop.invParam.xml = "<InvertParam>"
                               " <invType>CG_INVERTER</invType>"
                               " <RsdCG>1.0e-12</RsdCG>"
                               " <MaxCG>100</MaxCG>"
                               "</InvertParam>";

    DoInlinePropagatorInverter(u, quark_source, params_prop, t0, j_decay, quark_propagator,
                               xml_out);
}

void sink_smearing(const multi1d<LatticeColorMatrix> &u, LatticePropagator &quark_propagator,
                   int j_decay)
{
    GroupXML_t params_xml;
    params_xml.id = "SHELL_SINK";
    params_xml.path = "/Sink";
    params_xml.xml = "<Sink>"
                     " <version>2</version>"
                     " <SinkType>SHELL_SINK</SinkType>"
                     " <j_decay>" +
                     std::to_string(j_decay) +
                     "</j_decay>"
                     " <Displacement>"
                     "   <version>1</version>"
                     "   <DisplacementType>NONE</DisplacementType>"
                     " </Displacement>"

                     " <SmearingParam>"
                     "   <wvf_kind>GAUGE_INV_GAUSSIAN</wvf_kind>"
                     "   <wvf_param>2.0</wvf_param>"
                     "   <wvfIntPar>5</wvfIntPar>"
                     "   <no_smear_dir>3</no_smear_dir>"
                     " </SmearingParam>"

                     " <LinkSmearing>"
                     "   <LinkSmearingType>APE_SMEAR</LinkSmearingType>"
                     "   <link_smear_fact>2.5</link_smear_fact>"
                     "   <link_smear_num>1</link_smear_num>"
                     "   <no_smear_dir>3</no_smear_dir>"
                     " </LinkSmearing>"
                     "</Sink>";
    RunSinkSmearing(params_xml, u, quark_propagator);
}
void sink_smearing_bystep(const multi1d<LatticeColorMatrix> &u, LatticePropagator &quark_propagator,
                          int j_decay)
{
    multi1d<LatticeColorMatrix> u_smr(u);

    GroupXML_t params_xml;

    QDPIO::cout << "=== RunLinkSmearing APE_SMEAR ===" << std::endl;
    params_xml.id = "APE_SMEAR";
    params_xml.path = "/Param";
    params_xml.xml = "<Param>"
                     " <LinkSmearingType>APE_SMEAR</LinkSmearingType>"
                     " <link_smear_fact>2.5</link_smear_fact>"
                     " <link_smear_num>1</link_smear_num>"
                     " <no_smear_dir>3</no_smear_dir>"
                     "</Param>";
    RunLinkSmearing(params_xml, u_smr);

    QDPIO::cout << "=== QuarkDisplacement ===" << std::endl;
    params_xml.id = "NONE";
    params_xml.path = "/Displacement";
    params_xml.xml = "<Displacement>"
                     "  <version>1</version>"
                     "  <DisplacementType>NONE</DisplacementType>"
                     "</Displacement>";
    RunQuarkDisplacement(params_xml, u_smr, PLUS, quark_propagator);

    QDPIO::cout << "=== RunSinkSmearing GAUGE_INV_GAUSSIAN ===" << std::endl;
    params_xml.id = "GAUGE_INV_GAUSSIAN";
    params_xml.path = "/SmearingParam";
    params_xml.xml = "<SmearingParam>"
                     "  <wvf_kind>GAUGE_INV_GAUSSIAN</wvf_kind>"
                     "  <wvf_param>2.0</wvf_param>"
                     "  <wvfIntPar>5</wvfIntPar>"
                     "  <no_smear_dir>3</no_smear_dir>"
                     "</SmearingParam>";
    RunQuarkSmearing(params_xml, u_smr, quark_propagator);
}
void gauge_displacement(const multi1d<LatticeColorMatrix> &u, LatticePropagator &phi2)
{
    QDPIO::cout << " phi2" << phi2.elem(2).elem(1, 1).elem(1, 2) << endl;
    int length = 1;
    int dir = 0;
    Chroma::displacement(u, phi2, length, dir);
    QDPIO::cout << "-> phi2" << phi2.elem(2).elem(1, 1).elem(1, 2) << endl;
}

void write_prop(const LatticePropagator &quark_propagator, int j_decay, XMLWriter &xml_out)
{
    QDPIO::cout << "=== Write/ReadLatticePropagator quark_propagator ===" << std::endl;
    WriteLatticePropagator(quark_propagator, j_decay, "test.lime");
    LatticePropagator quark_propagator2;
    int j_decay2 = 0;
    ReadLatticePropagator("test.lime", quark_propagator2, j_decay2);
    QDPIO::cout << "j_decay2 =" << j_decay2 << std::endl;

    QDPIO::cout << "=== QIO_WRITE_NAMED_OBJECT quark_propagator ===" << std::endl;
    std::string obj_id = SetNamedLatticePropagator("", quark_propagator, j_decay, xml_out);

    GroupXML_t param_write_obj;
    param_write_obj.id = "QIO_WRITE_NAMED_OBJECT";
    param_write_obj.path = "/elem";
    param_write_obj.xml = "<elem>"
                          "<Name>QIO_WRITE_NAMED_OBJECT</Name>"
                          "<Frequency>1</Frequency>"
                          "<NamedObject>"
                          "  <object_id>" +
                          obj_id +
                          "</object_id>"
                          "  <object_type>LatticePropagator</object_type>"
                          "</NamedObject>"
                          "<File>"
                          "  <file_name>quark_prog.lime</file_name>"
                          "  <file_volfmt>SINGLEFILE</file_volfmt>"
                          "</File>"
                          "</elem>";

    RunInlineMeasurement(param_write_obj, 0, xml_out);
}

int main(int argc, char **argv)
{
    //call QDPIO::cout after ChromaInitialize
    std::cout << "=== Chroma initialize ===" << std::endl;
    Chroma::initialize(&argc, &argv); //op_chroma_initialize_self(init_input, NULL);
    START_CODE();

    QDPIO::cout << "=== layout creat ===" << std::endl;
    GroupXML_t param_nrow;
    param_nrow.xml = "<nrow>4 4 4 8</nrow>";
    param_nrow.path = "/nrow";
    CreateLayout(param_nrow);

    XMLFileWriter &xml_out = Chroma::getXMLOutputInstance();
    push(xml_out, "chroma");
    proginfo(xml_out); // Print out basic program info

    QDPIO::cout << "=== Initialise the RNG ===" << std::endl;
    GroupXML_t param_rng;
    param_rng.id = "RNG";
    param_rng.path = "/RNG";
    param_rng.xml = "<RNG>"
                    " <Seed>"
                    "  <elem>11</elem>"
                    "  <elem>11</elem>"
                    "  <elem>11</elem>"
                    "  <elem>0</elem>"
                    " </Seed>"
                    "</RNG>";
    InitRNG(param_rng, xml_out);

    QDPIO::cout << "=== load cfg as default ===" << std::endl;
    multi1d<LatticeColorMatrix> u(Nd);
    load_default_cfg(u, xml_out);

    QDPIO::cout << "=== MAKE_SOURCE Measurement: create LatticePropagator Source ===" << std::endl;
    LatticePropagator quark_source;
    int j_decay = 3;
    int t0 = 0;
#ifdef OP_BYSTEP
    make_default_source_bystep(quark_source, j_decay, t0, xml_out);
#else
    make_default_source(quark_source, j_decay, t0, xml_out);
#endif
    QDPIO::cout << "=== PropagatorInverter ===" << std::endl;
    LatticePropagator quark_propagator;
    int bc[] = {1, 1, 1, -1}; //TODO
    multi1d<int> boundary(bc, 4);
    propagator_inverter_bystep(u, quark_source, quark_propagator, boundary, j_decay, t0, xml_out);

    QDPIO::cout << "=== RunSinkSmearing SHELL_SINK ===" << std::endl;
#ifdef OP_BYSTEP
    sink_smearing_bystep(u, quark_propagator, j_decay);
#else
    sink_smearing(u, quark_propagator, j_decay);
#endif
    QDPIO::cout << "=== Initialize the slow Fourier transform phases ===" << std::endl;
    multi1d<int> t_srce(4);
    t_srce[0] = 0;
    t_srce[1] = 0;
    t_srce[2] = 0;
    t_srce[3] = t0;
    bool avg_equiv_mom = true;
    int mom2_max = 1;
    SftMom phases(mom2_max, t_srce, avg_equiv_mom, j_decay);
    // Keep a copy of the phases with NO momenta
    //SftMom phases_nomom(0, true, j_decay);
    assert(phases.getDir() == j_decay);

    QDPIO::cout << "=== ComputeMesons2 ===" << std::endl;
    multi2d<DComplex> mesprop; //result
    int gamma_value = 3;       //0~15
    ComputeMesons2(quark_propagator, quark_propagator, phases, t0, gamma_value, mesprop, xml_out);
    WriteMulti2d(mesprop, "mesons2.txt");

    QDPIO::cout << "=== ComputeBarhqlq ===" << std::endl;
    multi2d<DComplex> barprop;
    int bc_spec = boundary[j_decay];
    bool time_rev = false;
    ComputeBarhqlq(quark_propagator, quark_propagator, phases, t0, bc_spec, time_rev, 16, barprop,
                   xml_out);
    WriteMulti2d(barprop, "barprop.txt");

    // Reset the default gauge field
    InlineDefaultGaugeField::reset();
    TheNamedObjMap::Instance().erase_all();

    END_CODE();

    Chroma::finalize();
    return 0;
}

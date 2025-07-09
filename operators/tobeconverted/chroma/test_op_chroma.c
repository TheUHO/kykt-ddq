
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "oplib.h"
#include "op_chroma.h"

int gridsize[] = {4, 4, 4, 8};

int main(int argc, char **argv)
{
    oplib_init();
    ddq_op ring;
    // Chroma init
    ring = ddq_get();
    ddq_op op_init = ddq_spawn(&ring, processor_pthread, 4, 0);
    op_init->f = oplib_get(oplib_dl, "chroma", "ChromaInitialize");
    op_init->inputs[0] = obj_import(&argc, NULL, obj_prop_consumable | obj_prop_ready);
    op_init->inputs[1] = obj_import(argv, NULL, obj_prop_consumable | obj_prop_ready);
    op_init->inputs[2] = obj_import_int(4, obj_prop_consumable | obj_prop_ready);
    op_init->inputs[3] = obj_import(gridsize, NULL, obj_prop_consumable | obj_prop_ready);
    ddq_put(ring);
    ddq_loop();

    // init RNG
    ring = ddq_get();
    ddq_op op_rng = ddq_spawn(&ring, processor_pthread_qmp, 2, 0);
    op_rng->f = oplib_get(oplib_dl, "chroma", "InitRNG");
    const char *xml_rng = "<RNG>"
                          " <Seed>"
                          "  <elem>11</elem>"
                          "  <elem>11</elem>"
                          "  <elem>11</elem>"
                          "  <elem>0</elem>"
                          " </Seed>"
                          "</RNG>";
    op_rng->inputs[0] =
        obj_import_GroupXML_t("RNG", "/RNG", xml_rng, obj_prop_consumable | obj_prop_ready);
    op_rng->inputs[1] = obj_import_XMLOutputInstance(obj_prop_consumable | obj_prop_ready);
    ddq_put(ring);
    ddq_loop();

    // LoadConfig
    ring = ddq_get();
    ddq_op op_cfg = ddq_spawn_with_name("LoadConfig", &ring, processor_pthread_qmp, 2, 1);
    op_cfg->f = oplib_get(oplib_dl, "chroma", "LoadConfig");
    const char *xml_cfg = "<Cfg><cfg_type>WEAK_FIELD</cfg_type><cfg_file>dummy</cfg_file></Cfg>";
    op_cfg->inputs[0] =
        obj_import_GroupXML_t("Cfg", "/Cfg", xml_cfg, obj_prop_consumable | obj_prop_ready);
    op_cfg->inputs[1] = obj_import_XMLOutputInstance(obj_prop_consumable | obj_prop_ready);
    op_cfg->outputs[0] = obj_new(new_LatticeGauge, delete_LatticeGauge, obj_prop_consumable);

    // MesPlq
    ddq_op op_mes = ddq_spawn_with_name("MesPlq", &ring, processor_pthread_qmp, 2, 0);
    op_mes->f = oplib_get(oplib_dl, "chroma", "MesPlq");
    op_mes->inputs[0] = op_cfg->outputs[0];
    op_mes->inputs[1] = obj_import_XMLOutputInstance(obj_prop_consumable | obj_prop_ready);

    // RunQuarkSourceConstruction
    ddq_op op_psrc =
        ddq_spawn_with_name("RunQuarkSourceConstruction", &ring, processor_pthread_qmp, 3, 1);
    op_psrc->f = oplib_get(oplib_dl, "chroma", "RunQuarkSourceConstruction");
    const char *xml_psrc = "<Source>"
                           "   <version>2</version>"
                           "   <SourceType>SHELL_SOURCE</SourceType>"
                           "   <j_decay>3</j_decay>"
                           "   <t_srce>0 0 0 0</t_srce>"
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

    op_psrc->inputs[0] = obj_import_GroupXML_t("SHELL_SOURCE", "/Source", xml_psrc,
                                               obj_prop_consumable | obj_prop_ready);
    op_psrc->inputs[1] = op_cfg->outputs[0];
    op_psrc->inputs[2] = obj_import_int(eLatticePropagator, obj_prop_consumable | obj_prop_ready);
    op_psrc->outputs[0] =
        obj_new(new_LatticePropagator, delete_LatticePropagator, obj_prop_consumable);

    // DoInlinePropagatorInverter
    ddq_op op_invert =
        ddq_spawn_with_name("DoInlinePropagatorInverter", &ring, processor_pthread_qmp, 7, 1);
    op_invert->f = oplib_get(oplib_dl, "chroma", "DoInlinePropagatorInverter");
    op_invert->inputs[0] = op_cfg->outputs[0];
    op_invert->inputs[1] = op_psrc->outputs[0];
    op_invert->inputs[2] = op_psrc->inputs[2];
    const char *fermact_xml = "<FermionAction>"
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
                              "  <boundary>1 1 1 -1</boundary>"
                              " </FermionBC>"
                              "</FermionAction>";
    const char *invParam_xml = "<InvertParam>"
                               " <invType>CG_INVERTER</invType>"
                               " <RsdCG>1.0e-12</RsdCG>"
                               " <MaxCG>100</MaxCG>"
                               "</InvertParam>";
    op_invert->inputs[3] = obj_import_ChromaProp_t("WILSON", "/FermionAction", fermact_xml,
                                                   "CG_INVERTER", "/InvertParam", invParam_xml, 0,
                                                   0, obj_prop_consumable | obj_prop_ready);
    op_invert->inputs[4] = obj_import_int(0, obj_prop_ready); //t0
    op_invert->inputs[5] = obj_import_int(3, obj_prop_ready); //j_decay
    op_invert->inputs[6] = obj_import_XMLOutputInstance(obj_prop_ready);

    //obj *sobj_prop = sobj_new_LatticePropagator(3, obj_prop_consumable);
    //op_invert->outputs[0] = sobj_prop[0];
    op_invert->outputs[0] = obj_new(new_LatticePropagator, delete_LatticePropagator, obj_prop_consumable);

    // cfg copy
    //obj *sobj_smear = sobj_new_LatticeGauge(2, obj_prop_consumable);
    ddq_op op_copy_gauge =
        ddq_spawn_with_name("Copymulti1dLatticeColorMatrix", &ring, processor_pthread_qmp, 1, 1);
    op_copy_gauge->f = oplib_get(oplib_dl, "chroma", "Copymulti1dLatticeColorMatrix");
    op_copy_gauge->inputs[0] = op_cfg->outputs[0];
    //op_copy_gauge->outputs[0] = sobj_smear[0];
    op_copy_gauge->outputs[0] = obj_new(new_multi1dLatticeColorMatrix, new_multi1dLatticeColorMatrix, obj_prop_consumable);

    // cfg link smear
    ddq_op op_link_smear = ddq_spawn_with_name("RunLinkSmearing", &ring, processor_pthread_qmp, 2, 1);
    op_link_smear->f = oplib_get(oplib_dl, "chroma", "RunLinkSmearing");
    const char *link_smear_xml = "<Param>"
                                 " <LinkSmearingType>APE_SMEAR</LinkSmearingType>"
                                 " <link_smear_fact>2.5</link_smear_fact>"
                                 " <link_smear_num>1</link_smear_num>"
                                 " <no_smear_dir>3</no_smear_dir>"
                                 "</Param>";
    op_link_smear->inputs[0] = obj_import_GroupXML_t("APE_SMEAR", "/Param", link_smear_xml,
                                                     obj_prop_consumable | obj_prop_ready);
    op_link_smear->inputs[1] = op_copy_gauge->outputs[0];
    //op_link_smear->outputs[0] = sobj_smear[1];
    op_link_smear->outputs[0] = obj_dup(op_link_smear->inputs[1]);

    // prop displacement
    ddq_op op_disp = ddq_spawn_with_name("RunQuarkDisplacement", &ring, processor_pthread_qmp, 5, 1);
    op_disp->f = oplib_get(oplib_dl, "chroma", "RunQuarkDisplacement");
    const char *disp_xml = "<Displacement>"
                           "  <version>1</version>"
                           "  <DisplacementType>NONE</DisplacementType>"
                           "</Displacement>";
    op_disp->inputs[0] = obj_import_GroupXML_t("NONE", "/Displacement", disp_xml,
                                               obj_prop_consumable | obj_prop_ready);
    op_disp->inputs[1] = op_link_smear->outputs[0];
    op_disp->inputs[2] = obj_import_int(ePLUS, obj_prop_ready); //PLUS
    op_disp->inputs[3] = op_invert->outputs[0];
    op_disp->inputs[4] = obj_import_int(eLatticePropagator, obj_prop_ready);
    //op_disp->outputs[0] = sobj_prop[1];
    op_disp->outputs[0] = obj_dup(op_disp->inputs[3]);

    // prop quark smear
    ddq_op op_quark_smear = ddq_spawn_with_name("RunQuarkSmearing", &ring, processor_pthread_qmp, 4, 1);
    op_quark_smear->f = oplib_get(oplib_dl, "chroma", "RunQuarkSmearing");
    const char *quark_smear_xml = "<SmearingParam>"
                                  "  <wvf_kind>GAUGE_INV_GAUSSIAN</wvf_kind>"
                                  "  <wvf_param>2.0</wvf_param>"
                                  "  <wvfIntPar>5</wvfIntPar>"
                                  "  <no_smear_dir>3</no_smear_dir>"
                                  "</SmearingParam>";
    op_quark_smear->inputs[0] =
        obj_import_GroupXML_t("GAUGE_INV_GAUSSIAN", "/SmearingParam", quark_smear_xml,
                              obj_prop_consumable | obj_prop_ready);
    op_quark_smear->inputs[1] = op_link_smear->outputs[0];
    //op_quark_smear->inputs[2] = sobj_prop[1];
    op_quark_smear->inputs[2] = op_disp->outputs[0];
    op_quark_smear->inputs[3] = obj_import_int(eLatticePropagator, obj_prop_ready);
    //op_quark_smear->outputs[0] = sobj_prop[2];
    op_quark_smear->outputs[0] = obj_dup(op_quark_smear->inputs[2]);

    // compute mesons2
    ddq_op op_meson2 = ddq_spawn_with_name("ComputeMesons2", &ring, processor_pthread_qmp, 6, 1);
    op_meson2->f = oplib_get(oplib_dl, "chroma", "ComputeMesons2");
    op_meson2->inputs[0] = op_quark_smear->outputs[0];
    op_meson2->inputs[1] = op_quark_smear->outputs[0];
    op_meson2->inputs[2] = obj_import_SftMom(1, 0, 0, 0, 0, 1, 3, obj_prop_ready);
    op_meson2->inputs[3] = obj_import_int(0, obj_prop_ready); //t0
    op_meson2->inputs[4] = obj_import_int(3, obj_prop_ready); //gamma_value
    op_meson2->inputs[5] = obj_import_XMLOutputInstance(obj_prop_ready);
    op_meson2->outputs[0] = obj_new(new_multi2dDComplex, delete_multi2dDComplex, 0);

    // write meson2
    ddq_op op_write_meson2 = ddq_spawn_with_name("WriteMulti2d", &ring, processor_pthread_qmp, 3, 0);
    op_write_meson2->f = oplib_get(oplib_dl, "chroma", "WriteMulti2d");
    op_write_meson2->inputs[0] = op_meson2->outputs[0];
    op_write_meson2->inputs[1] = obj_import_int(eDComplex, obj_prop_ready);
    op_write_meson2->inputs[2] = obj_import("mesons2.dat", NULL, obj_prop_ready);

    // compute Barhqlq
    ddq_op op_barhqlq = ddq_spawn_with_name("ComputeBarhqlq", &ring, processor_pthread_qmp, 8, 1);
    op_barhqlq->f = oplib_get(oplib_dl, "chroma", "ComputeBarhqlq");
    op_barhqlq->inputs[0] = op_quark_smear->outputs[0];
    op_barhqlq->inputs[1] = op_quark_smear->outputs[0];
    op_barhqlq->inputs[2] = obj_import_SftMom(1, 0, 0, 0, 0, 1, 3, obj_prop_ready);
    op_barhqlq->inputs[3] = obj_import_int(0, obj_prop_ready);  //t0
    op_barhqlq->inputs[4] = obj_import_int(-1, obj_prop_ready); //boundary[j_decay]
    op_barhqlq->inputs[5] = obj_import_int(0, obj_prop_ready);  //time_rev=false
    op_barhqlq->inputs[6] = obj_import_int(16, obj_prop_ready); //baryon_value 16
    op_barhqlq->inputs[7] = obj_import_XMLOutputInstance(obj_prop_ready);
    op_barhqlq->outputs[0] = obj_new(new_multi2dDComplex, delete_multi2dDComplex, 0);

    // write Barhqlq
    ddq_op op_write_barhqlq = ddq_spawn_with_name("WriteMulti2d", &ring, processor_pthread_qmp, 3, 0);
    op_write_barhqlq->f = oplib_get(oplib_dl, "chroma", "WriteMulti2d");
    op_write_barhqlq->inputs[0] = op_barhqlq->outputs[0];
    op_write_barhqlq->inputs[1] = obj_import_int(eDComplex, obj_prop_ready);
    op_write_barhqlq->inputs[2] = obj_import("barprop.dat", NULL, obj_prop_ready);

    ddq_put(ring);
    ddq_loop();

    // Chroma final
    ring = ddq_get();
    ddq_op op_fin = ddq_spawn(&ring, processor_pthread_qmp, 0, 0);
    op_fin->f = oplib_get(oplib_dl, "chroma", "ChromaFinalize");
    ddq_put(ring);
    ddq_loop();

    //sobj_delete_LatticePropagator(sobj_prop);
    //sobj_delete_LatticeGauge(sobj_smear);

    return 0;
}

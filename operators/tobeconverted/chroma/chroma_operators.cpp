#include "chroma_operators.h"
#include "meas/smear/link_smearing_aggregate.h"
#include "meas/smear/quark_smearing_aggregate.h"
#include "meas/smear/quark_displacement_aggregate.h"
#include "meas/smear/quark_displacement_factory.h"
#include "meas/hadron/barhqlq_w.h"
#include "util/info/unique_id.h"

using namespace std;

void InitRNG(const GroupXML_t &param_rng, XMLWriter &xml_out)
{
    QDP::Seed rng_seed = 11; // default seed
    try {
        std::istringstream xml_s(param_rng.xml);
        XMLReader rng_xml_reader(xml_s);
        read(rng_xml_reader, param_rng.path, rng_seed);
    } catch (const std::string &e) {
        QDPIO::cerr << "Error reading XML : " << e << std::endl;
        QDP_abort(1);
    }
    QDP::RNG::setrn(rng_seed);
    write(xml_out, param_rng.id, rng_seed);
}

void CreateLayout(const GroupXML_t &param_nrow)
{
    /* <nrow>4 4 4 8</nrow>*/
    multi1d<int> nrow;
    try {
        std::istringstream xml_s(param_nrow.xml);
        XMLReader paramtop(xml_s);
        read(paramtop, param_nrow.path, nrow);
    } catch (const std::string &e) {
        QDPIO::cerr << "Error reading XML : " << e << std::endl;
        QDP_abort(1);
    }
    Layout::setLattSize(nrow);
    Layout::create();
}

void StartupConfig(const GroupXML_t &input_cfg, multi1d<LatticeColorMatrix> &u,
                   XMLReader &gauge_file_xml, XMLReader &gauge_xml)
{
    // Start up the config
    GaugeInitEnv::registerAll(); // not noly CfgTypeEnv::registerAll();
    // Start up the gauge field
    QDPIO::cout << "CHROMA: Attempt to read gauge field" << std::endl;
    try {
        std::istringstream xml_c(input_cfg.xml);
        XMLReader cfgtop(xml_c);
        QDPIO::cout << "CHROMA: Gauge initialization: cfg_type = " << input_cfg.id << std::endl;

        Handle<GaugeInit> gaugeInit(
            TheGaugeInitFactory::Instance().createObject(input_cfg.id, cfgtop, input_cfg.path));
        (*gaugeInit)(gauge_file_xml, gauge_xml, u);
    } catch (std::bad_cast) {
        QDPIO::cerr << "CHROMA: caught cast error" << std::endl;
        QDP_abort(1);
    } catch (std::bad_alloc) {
        // This might happen on any node, so report it
        std::cerr << "CHROMA: caught bad memory allocation" << std::endl;
        QDP_abort(1);
    } catch (const std::string &e) {
        QDPIO::cerr << "CHROMA: Caught Exception: " << e << std::endl;
        QDP_abort(1);
    } catch (std::exception &e) {
        QDPIO::cerr << "CHROMA: Caught standard library exception: " << e.what() << std::endl;
        QDP_abort(1);
    } catch (...) {
        // This might happen on any node, so report it
        std::cerr << "CHROMA: caught generic exception during gaugeInit" << std::endl;
        //QDP_abort(1);
        throw;
    }
}
void LoadConfig(const GroupXML_t &param_cfg, multi1d<LatticeColorMatrix> &u,
                XMLBufferWriter &config_xml, XMLWriter &xml_out)
{
    Cfg_t cfg;
    GroupXML_t input_cfg;
    try {
        std::istringstream xml_s(param_cfg.xml);
        XMLReader cfg_xml_reader(xml_s);
        input_cfg = readXMLGroup(cfg_xml_reader, param_cfg.path, "cfg_type");
        read(cfg_xml_reader, param_cfg.path, cfg);
    } catch (const std::string &e) {
        QDPIO::cerr << "Error reading XML : " << e << std::endl;
        QDP_abort(1);
    }

    XMLReader gauge_file_xml, gauge_xml;
    StartupConfig(input_cfg, u, gauge_file_xml, gauge_xml);

    config_xml << gauge_xml;

    // Write out the config header
    write(xml_out, "Config_info", gauge_xml);
}

// Test and grab a reference to the gauge field
void GetNamedGauge(const std::string &gauge_id, multi1d<LatticeColorMatrix> &u, XMLWriter &xml_out)
{
    XMLBufferWriter gauge_xml;
    try {
        u = TheNamedObjMap::Instance().getData<multi1d<LatticeColorMatrix>>(gauge_id);
        TheNamedObjMap::Instance().getData<multi1d<LatticeColorMatrix>>(gauge_id);
        TheNamedObjMap::Instance().get(gauge_id).getRecordXML(gauge_xml);

        // Write out the config header
        write(xml_out, "Config_info", gauge_xml);

    } catch (std::bad_cast) {
        QDPIO::cerr << "GetNamedGauge: caught dynamic cast error" << std::endl;
        QDP_abort(1);
    } catch (const std::string &e) {
        QDPIO::cerr << "GetNamedGauge: error extracting gauge field: " << e << std::endl;
        QDP_abort(1);
    }
}
// Read the quark propagator and extract headers
void GetNamedLatticePropagator(const std::string &prop_id, LatticePropagator &quark_propagator,
                               int &j_decay, XMLWriter &xml_out)
{
    //  int j_decay;  need this for diagnostics
    QDPIO::cout << "Attempt to read forward propagator" << std::endl;
    XMLReader prop_file_xml, prop_record_xml;
    try {
        // Grab a copy of the propagator. Will modify it later.
        quark_propagator = TheNamedObjMap::Instance().getData<LatticePropagator>(prop_id);

        // Snarf the prop info. This is will throw if the prop_id is not there
        TheNamedObjMap::Instance().get(prop_id).getFileXML(prop_file_xml);
        TheNamedObjMap::Instance().get(prop_id).getRecordXML(prop_record_xml);

        // Snarf out the first j_decay
        read(prop_record_xml, "/descendant::j_decay[1]", j_decay);

        // Write out the propagator header
        write(xml_out, "Prop_file_info", prop_file_xml);
        write(xml_out, "Prop_record_info", prop_record_xml);
    } catch (std::bad_cast) {
        QDPIO::cerr << "GetNamedLatticePropagator: caught dynamic cast error" << std::endl;
        QDP_abort(1);
    } catch (const std::string &e) {
        QDPIO::cerr << "GetNamedLatticePropagator: error extracting prop_header: " << e
                    << std::endl;
        QDP_abort(1);
    }
}
std::string SetNamedLatticePropagator(const std::string &prop_id,
                                      const LatticePropagator &quark_propagator, int j_decay,
                                      XMLWriter &xml_out)
{
    // Now write the source
    std::string pid = prop_id;
    if (pid.empty())
        pid = uniqueId();

    push(xml_out, "Save_Propagator");
    write(xml_out, "prop_id", pid);
    pop(xml_out);

    try {
        QDPIO::cout << "Attempt to update source" << std::endl;

        XMLBufferWriter file_xml;
        push(file_xml, "Save_Propagator");
        write(file_xml, "id", pid); // NOTE: new ID form，
        pop(file_xml);

        XMLBufferWriter record_xml;
        push(record_xml, "SavePropagator");
        write(record_xml, "j_decay", j_decay);
        pop(record_xml);

        // Store the source
        TheNamedObjMap::Instance().create<LatticePropagator>(pid);
        TheNamedObjMap::Instance().getData<LatticePropagator>(pid) = quark_propagator;
        TheNamedObjMap::Instance().get(pid).setFileXML(file_xml);
        TheNamedObjMap::Instance().get(pid).setRecordXML(record_xml);

        QDPIO::cout << "Source successfully update" << std::endl;
    } catch (std::bad_cast) {
        QDPIO::cerr << "SetNamedLatticePropagator: dynamic cast error" << std::endl;
        QDP_abort(1);
    } catch (const std::string &e) {
        QDPIO::cerr << "SetNamedLatticePropagator: error message: " << e << std::endl;
        QDP_abort(1);
    }
    return pid;
}

// Calculate some gauge invariant observables
void CheckMesPlsq(const multi1d<LatticeColorMatrix> &u, const std::string &name, XMLWriter &xml_out)
{
    MesPlq(xml_out, name, u);
}

// Sanity check - write out the propagator (pion) correlator in the Nd-1 direction
void CheckSanity(const LatticePropagator &quark_propagator, const std::string &prefix,
                 XMLWriter &xml_out)
{
    // Initialize the slow Fourier transform phases
    SftMom phases(0, true, Nd - 1);
    multi1d<Double> prop_corr = sumMulti(localNorm2(quark_propagator), phases.getSet());
    push(xml_out, prefix + "_correlator");
    write(xml_out, prefix + "_corr", prop_corr);
    pop(xml_out);
}
void CheckSanity(const LatticeStaggeredPropagator &quark_propagator, const std::string &prefix,
                 XMLWriter &xml_out)
{
    // Initialize the slow Fourier transform phases
    SftMom phases(0, true, Nd - 1);
    multi1d<Double> prop_corr = sumMulti(localNorm2(quark_propagator), phases.getSet());
    push(xml_out, prefix + "_correlator");
    write(xml_out, prefix + "_corr", prop_corr);
    pop(xml_out);
}

void WriteGauge(const multi1d<LatticeColorMatrix> &u, const std::string &file)
{
    XMLBufferWriter file_xml;
    push(file_xml, "Write_Gauge");
    write(file_xml, "file", file); // NOTE: new ID form，
    pop(file_xml);
    XMLBufferWriter record_xml;
    writeGauge(file_xml, record_xml, u, file, QDPIO_SINGLEFILE, QDPIO_SERIAL);
}
void ReadGauge(const std::string &file, multi1d<LatticeColorMatrix> &u)
{
    XMLReader file_xml;
    XMLReader record_xml;
    readGauge(file_xml, record_xml, u, file, QDPIO_SERIAL);
}

void WriteLatticePropagator(const LatticePropagator &quark_propagator, int j_decay,
                            const std::string &file)
{
    try {
        XMLBufferWriter file_xml;
        push(file_xml, "Write_Propagator");
        write(file_xml, "file", file); // NOTE: new ID form，
        pop(file_xml);

        XMLBufferWriter record_xml;
        push(record_xml, "WritePropagator");
        write(record_xml, "j_decay", j_decay);
        pop(record_xml);

        writeQprop(file_xml, record_xml, quark_propagator, file, QDPIO_SINGLEFILE, QDPIO_SERIAL);
        QDPIO::cout << "WriteLatticePropagator successfully update" << std::endl;
    } catch (std::bad_cast) {
        QDPIO::cerr << "WriteLatticePropagator: dynamic cast error" << std::endl;
        QDP_abort(1);
    } catch (const std::string &e) {
        QDPIO::cerr << "WriteLatticePropagator: error message: " << e << std::endl;
        QDP_abort(1);
    }
}
void ReadLatticePropagator(const std::string &file, LatticePropagator &quark_propagator,
                           int &j_decay)
{
    XMLReader file_xml;
    XMLReader record_xml;
    try {
        readQprop(file_xml, record_xml, quark_propagator, file, QDPIO_SERIAL);
        read(record_xml, "/descendant::j_decay[1]", j_decay);
    } catch (std::bad_cast) {
        QDPIO::cerr << "ReadLatticePropagator: dynamic cast error" << std::endl;
        QDP_abort(1);
    } catch (const std::string &e) {
        QDPIO::cerr << "ReadLatticePropagator: error message: " << e << std::endl;
        QDP_abort(1);
    }
}
void WriteLatticeFermion(const LatticeFermion &fermion, const std::string &file)
{
    XMLBufferWriter file_xml;
    push(file_xml, "Write_Fermion");
    write(file_xml, "file", file); // NOTE: new ID form，
    pop(file_xml);

    XMLBufferWriter record_xml;
    writeFermion(file_xml, record_xml, fermion, file, QDPIO_SINGLEFILE, QDPIO_SERIAL);
}
void ReadLatticeFermion(const std::string &file, LatticeFermion &fermion)
{
    XMLReader file_xml;
    XMLReader record_xml;
    readFermion(file_xml, record_xml, fermion, file, QDPIO_SERIAL);
}

template <class T>
void WriteMulti2dT(const multi2d<T> &data, const std::string &file)
{
    QDPIO::cout << "data size2=" << data.size2() << ", size1=" << data.size1() << " write to file "
                << file << std::endl;
    if (Layout::primaryNode()) {
        std::ofstream outfile(file, std::ios::out);
        for (int s = 0; s < data.size2(); s++) {
            for (int j = 0; j < data.size1(); j++) {
                outfile << data(s, j) << " ";
            }
            outfile << std::endl;
        }
    }
}

void WriteMulti2d(const multi2d<DComplex> &data, const std::string &file)
{
    return WriteMulti2dT(data, file);
}
void WriteMulti2d(const multi2d<Real> &data, const std::string &file)
{
    return WriteMulti2dT(data, file);
}
///////////////////////
void RunQuarkDisplacement(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                          enum PlusMinus isign, LatticePropagator &quark)
{
    //xml_id= "SIMPLE_DISPLACEMENT",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkDisplacementEnv::registerAll();

    Handle<QuarkDisplacement<LatticePropagator>> quarkDisplace(
        ThePropDisplacementFactory::Instance().createObject(xml_param.id, xml_reader,
                                                            xml_param.path));
    (*quarkDisplace)(quark, u, isign);
}
void RunQuarkDisplacement(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                          enum PlusMinus isign, LatticeStaggeredPropagator &quark)
{
    //xml_id= "SIMPLE_DISPLACEMENT",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkDisplacementEnv::registerAll();

    Handle<QuarkDisplacement<LatticeStaggeredPropagator>> quarkDisplace(
        TheStagPropDisplacementFactory::Instance().createObject(xml_param.id, xml_reader,
                                                                xml_param.path));
    (*quarkDisplace)(quark, u, isign);
}
void RunQuarkDisplacement(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                          enum PlusMinus isign, LatticeFermion &quark)
{
    //xml_id= "SIMPLE_DISPLACEMENT",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkDisplacementEnv::registerAll();

    Handle<QuarkDisplacement<LatticeFermion>> quarkDisplace(
        TheFermDisplacementFactory::Instance().createObject(xml_param.id, xml_reader,
                                                            xml_param.path));
    (*quarkDisplace)(quark, u, isign);
}

void RunQuarkDisplacement(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                          enum PlusMinus isign, LatticeColorVector &quark)
{
    //xml_id= "SIMPLE_DISPLACEMENT",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkDisplacementEnv::registerAll();

    Handle<QuarkDisplacement<LatticeColorVector>> quarkDisplace(
        TheColorVecDisplacementFactory::Instance().createObject(xml_param.id, xml_reader,
                                                                xml_param.path));
    (*quarkDisplace)(quark, u, isign);
}
////////////////
void RunQuarkSourceConstruction(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                                LatticePropagator &quark_prop)
{
    //xml_id= "POINT_SOURCE",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkSourceConstructionEnv::registerAll();

    Handle<QuarkSourceConstruction<LatticePropagator>> sourceConst(
        ThePropSourceConstructionFactory::Instance().createObject(xml_param.id, xml_reader,
                                                                  xml_param.path));
    quark_prop = (*sourceConst)(u);
}
void RunQuarkSourceConstruction(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                                LatticeStaggeredPropagator &quark_prop)
{
    //xml_id= "POINT_SOURCE",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkSourceConstructionEnv::registerAll();

    Handle<QuarkSourceConstruction<LatticeStaggeredPropagator>> sourceConst(
        TheStagPropSourceConstructionFactory::Instance().createObject(xml_param.id, xml_reader,
                                                                      xml_param.path));
    quark_prop = (*sourceConst)(u);
}
void RunQuarkSourceConstruction(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                                LatticeFermion &fermion)
{
    //xml_id= "POINT_SOURCE",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkSourceConstructionEnv::registerAll();

    Handle<QuarkSourceConstruction<LatticeFermion>> sourceConst(
        TheFermSourceConstructionFactory::Instance().createObject(xml_param.id, xml_reader,
                                                                  xml_param.path));
    fermion = (*sourceConst)(u);
}

////////////////////////
void RunLinkSmearing(const GroupXML_t &xml_param, multi1d<LatticeColorMatrix> &u)
{
    //xml_id= "STOUT_SMEAR","APE_SMEAR",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    LinkSmearingEnv::registerAll();

    Handle<LinkSmearing> linkSmear(
        TheLinkSmearingFactory::Instance().createObject(xml_param.id, xml_reader, xml_param.path));
    (*linkSmear)(u);
}
/////////////////////////////////////
void RunSinkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                     LatticePropagator &quark_sink)
{
    //xml_id= "POINT_SINK",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkSinkSmearingEnv::registerAll();

    Handle<QuarkSourceSink<LatticePropagator>> sinkSmear(
        ThePropSinkSmearingFactory::Instance().createObject(xml_param.id, xml_reader,
                                                            xml_param.path, u));
    (*sinkSmear)(quark_sink);
}
void RunSinkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                     LatticeStaggeredPropagator &quark_sink)
{
    //xml_id= "POINT_SINK",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkSinkSmearingEnv::registerAll();

    Handle<QuarkSourceSink<LatticeStaggeredPropagator>> sinkSmear(
        TheStagPropSinkSmearingFactory::Instance().createObject(xml_param.id, xml_reader,
                                                                xml_param.path, u));
    (*sinkSmear)(quark_sink);
}
void RunSinkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                     LatticeFermion &quark_sink)
{
    //xml_id= "POINT_SINK",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkSinkSmearingEnv::registerAll();

    Handle<QuarkSourceSink<LatticeFermion>> sinkSmear(
        TheFermSinkSmearingFactory::Instance().createObject(xml_param.id, xml_reader,
                                                            xml_param.path, u));
    (*sinkSmear)(quark_sink);
}
///////////////////////////////
void RunQuarkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                      LatticePropagator &quark)
{
    //xml_id= "GAUGE_INV_GAUSSIAN",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkSmearingEnv::registerAll();

    Handle<QuarkSmearing<LatticePropagator>> quarkSmear(
        ThePropSmearingFactory::Instance().createObject(xml_param.id, xml_reader, xml_param.path));
    (*quarkSmear)(quark, u);
}
void RunQuarkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                      LatticeStaggeredPropagator &quark)
{
    //xml_id= "GAUGE_INV_GAUSSIAN",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkSmearingEnv::registerAll();

    Handle<QuarkSmearing<LatticeStaggeredPropagator>> quarkSmear(
        TheStagPropSmearingFactory::Instance().createObject(xml_param.id, xml_reader,
                                                            xml_param.path));
    (*quarkSmear)(quark, u);
}
void RunQuarkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                      LatticeFermion &quark)
{
    //xml_id= "GAUGE_INV_GAUSSIAN",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkSmearingEnv::registerAll();

    Handle<QuarkSmearing<LatticeFermion>> quarkSmear(
        TheFermSmearingFactory::Instance().createObject(xml_param.id, xml_reader, xml_param.path));
    (*quarkSmear)(quark, u);
}
void RunQuarkSmearing(const GroupXML_t &xml_param, const multi1d<LatticeColorMatrix> &u,
                      LatticeColorVector &quark)
{
    //xml_id= "GAUGE_INV_GAUSSIAN",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    QuarkSmearingEnv::registerAll();

    Handle<QuarkSmearing<LatticeColorVector>> quarkSmear(
        TheColorVecSmearingFactory::Instance().createObject(xml_param.id, xml_reader,
                                                            xml_param.path));
    (*quarkSmear)(quark, u);
}

void RunInlineMeasurement(const GroupXML_t &xml_param, unsigned long update_no, XMLWriter &xml_out)
{
    //xml_id= "MAKE_SOURCE",
    std::istringstream xml_s(xml_param.xml);
    XMLReader xml_reader(xml_s);

    InlineAggregateEnv::registerAll();
    //include belows
    //WilsonTypeFermActsEnv::registerAll();
    //InlineMakeSourceEnv::registerAll();

    Handle<Chroma::AbsInlineMeasurement> inlineMeas(
        TheInlineMeasurementFactory::Instance().createObject(xml_param.id, xml_reader,
                                                             xml_param.path));
    QDPIO::cout << "inlineMeas->getFrequency() " << inlineMeas->getFrequency() << endl;

    (*inlineMeas)(update_no, xml_out);
}

// Ref: chroma/lib/meas/inline/hadron/inline_propagator_w.cc
void DoInlinePropagatorInverter(const multi1d<LatticeColorMatrix> &u,
                                const LatticePropagator &quark_prop_source,
                                const ChromaProp_t &params_prop, int t0, int j_decay,
                                LatticePropagator &quark_propagator, XMLWriter &xml_out)
{
    START_CODE();

    QDPIO::cout << "DoInlinePropagatorInverter: LatticePropagator calculation" << std::endl;

    StopWatch snoop;
    snoop.reset();
    snoop.start();

    push(xml_out, "propagator");
    proginfo(xml_out); // Print out basic program info

    push(xml_out, "Output_version");
    write(xml_out, "out_version", 1);
    pop(xml_out);

    // Calculate some gauge invariant observables just for info.
    MesPlq(xml_out, "Observables", u);

    // Sanity check - write out the norm2 of the source in the Nd-1 direction
    CheckSanity(quark_prop_source, "source", xml_out);

    int ncg_had = 0;

    //
    // Initialize fermion action
    //
    std::istringstream xml_s(params_prop.fermact.xml);
    XMLReader fermacttop(xml_s);

    //
    // Try the factories
    //
    bool success = false;

    if (!success) {
        try {
            StopWatch swatch;
            swatch.reset();
            QDPIO::cout << "Try the various factories" << std::endl;

            // Typedefs to save typing
            typedef LatticeFermion T;
            typedef multi1d<LatticeColorMatrix> P;
            typedef multi1d<LatticeColorMatrix> Q;

            success &= WilsonTypeFermActsEnv::registerAll();

            // Generic Wilson-Type stuff
            Handle<FermionAction<T, P, Q>> S_f(TheFermionActionFactory::Instance().createObject(
                params_prop.fermact.id, fermacttop, params_prop.fermact.path));

            Handle<FermState<T, P, Q>> state(S_f->createState(u));

            QDPIO::cout << "Suitable factory found: compute the quark prop" << std::endl;
            swatch.start();

            S_f->quarkProp(quark_propagator, xml_out, quark_prop_source, t0, j_decay, state,
                           params_prop.invParam, params_prop.quarkSpinType, params_prop.obsvP,
                           ncg_had);
            swatch.stop();
            QDPIO::cout << "Propagator computed: time= " << swatch.getTimeInSeconds() << " secs"
                        << std::endl;

            success = true;
        } catch (const std::string &e) {
            QDPIO::cout << "DoInlinePropagatorInverter: caught exception around quarkprop: " << e
                        << std::endl;
        }
    }

    if (!success) {
        QDPIO::cerr << "Error: no fermact found" << std::endl;
        QDP_abort(1);
    }

    push(xml_out, "Relaxation_Iterations");
    write(xml_out, "ncg_had", ncg_had);
    pop(xml_out);

    // Sanity check - write out the norm2 of the source in the Nd-1 direction
    CheckSanity(quark_propagator, "Prop", xml_out);

    pop(xml_out); // propagator

    snoop.stop();
    QDPIO::cout << "DoInlinePropagatorInverter: total time = " << snoop.getTimeInSeconds()
                << " secs" << std::endl;

    QDPIO::cout << "DoInlinePropagatorInverter: ran successfully" << std::endl;

    END_CODE();
}

// Ref: chroma/lib/meas/inline/hadron_s/inline_propagator_s.cc
void DoInlinePropagatorInverter(const multi1d<LatticeColorMatrix> &u,
                                const LatticeStaggeredPropagator &quark_prop_source,
                                const ChromaProp_t &params_prop, int t0, int j_decay,
                                LatticeStaggeredPropagator &quark_propagator, XMLWriter &xml_out)
{
    START_CODE();

    QDPIO::cout << "DoInlinePropagatorInverter: LatticeStaggeredPropagator calculation"
                << std::endl;

    StopWatch snoop;
    snoop.reset();
    snoop.start();

    push(xml_out, "propagator_stag");
    proginfo(xml_out); // Print out basic program info

    push(xml_out, "Output_version");
    write(xml_out, "out_version", 1);
    pop(xml_out);

    // Calculate some gauge invariant observables just for info.
    MesPlq(xml_out, "Observables", u);

    // Sanity check - write out the norm2 of the source in the Nd-1 direction
    CheckSanity(quark_prop_source, "source", xml_out);

    int ncg_had = 0;

    //
    // Initialize fermion action
    //
    std::istringstream xml_s(params_prop.fermact.xml);
    XMLReader fermacttop(xml_s);

    //
    // Try the factories
    //
    bool success = false;

    if (!success) {
        try {
            StopWatch swatch;
            swatch.reset();
            QDPIO::cout << "Try the various factories" << std::endl;

            // Typedefs to save typing
            typedef LatticeStaggeredFermion T;
            typedef multi1d<LatticeColorMatrix> P;
            typedef multi1d<LatticeColorMatrix> Q;

            success &= StaggeredTypeFermActsEnv::registerAll();

            // Generic Wilson-Type stuff
            Handle<StaggeredTypeFermAct<T, P, Q>> S_f(
                TheStagTypeFermActFactory::Instance().createObject(
                    params_prop.fermact.id, fermacttop, params_prop.fermact.path));

            Handle<FermState<T, P, Q>> state(S_f->createState(u));

            QDPIO::cout << "Suitable factory found: compute the quark prop" << std::endl;
            swatch.start();
            S_f->quarkProp(quark_propagator, xml_out, quark_prop_source, state,
                           params_prop.invParam, params_prop.quarkSpinType, ncg_had);
            swatch.stop();
            QDPIO::cout << "Propagator computed: time= " << swatch.getTimeInSeconds() << " secs"
                        << std::endl;

            success = true;
        } catch (const std::string &e) {
            QDPIO::cout << "DoInlinePropagatorInverter: caught exception around quarkprop: " << e
                        << std::endl;
        }
    }

    if (!success) {
        QDPIO::cerr << "Error: no fermact found" << std::endl;
        QDP_abort(1);
    }

    push(xml_out, "Relaxation_Iterations");
    write(xml_out, "ncg_had", ncg_had);
    pop(xml_out);

    // Sanity check - write out the propagator (pion) correlator in the Nd-1 direction
    CheckSanity(quark_propagator, "Prop", xml_out);

    pop(xml_out); // propagator

    snoop.stop();
    QDPIO::cout << "DoInlinePropagatorInverter: total time = " << snoop.getTimeInSeconds()
                << " secs" << std::endl;
    QDPIO::cout << "DoInlinePropagatorInverter: ran successfully" << std::endl;

    END_CODE();
}

//! Meson 2-pt functions
/* This routine is specific to Wilson fermions!
 *
 * Construct meson propagators and writes in COMPLEX 
 *
 * The two propagators can be identical or different.
 *
 * \param quark_prop_1  first quark propagator ( Read )
 * \param quark_prop_2  second (anti-) quark propagator ( Read )
 * \param t0            timeslice coordinate of the source ( Read )
 * \param phases        object holds list of momenta and Fourier phases ( Read )
 * \param xml_out           xml file object ( Write )
 *
 *        ____
 *        \
 * m(t) =  >  < m(t_source, 0) m(t + t_source, x) >
 *        /
 *        ----
 *          x
 */

//Ref: chroma/lib/meas/hadron/mesons2_w.cc
void ComputeMesons2(const LatticePropagator &quark_prop_1, const LatticePropagator &quark_prop_2,
                    const SftMom &phases, int t0, int gamma_value, multi2d<DComplex> &mesprop,
                    XMLWriter &xml_out)
{
    START_CODE();

    // Length of lattice in decay direction
    int length = phases.numSubsets();

    // Construct the anti-quark propagator from quark_prop_2
    int G5 = Ns * Ns - 1;
    LatticePropagator anti_quark_prop = Gamma(G5) * quark_prop_2 * Gamma(G5);

    // This variant uses the function SftMom::sft() to do all the work
    // computing the Fourier transform of the meson correlation function
    // inside the class SftMom where all the of the Fourier phases and
    // momenta are stored.  It's primary disadvantage is that it
    // requires more memory because it does all of the Fourier transforms
    // at the same time.

    push(xml_out, "mesons2");
    write(xml_out, "gamma_value", gamma_value);

    // Construct the meson correlation function
    LatticeComplex corr_fn;
    corr_fn =
        trace(adj(anti_quark_prop) * (Gamma(gamma_value) * quark_prop_1 * Gamma(gamma_value)));

    multi2d<DComplex> hsum;
    hsum = phases.sft(corr_fn);

    // Loop over sink momenta
    mesprop.resize(phases.numMom(), length);
    for (int sink_mom_num = 0; sink_mom_num < phases.numMom(); ++sink_mom_num) {
        push(xml_out, "sink");
        write(xml_out, "sink_mom_num", sink_mom_num);
        write(xml_out, "sink_mom", phases.numToMom(sink_mom_num));

        for (int t = 0; t < length; ++t) {
            int t_eff = (t - t0 + length) % length;
            mesprop[sink_mom_num][t_eff] = hsum[sink_mom_num][t];
        }

        write(xml_out, "mesprop", mesprop[sink_mom_num]);
        pop(xml_out);

    } // end for(sink_mom_num)

    pop(xml_out);
    END_CODE();
}

//! Construct current correlators
/*!
 * \ingroup hadron
 *
 * This routine is specific to Wilson fermions!
 *
 *  The two propagators can be identical or different.

 * This includes the "rho_1--rho_2" correlators used for O(a) improvement

 * For use with "rotated" propagators we added the possibility of also
 * computing the local std::vector current, when no_vec_cur = 4. In this
 * case the 3 local currents come last.

 * \param u               gauge field ( Read )
 * \param quark_prop_1    first quark propagator ( Read )
 * \param quark_prop_2    second (anti-) quark propagator ( Read )
 * \param phases          fourier transform phase factors ( Read )
 * \param t0              timeslice coordinate of the source ( Read )
 * \param no_vec_cur      number of std::vector current types, 3 or 4 ( Read )
 * \param xml_out             namelist file object ( Read )
 *
 *         ____
 *         \
 * cc(t) =  >  < m(t_source, 0) c(t + t_source, x) >
 *         /                    
 *         ----
 *           x
 */

// Ref: chroma/lib/meas/hadron/curcor2_w.cc
// LatticePropagator only for POINT_SINK?
void ComputeCurcor2(const multi1d<LatticeColorMatrix> &u, const LatticePropagator &quark_prop_1,
                    const LatticePropagator &quark_prop_2, const SftMom &phases, int t0,
                    int no_vec_cur, multi2d<Real> &vector_current, multi2d<Real> &axial_current,
                    XMLWriter &xml_out)
{
    START_CODE();

    if (no_vec_cur < 2 || no_vec_cur > 4)
        QDP_error_exit("no_vec_cur must be 2 or 3 or 4", no_vec_cur);

    push(xml_out, "curcor2");
    write(xml_out, "num_vec_cur", no_vec_cur);

    // Length of lattice in decay direction
    int length = phases.numSubsets();
    int j_decay = phases.getDir();

    LatticePropagator tmp_prop1;
    LatticePropagator tmp_prop2;

    LatticeReal psi_sq;
    LatticeReal chi_sq;

    multi1d<Double> hsum(length);

    // Construct the anti-quark propagator from quark_prop_2
    int G5 = Ns * Ns - 1;
    LatticePropagator anti_quark_prop = Gamma(G5) * quark_prop_2 * Gamma(G5);

    vector_current.resize(no_vec_cur * (Nd - 1), length);
    // Vector currents
    {
        /* Construct the 2*(Nd-1) non-local std::vector-current to rho correlators */
        int kv = -1;
        int kcv = Nd - 2;

        for (int k = 0; k < Nd; ++k) {
            if (k != j_decay) {
                int n = 1 << k;
                kv = kv + 1;
                kcv = kcv + 1;

                tmp_prop2 = u[k] * shift(quark_prop_1, FORWARD, k) * Gamma(n);
                chi_sq = -real(trace(adj(anti_quark_prop) * tmp_prop2));

                tmp_prop1 = Gamma(n) * tmp_prop2;
                psi_sq = real(trace(adj(anti_quark_prop) * tmp_prop1));

                tmp_prop2 = u[k] * shift(anti_quark_prop, FORWARD, k) * Gamma(n);
                chi_sq += real(trace(adj(tmp_prop2) * quark_prop_1));

                tmp_prop1 = Gamma(n) * tmp_prop2;
                psi_sq += real(trace(adj(tmp_prop1) * quark_prop_1));

                chi_sq += psi_sq;

                /* Do a slice-wise sum. */

                //    Real dummy1 = Real(meson_eta[n]) / Real(2);
                Real dummy1 = 0.5;

                /* The nonconserved std::vector current first */
                hsum = sumMulti(psi_sq, phases.getSet());

                for (int t = 0; t < length; ++t) {
                    int t_eff = (t - t0 + length) % length;
                    vector_current[kv][t_eff] = dummy1 * Real(hsum[t]);
                }

                /* The conserved std::vector current next */
                hsum = sumMulti(chi_sq, phases.getSet());

                for (int t = 0; t < length; ++t) {
                    int t_eff = (t - t0 + length) % length;
                    vector_current[kcv][t_eff] = dummy1 * Real(hsum[t]);
                }
            }
        }

        /* Construct the O(a) improved std::vector-current to rho correlators,
       if desired */
        if (no_vec_cur >= 3) {
            kv = 2 * Nd - 3;
            int jd = 1 << j_decay;

            for (int k = 0; k < Nd; ++k) {
                if (k != j_decay) {
                    int n = 1 << k;
                    kv = kv + 1;
                    int n1 = n ^ jd;

                    psi_sq =
                        real(trace(adj(anti_quark_prop) * Gamma(n1) * quark_prop_1 * Gamma(n)));

                    //	dummy1 = - Real(meson_eta[n]);
                    Real dummy1 = -1;

                    /* Do a slice-wise sum. */
                    hsum = sumMulti(psi_sq, phases.getSet());

                    for (int t = 0; t < length; ++t) {
                        int t_eff = (t - t0 + length) % length;
                        vector_current[kv][t_eff] = dummy1 * Real(hsum[t]);
                    }
                }
            }
        }

        /* Construct the local std::vector-current to rho correlators, if desired */
        if (no_vec_cur >= 4) {
            kv = 3 * Nd - 4;

            for (int k = 0; k < Nd; ++k) {
                if (k != j_decay) {
                    int n = 1 << k;
                    kv = kv + 1;

                    psi_sq = real(trace(adj(anti_quark_prop) * Gamma(n) * quark_prop_1 * Gamma(n)));

                    //	dummy1 = Real(meson_eta[n]);
                    Real dummy1 = 1;

                    /* Do a slice-wise sum. */
                    hsum = sumMulti(psi_sq, phases.getSet());

                    for (int t = 0; t < length; ++t) {
                        int t_eff = (t - t0 + length) % length;
                        vector_current[kv][t_eff] = dummy1 * Real(hsum[t]);
                    }
                }
            }
        }

        // Loop over currents to print
        for (int current_value = 0; current_value < vector_current.size2(); ++current_value) {
            push(xml_out, "Vector_currents"); // next array element

            write(xml_out, "current_value", current_value);
            write(xml_out, "vector_current", vector_current[current_value]);

            pop(xml_out);
        }
    }

    //
    // Axial currents
    axial_current.resize(2, length);
    {
        /* Construct the 2 axial-current to pion correlators */
        int n = G5 ^ (1 << j_decay);

        /* The local axial current first */
        psi_sq = real(trace(adj(anti_quark_prop) * Gamma(n) * quark_prop_1 * Gamma(G5)));

        /* The nonlocal axial current next */
        chi_sq = real(trace(adj(anti_quark_prop) * Gamma(n) * u[j_decay] *
                            shift(quark_prop_1, FORWARD, j_decay) * Gamma(G5)));

        // The () forces precedence
        chi_sq -= real(trace(
            adj(Gamma(n) * (u[j_decay] * shift(anti_quark_prop, FORWARD, j_decay)) * Gamma(G5)) *
            quark_prop_1));

        /* Do a slice-wise sum. */

        Real dummy1 = Real(-1) / Real(2);

        /* The local axial current first */
        hsum = sumMulti(psi_sq, phases.getSet());

        for (int t = 0; t < length; ++t) {
            int t_eff = (t - t0 + length) % length;
            axial_current[1][t_eff] = -Real(hsum[t]);
        }

        /* The nonlocal axial current next */
        hsum = sumMulti(chi_sq, phases.getSet());

        for (int t = 0; t < length; ++t) {
            int t_eff = (t - t0 + length) % length;
            axial_current[0][t_eff] = dummy1 * Real(hsum[t]);
        }

        // Loop over currents to print
        for (int current_value = 0; current_value < axial_current.size2(); ++current_value) {
            push(xml_out, "Axial_currents"); // next array element

            write(xml_out, "current_value", current_value);
            write(xml_out, "axial_current", axial_current[current_value]);

            pop(xml_out);
        }
    }
    pop(xml_out);
    END_CODE();
}

//! Heavy-light baryon 2-pt functions
/*!
   * \ingroup hadron
   *
   * This routine is specific to Wilson fermions! 
   *
   * Construct baryon propagators for the Proton and the Delta^+ with
   * degenerate "u" and "d" quarks, as well as the Lambda for, in
   * addition, a degenerate "s" quark. For these degenerate quarks, the
   * Lambda is degenerate with the Proton, but we keep it for compatibility
   * with the sister routine that treats non-degenerate quarks.

   * The routine optionally computes time-charge reversed baryons and adds them
   * in for increased statistics.

   * \param propagator_1   "s" quark propagator ( Read )
   * \param propagator_2   "u" quark propagator ( Read )
   * \param t0             cartesian coordinates of the source ( Read )
   * \param bc_spec        boundary condition for spectroscopy ( Read )
   * \param time_rev       add in time reversed contribution if true ( Read )
   * \param phases         object holds list of momenta and Fourier phases ( Read )
   * \param xml_out        xml file object ( Read )
   *
   */
//Ref: chroma/lib/meas/hadron/barhqlq_w.cc
void ComputeBarhqlq(const LatticePropagator &propagator_1, const LatticePropagator &propagator_2,
                    const SftMom &phases, int t0, int bc_spec, bool time_rev, int baryons,
                    multi2d<DComplex> &barprop, XMLWriter &xml_out)
{
    START_CODE();

    if (Ns != 4 || Nc != 3) /* Code is specific to Ns=4 and Nc=3. */
        return;

    multi3d<DComplex> bardisp1;
    multi3d<DComplex> bardisp2;

    // Forward
    barhqlq(propagator_1, propagator_2, phases, bardisp1);

    // Possibly add in a time-reversed contribution
    bool time_revP = (bc_spec * bc_spec == 1) ? time_rev : false;

    if (time_revP) {
        /* Time-charge reverse the quark propagators */
        /* S_{CT} = gamma_5 gamma_4 = gamma_1 gamma_2 gamma_3 = Gamma(7) */
        LatticePropagator q1_tmp = -(Gamma(7) * propagator_1 * Gamma(7));
        LatticePropagator q2_tmp = -(Gamma(7) * propagator_2 * Gamma(7));

        barhqlq(q1_tmp, q2_tmp, phases, bardisp2);
    }

    int num_baryons = bardisp1.size3(); //17
    int num_mom = bardisp1.size2();
    int length = bardisp1.size1();

    if (baryons < 0 || baryons >= num_baryons) {
        QDPIO::cerr << "ComputeBarhqlq num_baryons overflow: " << baryons << " , " << num_baryons
                    << std::endl;
        QDP_abort(1);
    }

    barprop.resize(num_mom, length);

    // Loop over baryons
    // for (int baryons = 0; baryons < num_baryons; ++baryons) {
    push(xml_out, "baryon"); // next array element
    write(xml_out, "baryon_num", baryons);
    for (int sink_mom_num = 0; sink_mom_num < num_mom; ++sink_mom_num) {
        write(xml_out, "sink_mom_num", sink_mom_num);
        write(xml_out, "sink_mom", phases.numToMom(sink_mom_num));
        /* forward */
        for (int t = 0; t < length; ++t) {
            int t_eff = (t - t0 + length) % length;

            if (bc_spec < 0 && (t_eff + t0) >= length)
                barprop[sink_mom_num][t_eff] = -bardisp1[baryons][sink_mom_num][t];
            else
                barprop[sink_mom_num][t_eff] = bardisp1[baryons][sink_mom_num][t];
        }
        if (time_revP) {
            /* backward */
            for (int t = 0; t < length; ++t) {
                int t_eff = (length - t + t0) % length;

                if (bc_spec < 0 && (t_eff - t0) > 0) {
                    barprop[sink_mom_num][t_eff] -= bardisp2[baryons][sink_mom_num][t];
                    barprop[sink_mom_num][t_eff] *= 0.5;
                } else {
                    barprop[sink_mom_num][t_eff] += bardisp2[baryons][sink_mom_num][t];
                    barprop[sink_mom_num][t_eff] *= 0.5;
                }
            }
        }
        write(xml_out, "barprop", barprop[sink_mom_num]);
    } // end for(sink_mom_num)
    pop(xml_out);
    //}  end for(baryons)

    END_CODE();
}
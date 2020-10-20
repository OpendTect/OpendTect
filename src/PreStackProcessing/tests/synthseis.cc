/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A. Huck
 * DATE     : Oct 2013
-*/


#include "synthseisgenerator.h"

#include "batchprog.h"
#include "testprog.h"
#include "commandlineparser.h"
#include "elasticmodel.h"
#include "factory.h"
#include "ioobj.h"
#include "moddepmgr.h"
#include "dbkey.h"
#include "ptrman.h"
#include "raytrace1d.h"
#include "raytracerrunner.h"
#include "seistrc.h"
#include "survinfo.h"
#include "waveletmanager.h"
#include "raysynthgenerator.h"

static const char* sKeyWaveletID()	{ return "Wavelet"; }
#define cStep 0.004f
#define mTest( testname, test, message ) \
if ( (test)==true ) \
{ \
    handleTestResult( true, testname ); \
} \
else \
{ \
    handleTestResult( false, testname, message ); \
    return false; \
}


void initTest( bool onespike, bool onemodel, float start_depth,
	       ElasticModelSet& models )
{
    AILayer layer1 = AILayer( start_depth, 2000.f, 2500.f );
    AILayer layer2 = AILayer( 520.f, 2600.f, 2300.f );
    AILayer layer3 = AILayer( 385.f, 3500.f, 2200.f );
    AILayer layer4 = AILayer( 350.f, 4000.f, 2800.f );

    models += new ElasticModel();
    *models[0] += layer1;
    *models[0] += layer3;
    if ( !onespike )
	*models[0] += layer4;

    if ( onemodel ) return;

    models += new ElasticModel();
    *models[1] += layer1;
    *models[1] += layer2;
    if ( onespike ) return;
    *models[1] += layer3;
    *models[1] += layer4;
}


bool testRayTracerRunner( od_ostream& strm, bool success,
			  const RayTracerRunner& rtrunner )
{
    const BufferString testname( "test RayTracerRunner" );
    mTest( testname, success, rtrunner.errMsg().getString() )

    return true;
}


bool testSynthGeneration( od_ostream& strm, bool success,
			  const RaySynthGenerator& synthgen )
{
    const BufferString testname( "test Synthetic generation" );
    mTest( testname, success, synthgen.message().getString() )

    return true;
}


bool testTraceSize( od_ostream& strm, const SeisTrc& trc )
{
    BufferString testname( "test Trace size" );
    const StepInterval<float> zrg1( 0.028f, 0.688f, cStep );
    const StepInterval<float> zrg2( -0.012f, 0.728f, cStep );
    const StepInterval<float>& zrg = trc.zRange().isEqual(zrg1,1e-4f)
				   ? zrg1 : zrg2;
    BufferString msg( "Expected trace range: [", zrg.start, " " );
    msg.add( zrg.stop ).add( "] step " ).add( zrg.step ).add( "\n" );
    msg.add( "Output trace range: [" ).add( trc.startPos() ).add( " " );
    msg.add( trc.endPos() ).add( "] step " ).add( trc.info().sampling_.step );
    mTest( testname, trc.zRange().isEqual(zrg,1e-4f), msg )

    return true;
}


bool testSpike( od_ostream& strm, const ReflectivitySpike& spike,
		const SeisTrc& trc, float scal, int nr )
{
    BufferString testname( "test Spike ", nr, " is defined" );
    mTest( testname, spike.isDefined(), "Spike is not defined" )

    testname = "test amplitude of the spike ";
    testname.add( nr ).add( " in the trace" );
    const float twt = spike.time_;
    const float ref = spike.reflectivity_.real();
    const float traceval = trc.getValue( twt, 0 );
    BufferString msg( "Trace amplitude: ", traceval, " at " );
    msg.add( twt ).add( "\n" ).add( "Expected amplitude is: " ).add( ref*scal );
    mTest( testname, mIsEqual(traceval,ref*scal,1e-3f), msg )

    return true;
}


bool testTracesAmplitudes( od_ostream& strm,
			   const SynthSeis::RayModelSet& raymodels,
			   const SynthSeis::DataSet& ds, float scal )
{
    BufferString testname( "test Traces amplitudes" );
    bool success = true;
    int nr = 0;
    for ( int imod=0; imod<raymodels.size(); imod++ )
    {
	const SynthSeis::RayModel& raymodel = *raymodels.get( imod );
	const ReflectivityModelSet& reflmodels = raymodel.reflModels();
	for ( int ioff=0; ioff<reflmodels.size(); ioff++ )
	{
	    const float offset = raymodel.rayTracerOutput().getOffset( ioff );
	    const ReflectivityModel& reflmodel = *reflmodels.get( ioff );
	    const SeisTrc& trout = *ds.getTrace( imod, offset );
	    for ( int idz=0; idz<reflmodel.size(); idz++, nr++ )
	    {
		if ( !testSpike(strm,reflmodel[idz],trout,scal,nr) )
		    success = false;
	    }
	}
    }

    mTest( testname, success, 0 )

    return true;
}


mLoad1Module("PreStackProcessing")

bool BatchProgram::doWork( od_ostream& strm )
{
    mInitBatchTestProg();

    // Inputs
    ElasticModelSet models;
    const bool singlespike = false;
    const int nrmodels = 2; // model1: 2 spikes, model2: 3 spikes
    const float start_depth = 48.f;

    ObjectSet<const Wavelet> wvlts;
    RefMan<Wavelet> syntheticricker = new Wavelet( true, 50.f, cStep, 1.f );
    wvlts += syntheticricker;

    DBKey wavid;
    if ( !pars().get(sKeyWaveletID(),wavid) )
    {
	strm << "Can not find wavelet from parameter file" << od_newline;
	return false;
    }
    ConstRefMan<Wavelet> realwav = WaveletMGR().fetch( wavid );
    if ( !realwav )
    {
	strm << "Input wavelet could not be read." << od_newline;
	return false;
    }

    wvlts += realwav;
    initTest( singlespike, nrmodels==1, start_depth, models );

    PtrMan<IOPar> raypar = pars().subselect( "Ray Tracer" );
    if ( !raypar )
    {
	strm << "Input RayTracer could not be found." << od_newline;
	return false;
    }

    // Run
    PtrMan<TaskRunner> taskr = new SilentTaskRunner;
    IOPar genpar;
    genpar.setYN( SynthSeis::GenBase::sKeyFourier(), true );
    for ( int iwav=0; iwav<wvlts.size(); iwav++ )
    {
	const Wavelet* wav = wvlts[iwav];
	if ( !wav )
	    return false;

	PtrMan<RayTracerRunner> raytracerunner =
				new RayTracerRunner( models, *raypar );
	if ( !testRayTracerRunner(strm,taskr->execute(*raytracerunner),
				  *raytracerunner) )
	    return false;

	const RefObjectSet<RayTracerData>& runnerres =raytracerunner->results();
	RefMan<SynthSeis::RayModelSet> raymodels = new SynthSeis::RayModelSet;
	for ( int imod=0; imod<runnerres.size(); imod++ )
	{
	    RefMan<SynthSeis::RayModel> raymodel =
				new SynthSeis::RayModel(*runnerres.get(imod) );
	    raymodels->add( raymodel.ptr() );
	}

	raytracerunner = 0;

	SynthSeis::GenParams sgp;
	sgp.setWaveletName( wav->name() );
	PtrMan<RaySynthGenerator> synthgen =
				new RaySynthGenerator( sgp, *raymodels.ptr() );
	synthgen->setWavelet( wav );
	synthgen->usePar( genpar );
	if ( !testSynthGeneration(strm,taskr->execute(*synthgen) &&
				  synthgen->isResultOK(),*synthgen) )
	    return false;

	ConstRefMan<SynthSeis::DataSet> ds = &synthgen->dataSet();
	synthgen = 0;

	const float scal = wav->get( wav->centerSample() );
	if ( !testTraceSize(strm,*ds->getTrace(0)) ||
	     !testTracesAmplitudes(strm,*raymodels,*ds,scal) )
	    return false;

	raymodels = 0;
    }

    taskr = 0;

    return true;
}

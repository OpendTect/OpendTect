/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"
#include "testprog.h"

#include "ailayer.h"
#include "ioman.h"
#include "ioobj.h"
#include "moddepmgr.h"
#include "reflcalc1d.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "synthseis.h"
#include "wavelet.h"

static const char* sKeyWaveletID()	{ return "Wavelet"; }
#define cStep 0.004f
#define cFreq 50.f
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
    AILayer layer1( start_depth, 2000.f, 2500.f );
    AILayer layer2( 520.f, 2600.f, 2300.f );
    AILayer layer3( 385.f, 3500.f, 2200.f );
    AILayer layer4( 350.f, 4000.f, 2800.f );

    models.add( new ElasticModel );
    models.get(0)->add( layer1.clone() );
    models.get(0)->add( layer3.clone() );
    if ( !onespike )
	models.get(0)->add( layer4.clone() );

    if ( onemodel )
	return;

    models.add( new ElasticModel );
    models.get(1)->add( layer1.clone() );
    models.get(1)->add( layer2.clone() );
    if ( onespike )
	return;

    models.get(1)->add( layer3.clone() );
    models.get(1)->add( layer4.clone() );
}


bool testSynthGeneration( od_ostream& strm, bool success,
			  Seis::RaySynthGenerator& synthgen )
{
    const BufferString testname( "test Synthetic generation" );
    mTest( testname, success, toString(synthgen.uiMessage()) )

    return true;
}


bool testTraceSize( od_ostream& strm, const SeisTrc& trc )
{
    const BufferString testname( "test Trace size" );
    const ZSampling zrg1( 0.028f, 0.688f, cStep );
    const ZSampling zrg2( -0.012f, 0.728f, cStep );
    const ZSampling& zrg = trc.zRange().isEqual(zrg1,1e-4f) ? zrg1 : zrg2;

    BufferString msg( "Expected trace range: [", zrg.start_, " " );
    msg.add( zrg.stop_ ).add( "] step " ).add( zrg.step_ ).add( "\n" );
    msg.add( "Output trace range: [" ).add( trc.startPos() ).add( " " );
    msg.add( trc.endPos() ).add( "] step " ).add( trc.info().sampling_.step_ );
    mTest( testname, trc.zRange().isEqual(zrg,1e-4f), msg )

    return true;
}


bool testSpikes( od_ostream& strm, const ReflectivityModelBase& refmodel,
		 const SeisTrcBuf& trcs, int ioff, float scal, int& nr )
{
    BufferString testname( "test RefModel ", ioff, " has data" );
    BufferString msg;

    const int nrrefs = refmodel.nrSpikes();
    const float* reftimes = refmodel.getReflTimes( ioff );
    const ReflectivityModelTrace* reftrc =
				refmodel.getReflectivities( ioff );
    const SeisTrc* trout = trcs.get( ioff );
    mTest( testname, reftimes && reftrc && refmodel.getReflDepths() &&
		     reftrc->size() == nrrefs && trout, msg );

    if ( !reftimes || !reftrc || !refmodel.getReflDepths() )
	return false;

    for ( int idz=0; idz<nrrefs; idz++, nr++ )
    {
	testname.set( "test Spike " ).add( nr ).add( " is defined" );
	mTest( testname, refmodel.isSpikeDefined(ioff,idz),
	       "Spike is not defined" )

	testname.set( "test amplitude of the spike " )
		.add( nr++ ).add( " in the trace" );
	const float twt = reftimes[idz];
	const float ref = reftrc->arr()[idz].real();
	const float traceval = trout->getValue( twt, 0 );
	msg.set( "Trace amplitude: " ).add( traceval ).add( " at " )
	   .add( twt ).add( "\n" )
	   .add( "Expected amplitude is: " ).add( ref*scal );
	mTest( testname, mIsEqual(traceval,ref*scal,1e-3f), msg )
    }

    return true;
}


bool testTracesAmplitudes( od_ostream& strm,
			   const ReflectivityModelSet& refmodels,
			   const Seis::RaySynthGenerator& synthgen, float scal )
{
    const BufferString testname( "test Traces amplitudes" );
    bool success = true;
    const int nrpos = synthgen.totalNr();
    int nr = 0;
    for ( int ipos=0; ipos<nrpos; ipos++ )
    {
	const ReflectivityModelBase* refmodel = refmodels.get( ipos );
	if ( !refmodel )
	    return false;

	const int nroffs = refmodel->nrRefModels();
	SeisTrcBuf tbuf( true );
	if ( !synthgen.getTraces(tbuf,ipos) || tbuf.size() != nroffs )
	    return false;

	for ( int ioff=0; ioff<nroffs; ioff++ )
	{
	    if ( !testSpikes(strm,*refmodel,tbuf,ioff,scal,nr) )
		success = false;
	}
    }

    mTest( testname, success, 0 )

    return true;
}


mLoad1Module("Seis")

bool BatchProgram::doWork( od_ostream& strm )
{
    // Inputs
    ElasticModelSet models;
    const bool singlespike = false;
    const int nrmodels = 2; // model1: 2 spikes, model2: 3 spikes
    const float start_depth = 48.f;

    ObjectSet<const Wavelet> wvlts;
    const Wavelet syntheticricker( true, cFreq, cStep, 1.f );
    //Ricker 50Hz, 4ms SR
    wvlts += &syntheticricker;
    MultiID wavid;
    if ( !pars().get(sKeyWaveletID(),wavid) )
    {
	strm << "Can not find wavelet from parameter file" << od_newline;
	return false;
    }

    PtrMan<IOObj> wavioobj = IOM().get( wavid );
    if ( !wavioobj )
    {
	strm << "Input wavelet is not available." << od_newline;
	return false;
    }

    PtrMan<Wavelet> realwav = Wavelet::get( wavioobj.ptr() );
    if ( !realwav )
    {
	strm << "Input wavelet could not be read." << od_newline;
	return false;
    }

    wvlts += realwav.ptr();
    initTest( singlespike, nrmodels==1, start_depth, models );

    PtrMan<IOPar> reflpar = pars().subselect( ReflCalc1D::sKeyReflPar() );
    if ( !reflpar )
    {
	strm << "Input calculator could not be found." << od_newline;
	return false;
    }

    PtrMan<TaskRunner> taskr = new TextTaskRunner( strm );
    const bool srd = 0.f;
    const Seis::OffsetType offstyp = Seis::OffsetType::OffsetMeter;
    const OD::AngleType azityp = OD::AngleType::Degrees;
    const ZDomain::DepthType depthtype = ZDomain::DepthType::Meter;

    // Run
    for ( int iwav=0; iwav<wvlts.size(); iwav++ )
    {
	const Wavelet* wav = wvlts[iwav];
	if ( !wav )
	    return false;

	const float scal = wav->get( wav->centerSample() );

	uiString msg;
	ConstRefMan<ReflectivityModelSet> refmodels =
		  Seis::RaySynthGenerator::getRefModels( models, *reflpar, msg,
			  taskr.ptr(), srd, offstyp, azityp, depthtype );
	if ( !refmodels )
	{
	    strm << ::toString(msg) << od_endl;
	    return false;
	}

	Seis::RaySynthGenerator synthgen( *refmodels.ptr() );
	synthgen.setWavelet( wav, OD::UsePtr );
	synthgen.enableFourierDomain( true );
	synthgen.doSampledTimeReflectivity( false );

	if ( !testSynthGeneration(strm,
				  TaskRunner::execute(taskr.ptr(),synthgen),
				  synthgen) )
	    return false;

	ConstPtrMan<SeisTrc> stack = synthgen.stackedTrc( nrmodels-1, false );
	if ( !stack ||
	     !testTraceSize(strm,*stack.ptr()) ||
	     !testTracesAmplitudes(strm,*refmodels.ptr(),synthgen,scal) )
	    return false;
    }

    return true;
};

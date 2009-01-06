/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2008
-*/

static const char* rcsID = "$Id: od_process_prestack.cc,v 1.2 2009-01-06 12:31:44 cvsranojay Exp $";

#include "batchprog.h"

//#include "attribdescset.h"
//#include "attribdescsettr.h"
//#include "attribengman.h"
//#include "attriboutput.h"
//#include "attribprocessor.h"
//#include "attribstorprovider.h"
#include "cubesampling.h"
#include "envvars.h"
//#include "filegen.h"
#include "prestackprocessor.h"
#include "hostdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "mmsockcommunic.h"
#include "progressmeter.h"
#include "posinfo.h"
#include "flatposdata.h"
//#include "seisjobexecprov.h"
#include "prestackgather.h"
#include "seispsread.h"
#include "seispswrite.h"
#include "seispsioprov.h"
#include "seistrc.h"
//#include "seis2dline.h"
#include "seistype.h"
//#include "separstr.h"
#include "survinfo.h"
//#include "socket.h"
#include "timefun.h"


#include "initalgo.h"
#include "initgeneral.h"
#include "initgeometry.h"
#include "initearthmodel.h"
#include "initseis.h"
#include "initattributeengine.h"
#include "initprestackprocessing.h"

#include <iostream>


#define mDestroyWorkers \
	{ delete procman; procman = 0; writer = 0; }

//defineTranslatorGroup(AttribDescSet,"Attribute definitions");


#define mRetError(s) \
{ errorMsg(s); mDestroyWorkers; return false; }

#define mRetHostErr(s) \
	{  \
	    if ( comm ) comm->setState( MMSockCommunic::HostError ); \
	    mRetError(s) \
	}

#define mRetJobErr(s) \
	{  \
	    if ( comm ) comm->setState( MMSockCommunic::JobError ); \
	    mRetError(s) \
	}

#define mRetFileProb(fdesc,fnm,s) \
	{ \
	    BufferString msg(fdesc); \
	    msg += " ("; msg += fnm; msg += ") "; msg += s; \
	    mRetHostErr( msg ); \
	}

#define mStrmWithProcID(s) \
    strm << "\n[" << process_id << "]: " << s << "." << std::endl

#define mSetCommState(State) \
	if ( comm ) \
	{ \
	    comm->setState( MMSockCommunic::State ); \
	    if ( !comm->updateState() ) \
		mRetHostErr( comm->errMsg() ) \
	}


bool BatchProgram::go( std::ostream& strm )
{
    strm << "Processing on " << HostData::localHostName() << '.' << std::endl;

    if ( !parversion_.isEmpty() )
    {
	const float vsn = atof( parversion_.buf() );
	if ( vsn < 3.2 )
	    { errorMsg("\nCannot execute pre-3.2 par files"); return false; }
    }

    const int process_id = GetPID();

    Algo::initStdClasses();
    General::initStdClasses();
    Geometry::initStdClasses();
    EarthModel::initStdClasses();
    Seis::initStdClasses();
    AttributeEngine::initStdClasses();
    PreStackProcessing::initStdClasses();

    double startup_wait = 0;
    pars().get( "Startup delay time", startup_wait );
    Time_sleep( startup_wait );

    const double pause_sleep_time = GetEnvVarDVal( "OD_BATCH_SLEEP_TIME", 1 );
    TextStreamProgressMeter progressmeter(strm);

    CubeSampling cubesampling( true );
    cubesampling.usePar( pars() );

    BufferString linekey;
    pars().get( sKey::LineKey, linekey );

    Seis::GeomType geomtype;
    if ( !Seis::getFromPar( pars(), geomtype ) )
    {
	errorMsg("\nCannot read geometry type");
	return false;
    }

    if ( geomtype!=Seis::VolPS || geomtype!=Seis::LinePS )
    {
	errorMsg("\nGeometry is not prestack");
	return false;
    }

    
    if ( geomtype==Seis::LinePS && linekey.isEmpty() )
    {
	errorMsg("\nNo linekey set" );
	return false;
    }

    MultiID inputmid;
    if ( !pars().get("Input", inputmid ) )
    {
	errorMsg("\nCannot read input id");
	return false;
    }

    PtrMan<IOObj> inputioobj = IOM().get( inputmid );
    if ( !inputioobj )
    {
	errorMsg("\nCannot create input object");
	return false;
    }

    MultiID outputmid;
    if ( !pars().get("Input", outputmid ) )
    {
	errorMsg("\nCannot read output id");
	return false;
    }

    PtrMan<IOObj> outputioobj = IOM().get( outputmid );
    if ( !outputioobj )
    {
	errorMsg("\nCannot create output object");
	return false;
    }

    PtrMan<SeisPS3DReader> reader3d = 0;
    PtrMan<SeisPS2DReader> reader2d = 0;

    StepInterval<int> cdprange(0,0,1);

    if ( geomtype==Seis::VolPS )
	reader3d = SPSIOPF().get3DReader( *inputioobj,
					  cubesampling.hrg.start.inl );
    else
    {
	reader2d = SPSIOPF().get2DReader( *inputioobj, linekey.buf() );
	if ( reader2d && !pars().get( "CDP Range", cdprange ) )
	{
	    const PosInfo::Line2DData& posdata = reader2d->posData();
	    for ( int idx=0; idx<posdata.posns_.size(); idx++ )
	    {
		if ( !idx )
		    cdprange.start = cdprange.stop = posdata.posns_[idx].nr_;
		else
		    cdprange.include( posdata.posns_[idx].nr_ );
	    }
	}
    }

    if ( !reader3d && !reader2d )
    {
	errorMsg("\nCannot create input reader");
	return false;
    }

    SeisPSReader* reader = reader3d
	? (SeisPSReader*) reader3d
	: (SeisPSReader*) reader2d;

    PtrMan<SeisPSWriter> writer = geomtype==Seis::VolPS
	? SPSIOPF().get3DWriter( *outputioobj )
	: SPSIOPF().get2DWriter( *outputioobj, linekey.buf() );

    if ( !writer )
    {
	errorMsg("\nCannot create output writer");
	return false;
    }

    mDeclareAndTryAlloc( PreStack::ProcessManager*, procman,
	    		 PreStack::ProcessManager );
    if ( !procman || !procman->usePar( pars() ) )
    {
	errorMsg("\nCannot setup process flow");
	return false;
    }

    const BinID stepout = procman->getInputStepout();


    BinID curbid; //inl not used if 2D
    BinID step;   //inl not used if 2D
    HorSamplingIterator hiter( cubesampling.hrg );

    if ( geomtype==Seis::LinePS )
    {
	curbid.crl = cdprange.start;
	step.crl = 1;
    }
    else
    {
	if ( !hiter.next(curbid) )
	{
	    errorMsg("\nNo CDP's to process");
	    return false;
	}

	step.inl = SI().inlRange(true).step;
	step.crl = SI().crlRange(true).step;
    }

    mSetCommState(Working);

    while ( true )
    {
	bool paused = false;

	if ( pauseRequested() )
	{ 
	    paused = true;
	    mSetCommState(Paused);
	    Time_sleep( pause_sleep_time );  
	    continue;
	}

	if ( paused )
	{
	    paused = false;
	    mSetCommState(Working);
	}

	procman->reset();
	BinID relbid;

	for ( relbid.inl=-stepout.inl; relbid.inl<=stepout.inl; relbid.inl++ )
	{
	    for ( relbid.crl=-stepout.crl; relbid.crl<=stepout.crl;relbid.crl++)
	    {
		if ( !procman->wantsInput( relbid ) )
		    continue;

		const BinID inputbid( curbid.inl+relbid.inl*step.inl,
				      curbid.crl+relbid.crl*step.crl );

		PreStack::Gather* gather = new PreStack::Gather;
		if ( !gather->readFrom(*inputioobj,*reader,inputbid,0) )
		{
		    delete gather;
		    continue;
		}

		DPM( DataPackMgr::FlatID() ).addAndObtain( gather );
		procman->setInput( relbid, gather->id() );
		DPM( DataPackMgr::FlatID() ).release( gather );
	    }
	}

	if ( procman->prepareWork() || procman->process() )
	{
	    const DataPack* dp =
		DPM(DataPackMgr::FlatID()).obtain(procman->getOutput());
	    mDynamicCastGet( const PreStack::Gather*, gather, dp );
	    if ( gather )
	    {
		const int nrtraces =
		    gather->size( !PreStack::Gather::offsetDim() );
		const int nrsamples =
		    gather->size( PreStack::Gather::offsetDim() );
		const StepInterval<double> zrg =
		    gather->posData().range( PreStack::Gather::offsetDim() );
		SeisTrc trc( nrsamples );
		trc.info().sampling.start = zrg.start;
		trc.info().sampling.step = zrg.step;

		if ( reader2d )
		{
		    trc.info().nr = curbid.crl;
		    PosInfo::Line2DPos linepos;
		    if ( reader2d->posData().getPos( curbid.crl, linepos ) )
			trc.info().coord = linepos.coord_;
		}
		else
		{
		    trc.info().binid = curbid;
		    trc.info().coord = SI().transform( curbid );
		}

		for ( int idx=0; idx<nrtraces; idx++ )
		{
		    trc.info().azimuth = gather->getAzimuth( idx );
		    trc.info().offset = gather->getOffset( idx );
		    for ( int idy=0; idy<nrsamples; idy++ )
			trc.set( idy, gather->data().get( idx, idy ), 0 );

		    if ( !writer->put( trc ) )
		    {
			errorMsg("\nCannot write output");
			return false;
		    }
		}
	    }

	    DPM(DataPackMgr::FlatID()).release( dp );
	}

	if ( geomtype==Seis::VolPS )
	{
	    if ( !hiter.next(curbid) )
		break;
	}
	else
	{
	    curbid.crl += cdprange.step;
	    if ( !cdprange.includes( curbid.crl ) )
		break;
	}

	++progressmeter;
    }

    // It is VERY important workers are destroyed BEFORE the last sendState!!!
    mDestroyWorkers
    progressmeter.setFinished();
    mStrmWithProcID( "Threads closed; Writing finish status" );

    if ( !comm ) return true;

    comm->setState( MMSockCommunic::Finished );
    bool ret = comm->sendState();

    if ( ret )
	mStrmWithProcID( "Successfully wrote finish status" );
    else
	mStrmWithProcID( "Could not write finish status" );

    return ret;
}



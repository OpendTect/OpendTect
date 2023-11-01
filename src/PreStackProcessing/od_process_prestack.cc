/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"
#include "envvars.h"
#include "flatposdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "jobcommunic.h"
#include "moddepmgr.h"
#include "progressmeter.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "prestackgather.h"
#include "prestackprocessor.h"
#include "prestackprocessortransl.h"
#include "seispsread.h"
#include "seispswrite.h"
#include "seispsioprov.h"
#include "seistrc.h"
#include "seistype.h"
#include "survinfo.h"
#include "trckeysampling.h"

#include <iostream>

using namespace PreStack;

#define mDestroyWorkers \
{ delete procman; procman = 0; writer = 0; }


mLoad1Module("PreStackProcessing")

bool BatchProgram::doWork( od_ostream& strm )
{
    PtrMan<SeisPSWriter> writer;
    ProcessManager* procman = nullptr;

    const int odversion = pars().odVersion();
    if ( odversion < 320 )
    { mRetError("\nCannot execute pre-3.2 par files"); }

    double startup_wait = 0.;
    pars().get( "Startup delay time", startup_wait );
    sleepSeconds( startup_wait );

    const double pause_sleep_time = GetEnvVarDVal( "OD_BATCH_SLEEP_TIME", 1 );
    TextStreamProgressMeter progressmeter(strm);

    TrcKeySampling trcsampling( true );
    const bool hastrcsampling = trcsampling.usePar( pars() );

    BufferString linekey;
    pars().get( ProcessManager::sKeyLineKey(), linekey );

    Seis::GeomType geomtype;
    if ( !Seis::getFromPar(pars(),geomtype) )
    {
	mRetError("\nCannot read geometry type");
    }

    if ( geomtype!=Seis::VolPS && geomtype!=Seis::LinePS )
    {
	mRetError("\nGeometry is not prestack");
    }

    MultiID setupmid;
    if ( !pars().get(ProcessManager::sKeySetup(),setupmid) )
    {
	mRetError( "\nCannot read setup" );
    }

    PtrMan<IOObj> setupioobj = IOM().get( setupmid );
    if ( !setupioobj )
    {
	mRetError("\nCannot create setup object");
    }

    procman = new ProcessManager;
    if ( !procman )
    {
	mRetError( "Cannot create processor");
    }

    uiString errmsg;
    if ( !PreStackProcTranslator::retrieve(*procman,setupioobj,errmsg) )
    {
	mRetError( errmsg );
    }

    if ( geomtype==Seis::LinePS && linekey.isEmpty() )
    {
	mRetError("\nNo linekey set" );
    }

    PtrMan<IOObj> inputioobj;
    if ( procman->needsPreStackInput() )
    {
	MultiID inputmid;
	if ( !pars().get(ProcessManager::sKeyInputData(),inputmid) )
	{
	    mRetError("\nCannot read input id");
	}

	inputioobj = IOM().get( inputmid );
	if ( !inputioobj )
	{
	    mRetError("\nCannot create input object");
	}
    }

    MultiID outputmid;
    if ( !pars().get(ProcessManager::sKeyOutputData(),outputmid) )
    {
	mRetError("\nCannot read output id");
    }

    PtrMan<IOObj> outputioobj = IOM().get( outputmid );
    if ( !outputioobj )
    {
	mRetError("\nCannot create output object");
    }

    SeisPSReader* reader = nullptr;
    PtrMan<SeisPS3DReader> reader3d;
    PtrMan<SeisPS2DReader> reader2d;
    StepInterval<int> cdprange(0,0,1);
    const bool needpsinput = procman->needsPreStackInput();
    if ( needpsinput )
    {
	if ( geomtype==Seis::VolPS )
	{
	    reader3d = SPSIOPF().get3DReader( *inputioobj );
	    reader = (SeisPSReader*)reader3d;
	    if ( reader3d && !hastrcsampling )
	    {
		const PosInfo::CubeData& posdata = reader3d->posData();
		if ( posdata.size() )
		{
		    StepInterval<int> inlrg, crlrg;
		    posdata.getInlRange( inlrg );
		    posdata.getCrlRange( crlrg );

		    trcsampling.init();
		    trcsampling.setInlRange( inlrg );
		    trcsampling.setCrlRange( crlrg );
		}
	    }

	    progressmeter.setTotalNr( trcsampling.totalNr() );
	}
	else
	{
	    reader2d = SPSIOPF().get2DReader( *inputioobj, linekey.buf() );
	    reader = (SeisPSReader*)reader2d;
	    if ( reader2d &&
		 !pars().get(ProcessManager::sKeyCDPRange(),cdprange) )
	    {
		const PosInfo::Line2DData& posdata = reader2d->posData();
		for ( int idx=0; idx<posdata.positions().size(); idx++ )
		{
		    if ( !idx )
			cdprange.start = cdprange.stop
				       = posdata.positions()[idx].nr_;
		    else
			cdprange.include( posdata.positions()[idx].nr_ );
		}
	    }

	    progressmeter.setTotalNr( cdprange.nrSteps()+1 );
	}

	if ( !reader )
	{
	    mRetError("\nCannot create input reader");
	}
    }
    else
    {
	procman->getProcessor(0)->adjustPossibleCompArea( trcsampling );
	progressmeter.setTotalNr( trcsampling.totalNr() );
    }


    writer = geomtype==Seis::VolPS
	    ? SPSIOPF().get3DWriter( *outputioobj )
	    : SPSIOPF().get2DWriter( *outputioobj, linekey.buf() );

    if ( !writer )
    {
	mRetError("\nCannot create output writer");
    }

    BinID curbid; //inl not used if 2D
    BinID step;   //inl not used if 2D
    TrcKeySamplingIterator hiter( trcsampling );

    if ( geomtype==Seis::LinePS )
    {
	curbid.crl() = cdprange.start;
	step.crl() = 1;
    }
    else
    {
	if ( !hiter.next(curbid) )
	{
	    mRetError("\nNo CDP's to process");
	}

	step.inl() = SI().inlRange(true).step;
	step.crl() = SI().crlRange(true).step;
    }

    mSetCommState(Working);

    RefObjectSet<Gather> gathers;
    gathers.setNullAllowed();
    TypeSet<BinID> bids;

    while ( true )
    {
	bool paused = false;

	if ( pauseRequested() )
	{
	    paused = true;
	    mSetCommState(Paused);
	    sleepSeconds( pause_sleep_time );
	    continue;
	}

	if ( paused )
	{
	    paused = false;
	    mSetCommState(Working);
	    setResumed();
	}

	procman->reset( false );
	BinID relbid;

	if ( !procman->prepareWork() )
	{
	    mRetError("\nCannot prepare processing.");
	}

	const BinID stepout = procman->getInputStepout();

	int nrfound = 0;
	RefMan<Gather> sparegather;
	for ( relbid.inl()=-stepout.inl(); relbid.inl()<=stepout.inl();
					   relbid.inl()++ )
	{
	    for ( relbid.crl()=-stepout.crl(); relbid.crl()<=stepout.crl();
					       relbid.crl()++)
	    {
		if ( !procman->wantsInput(relbid) )
		    continue;

		const BinID inputbid( curbid.inl()+relbid.inl()*step.inl(),
				      curbid.crl()+relbid.crl()*step.crl() );

		RefMan<Gather> gather;
		const int bufidx = bids.indexOf( inputbid );
		if ( bufidx!=-1 )
		{
		    gather = gathers[bufidx];
		}
		else
		{
		    if ( sparegather )
		    {
			gather = sparegather;
			sparegather = nullptr;
		    }
		    else
		    {
			gather = new Gather;
		    }

		    TrcKey tk;
		    if ( reader3d )
			tk.setPosition( inputbid );
		    else if ( reader2d )
		    {
			tk.setGeomID( reader2d->geomID() )
			  .setTrcNr( inputbid.trcNr() );
		    }

		    if ( procman->needsPreStackInput() &&
			 !gather->readFrom(*inputioobj,*reader,tk) )
		    {
			sparegather = gather;
			gather = nullptr;
		    }

		    bids += inputbid;
		    gathers += gather;
		    DPM( DataPackMgr::FlatID() ).add( gather );
		}

		if ( !gather )
		    continue;

		nrfound ++;

		procman->setInput( relbid, gather->id() );
	    }
	}

	sparegather = nullptr;

	if ( !needpsinput )
	    procman->getProcessor(0)->retainCurBID( curbid );

	if ( nrfound && procman->process() )
	{
	    auto gather = DPM(DataPackMgr::FlatID()).get<Gather>(
						    procman->getOutput() );
	    if ( gather )
	    {
		const int nrtraces =
		    gather->size( !Gather::offsetDim() );
		const int nrsamples =
		    gather->size( Gather::offsetDim() );
		const StepInterval<double> zrg =
		    gather->posData().range( Gather::offsetDim() );
		SeisTrc trc( nrsamples );
		trc.info().sampling.start = (float) zrg.start;
		trc.info().sampling.step = (float) zrg.step;

		if ( reader2d )
		{
		    trc.info().setGeomID( reader2d->geomID() )
			      .setTrcNr( curbid.trcNr() );
		}
		else
		    trc.info().setPos( curbid );

		trc.info().calcCoord();

		for ( int idx=0; idx<nrtraces; idx++ )
		{
		    if ( needpsinput )
			trc.info().azimuth = gather->getAzimuth( idx );
		    trc.info().offset = gather->getOffset( idx );
		    for ( int idy=0; idy<nrsamples; idy++ )
			trc.set( idy, gather->data().get( idx, idy ), 0 );

		    if ( !writer->put( trc ) )
		    {
			mRetError("\nCannot write output");
		    }
		}
	    }

	    ++progressmeter;
	}


	if ( geomtype==Seis::VolPS )
	{
	    const int prevline = curbid.inl();
	    if ( !hiter.next(curbid) )
		break;

	    if ( prevline!=curbid.inl() )
	    {
		const int obsoleteline =
		    curbid.inl() - (stepout.inl()+1)*step.inl();
		for ( int idx=bids.size()-1; idx>=0; idx-- )
		{
		    if ( bids[idx].inl()<=obsoleteline )
		    {
			bids.removeSingle( idx );
			gathers.removeSingle( idx );
		    }
		}
	    }
	}
	else
	{
	    curbid.crl() += cdprange.step;
	    if ( !cdprange.includes( curbid.crl(), true ) )
		break;
	    const int obsoletetrace =
		curbid.crl() -(stepout.crl()+1)*cdprange.step;
	    for ( int idx=bids.size()-1; idx>=0; idx-- )
	    {
		if ( bids[idx].crl()<=obsoletetrace )
		{
		    bids.removeSingle( idx );
		    gathers.removeSingle( idx );
		}
	    }
	}
    }

    // It is VERY important workers are destroyed BEFORE the last sendState!!!
    mDestroyWorkers
    progressmeter.setFinished();

    mMessage( "Threads closed; Writing finish status" );

    gathers.setEmpty();

    if ( !comm_ )
    {
	delete procman;
	return true;
    }

    comm_->setState( JobCommunic::Finished );
    const bool ret = comm_->sendState();
    if ( ret )
	mMessage( "Successfully wrote finish status" );
    else
	mMessage( "Could not write finish status" );

    delete procman;

    return ret;
}

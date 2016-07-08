/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2008
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
#include "progressmeterimpl.h"
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
#include "uistrings.h"

#include <iostream>

using namespace PreStack;

#define mDestroyWorkers \
{ delete procman; procman = 0; writer = 0; }


bool BatchProgram::go( od_ostream& strm )
{
    PtrMan<SeisPSWriter> writer = 0;
    ProcessManager* procman = 0;

    const int odversion = pars().odVersion();
    if ( odversion < 320 )
    { mRetError(tr("\nCannot execute pre-3.2 par files")); }

    OD::ModDeps().ensureLoaded( "PreStackProcessing" );

    double startup_wait = 0;
    pars().get( "Startup delay time", startup_wait );
    sleepSeconds( startup_wait );

    const double pause_sleep_time = GetEnvVarDVal( "OD_BATCH_SLEEP_TIME", 1 );
    TextStreamProgressMeter progressmeter(strm);

    TrcKeySampling trcsampling( true );
    const bool hastrcsampling = trcsampling.usePar( pars() );

    BufferString linekey;
    Seis::GeomType geomtype;
    if ( !Seis::getFromPar(pars(),geomtype) )
    {
	mRetError(tr("\nCannot read geometry type"));
    }

    if ( geomtype!=Seis::VolPS && geomtype!=Seis::LinePS )
    {
	mRetError(tr("\nGeometry is not prestack"));
    }

    MultiID setupmid;
    if ( !pars().get(ProcessManager::sKeySetup(),setupmid) )
    {
	mRetError(tr("\nCannot read setup"));
    }

    PtrMan<IOObj> setupioobj = IOM().get( setupmid );
    if ( !setupioobj )
    {
	mRetError(tr("\nCannot create setup object"));
    }

    procman = new ProcessManager;
    if ( !procman )
    {
	mRetError(uiStrings::phrCannotCreate(tr("processor")));
    }

    uiString errmsg;
    if ( !PreStackProcTranslator::retrieve(*procman,setupioobj,errmsg) )
    {
	mRetError( errmsg );
    }

    if ( geomtype==Seis::LinePS && linekey.isEmpty() )
    {
	mRetError(tr("\nNo line Name set"));
    }

    PtrMan<IOObj> inputioobj = 0;
    if ( procman->needsPreStackInput() )
    {
	MultiID inputmid;
	if ( !pars().get(ProcessManager::sKeyInputData(),inputmid) )
	{
	    mRetError(tr("\nCannot read input id"));
	}

	inputioobj = IOM().get( inputmid );
	if ( !inputioobj )
	{
	    mRetError(tr("\nCannot create input object"));
	}
    }

    MultiID outputmid;
    if ( !pars().get(ProcessManager::sKeyOutputData(),outputmid) )
    {
	mRetError(tr("\nCannot read output id"));
    }

    PtrMan<IOObj> outputioobj = IOM().get( outputmid );
    if ( !outputioobj )
    {
	mRetError(tr("\nCannot create output object"));
    }

    SeisPSReader* reader = 0;
    PtrMan<SeisPS3DReader> reader3d = 0;
    PtrMan<SeisPS2DReader> reader2d = 0;
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
	    mRetError(tr("\nCannot create input reader"));
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
	mRetError(tr("\nCannot create output writer"));
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
	if ( !hiter.next() )
	{
	    mRetError(tr("\nNo CDP's to process"));
	}

	curbid = hiter.curBinID();

	step.inl() = SI().inlRange(true).step;
	step.crl() = SI().crlRange(true).step;
    }

    mSetCommState(Working);

    ObjectSet<Gather> gathers;
    gathers.allowNull( true );
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
	}

	procman->reset( false );
	BinID relbid;

	if ( !procman->prepareWork() )
	{
	    mRetError(tr("\nCannot prepare processing."));
	}

	const BinID stepout = procman->getInputStepout();

	int nrfound = 0;
	RefMan<Gather> sparegather = 0;
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

		RefMan<Gather> gather = 0;
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
			sparegather = 0;
		    }
		    else
		    {
			gather = new Gather;
		    }

		    TrcKey tk( inputbid );
		    if ( procman->needsPreStackInput() &&
			 !gather->readFrom(*inputioobj,*reader,tk,0) )
		    {
			sparegather = gather;
			gather = 0;
		    }

		    bids += inputbid;

		    gathers += gather;
		    gather->ref();

		    DPM( DataPackMgr::FlatID() ).add( gather );
		}

		if ( !gather )
		    continue;

		nrfound ++;

		procman->setInput( relbid, gather->id() );
	    }
	}

	sparegather = 0;

	if ( !needpsinput )
	    procman->getProcessor(0)->retainCurBID( curbid );

	if ( nrfound && procman->process() )
	{
            RefMan<PreStack::Gather> gather =
	DPM(DataPackMgr::FlatID()).getAndCast<PreStack::Gather>(
						procman->getOutput());

	    if ( gather )
	    {
		const int nrtraces =
		    gather->size( !Gather::offsetDim() );
		const int nrsamples =
		    gather->size( Gather::offsetDim() );
		const StepInterval<double> zrg =
		    gather->posData().range( Gather::offsetDim() );
		SeisTrc trc( nrsamples );
		trc.info().sampling_.start = (float) zrg.start;
		trc.info().sampling_.step = (float) zrg.step;

		if ( reader2d )
		{
		    trc.info().trckey_ = TrcKey( curbid.lineNr(),
						 curbid.trcNr() );
		    PosInfo::Line2DPos linepos;
		    if ( reader2d->posData().getPos(curbid.crl(),linepos) )
			trc.info().coord_ = linepos.coord_;
		}
		else
		{
		    trc.info().trckey_ = TrcKey( curbid );
		    trc.info().coord_ = SI().transform( curbid );
		}

		for ( int idx=0; idx<nrtraces; idx++ )
		{
		    if ( needpsinput )
			trc.info().azimuth_ = gather->getAzimuth( idx );
		    trc.info().offset_ = gather->getOffset( idx );
		    for ( int idy=0; idy<nrsamples; idy++ )
			trc.set( idy, gather->data().get( idx, idy ), 0 );

		    if ( !writer->put( trc ) )
		    {
			mRetError(tr("\nCannot write output"));
		    }
		}
	    }

	    ++progressmeter;
	}


	if ( geomtype==Seis::VolPS )
	{
	    const int prevline = curbid.inl();
	    if ( !hiter.next() )
		break;

	    curbid = hiter.curBinID();
	    if ( prevline!=curbid.inl() )
	    {
		const int obsoleteline =
		    curbid.inl() - (stepout.inl()+1)*step.inl();
		for ( int idx=bids.size()-1; idx>=0; idx-- )
		{
		    if ( bids[idx].inl()<=obsoleteline )
		    {
			bids.removeSingle( idx );
			unRefPtr( gathers.removeSingle(idx) );
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
		    unRefPtr( gathers.removeSingle(idx) );
		}
	    }
	}
    }

    // It is VERY important workers are destroyed BEFORE the last sendState!!!
    mDestroyWorkers
    progressmeter.setFinished();

    mMessage( "Threads closed; Writing finish status" );

    deepUnRef( gathers );

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

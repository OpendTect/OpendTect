/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2013
-*/


#include "testprog.h"
#include "seisblockswriter.h"
#include "seisprovider.h"
#include "moddepmgr.h"
#include "paralleltask.h"


static bool testWriting()
{
    Seis::Blocks::Writer wrr;
    wrr.setFileNameBase( "test_seisblocks" );
    wrr.setCubeName( "Test Seisblocks Cube" );
    wrr.addComponentName( "Test SeisBlocks Component" );
    IOPar iop;
    iop.set( "Test Seisblocks key", "Test Seisblocks value" );

    wrr.addAuxInfo( "Test SeisBlocks section", iop );

    Seis::Provider* prov = Seis::Provider::create(
				DBKey::getFromString("100010.2" ) );
    if ( !prov )
	return true;

    /*
    SeisTrc trc; uiRetVal uirv; int prevlinenr = -1;
    while ( true )
    {
	uirv = prov->getNext( trc );
	if ( !uirv.isOK() )
	{
	    if ( isFinished(uirv) )
		break;
	    tstStream(true) << uirv << od_endl;
	    return false;
	}

	if ( trc.info().lineNr() != prevlinenr )
	{
	    tstStream(false) << '.';
	    prevlinenr = trc.info().lineNr();
	}

	uirv = wrr.add( trc );
	if ( uirv.isError() )
	{
	    tstStream(true) << uirv << od_endl;
	    return false;
	}
    }

    PtrMan<Task> finisher = wrr.finisher();
    if ( finisher )
    {
	bool parallel = false;
	mDynamicCastGet(ParallelTask*,ptask,finisher.ptr());
	bool res;
	if ( ptask )
	    res = ptask->executeParallel(parallel);
	else
	    res = finisher->execute();
	if ( !res )
	{
	    tstStream(true) << finisher->message() << od_endl;
	    return false;
	}
    }

    */
    return true;
}

int testMain( int argc, char** argv )
{
    mInitTestProg();
    OD::ModDeps().ensureLoaded("Seis");

    if ( !testWriting() )
	return 1;

    return 0;
}

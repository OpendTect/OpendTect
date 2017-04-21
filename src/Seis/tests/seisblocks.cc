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


static const char* sNormSeisIDStr = "100010.2";
static const char* sSteerSeisIDStr = "100010.3";
static const char* sMonsterSeisIDStr = "100010.311";

static bool testWriting()
{
    const bool usesteer = false;
    const bool usemonster = true;

    const char* seisidstr = usesteer ? sSteerSeisIDStr : sNormSeisIDStr;
    if ( usemonster )
	seisidstr = sMonsterSeisIDStr;
    Seis::Provider* prov = Seis::Provider::create(
				    DBKey::getFromString(seisidstr) );
    if ( !prov )
    {
	tstStream(true) << "Cur survey has no " << seisidstr << od_endl;
	return true; // don't need e-mails from CDash
    }

    Seis::Blocks::Writer wrr;
    if ( !usemonster )
	wrr.setFileNameBase( usesteer ? "test_blocks_steering" : "test_blocks");
    else
	wrr.setFileNameBase( "monster" );
    wrr.setCubeName( prov->name() );
    BufferStringSet compnms;
    uiRetVal uirv = prov->getComponentInfo( compnms );
    if ( uirv.isError() )
	tstStream(true) << "Hmmm can't get component info" << od_endl;
    else
    {
	for ( int idx=0; idx<compnms.size(); idx++ )
	    wrr.addComponentName( compnms.get(idx) );
    }
    IOPar iop;
    iop.setStdCreationEntries();
    iop.set( "Input DBKey", seisidstr );
    wrr.addAuxInfo( "Test section", iop );

    ///*
    SeisTrc trc; int prevlinenr = -1;
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

    //*/
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

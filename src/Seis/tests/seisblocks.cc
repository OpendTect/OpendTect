/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2013
-*/


#include "testprog.h"
#include "seisblockswriter.h"
#include "seisblocksreader.h"
#include "seisprovider.h"
#include "seistrc.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "genc.h"
#include "executor.h"


static const char* sNormSeisIDStr = "100010.2";
static const char* sSteerSeisIDStr = "100010.3";
static const char* sMonsterSeisIDStr = "100010.311";

static const bool usesteer = false;
static const bool usemonster = true;


static void prTrc( const SeisTrc& trc )
{
    const BinID bid( trc.info().binID() );
    tstStream( false ) << bid.inl() << '/' << bid.crl() << '(' << trc.size()
		       << "):" << od_endl;

    const int nrcomps = trc.nrComponents();
    for ( int idx=10; idx<trc.size(); idx +=20 )
    {
	tstStream( false ) << trc.samplePos(idx) << '=';
	for ( int icomp=0; icomp<nrcomps; icomp++ )
	    tstStream( false ) << trc.get(idx,icomp) << ' ';
	tstStream( false ) << od_endl;
    }
}


static bool testWriting()
{
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
	wrr.setFileNameBase( usesteer ? "steering" : "org_seis" );
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

    SeisTrc trc; int prevlinenr = -1;
    while ( true )
    {
	uirv = prov->getNext( trc );
	if ( !uirv.isOK() )
	{
	    if ( isFinished(uirv) )
		break;
	    tstStream(true) << uirv.getText() << od_endl;
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
	    tstStream(true) << uirv.getText() << od_endl;
	    return false;
	}
    }

    PtrMan<Task> finisher = wrr.finisher();
    if ( finisher )
    {
	LoggedTaskRunner ttr( tstStream(false) );
	ttr.execute( *finisher );
    }

    return true;
}

static bool testReading()
{
    File::Path fp( GetBaseDataDir(), sSeismicSubDir() );
    if ( !usemonster )
	fp.add( usesteer ? "steering" : "org_seis" );
    else
	fp.add( "monster" );
    fp.setExtension( "info" );

    Seis::Blocks::Reader rdr( fp.fullPath() );
    if ( rdr.state().isError() )
    {
	tstStream(true) << rdr.state().getText() << od_endl;
	return false;
    }

    SeisTrc trc;
    uiRetVal uirv;
    for ( int idx=0; idx<2000; idx++ )
    {
	uirv = rdr.getNext( trc );
	if ( uirv.isError() )
	{
	    tstStream(true) << uirv.getText() << od_endl;
	    return false;
	}
    }
    prTrc( trc );

    rdr.get( BinID(425,800), trc );
    prTrc( trc );
    return true;
}


int testMain( int argc, char** argv )
{
    mInitTestProg();
    OD::ModDeps().ensureLoaded("Seis");


    if ( !testWriting() )
	return 1;

    if ( !testReading() )
	return 1;

    return 0;
}

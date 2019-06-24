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
#include "plugins.h"

/*

static const char* sNormSeisIDStr = "100010.2";
static const char* sSteerSeisIDStr = "100010.3";
static const char* sMonsterSeisIDStr = "100010.321";

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
    Seis::Provider* prov = Seis::Provider::create( DBKey(seisidstr) );
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
	return true;
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

*/

/* Expected output:

   175/560(401):
   0.24=2236
   0.32=-184
   0.4=2641
   0.48=1923
   0.56=2484
   0.64=165
   0.72=1600
   0.8=-1866
   0.88=4456
   0.96=-555
   1.04=3853
   1.12=1410
   1.2=-1148
   1.28=-4339
   1.36=1431
   1.44=246
   1.52=4077
   1.6=4706
   1.68=1953
   1.76=-1134
   425/800(401):
   0.24=639
   0.32=790
   0.4=-2913
   0.48=864
   0.56=5691
   0.64=-2615
   0.72=-1428
   0.8=831
   0.88=1933
   0.96=774
   1.04=3927
   1.12=-311
   1.2=-1351
   1.28=2836
   1.36=-2900
   1.44=5421
   1.52=-207
   1.6=-5437
   1.68=862
   1.76=176
*/


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    OD::ModDeps().ensureLoaded("Seis");
    PIM().loadAuto( false );

    /*
    if ( !testWriting() )
	return 1;

    if ( !testReading() )
	return 1;
    */

    return 0;
}

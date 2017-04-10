/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2016
-*/

#include "seisprovider.h"
#include "seisinfo.h"
#include "seisbuf.h"
#include "seisselectionimpl.h"
#include "testprog.h"
#include "moddepmgr.h"
#include "seispreload.h"
#include "executor.h"

#include <iostream>

#define mCreateProvider( dbkeystr ) \
const DBKey dbky = DBKey::getFromString( dbkeystr ); \
\
uiRetVal uirv; \
PtrMan<Seis::Provider> prov = Seis::Provider::create( dbky, &uirv ); \
if ( !prov ) \
{ \
    od_cout() << uirv << od_endl; \
    return true; \
}

#define mRetIfNotOK( uirv ) \
if ( !uirv.isOK() ) \
{ \
   if ( isFinished(uirv) ) \
       od_cout() << ">At End<" << od_endl; \
   else \
       od_cout() << uirv << od_endl; \
   \
   return; \
}

// Using F3_Test_Survey
static const char* dbkeyvol = "100010.2";
static const char* dbkeyvol_with_missing_trcs = "100010.14";
static const char* dbkeysteer = "100010.9";
static const char* dbkeyline = "100010.13";
static const char* dbkeyline_with_missing_trcs = "100010.21";
static const char* dbkeyps3d = "100010.5";

static const TrcKey tk_1300_1200( BinID(1300,1200) );
static const TrcKey tk_500_500( BinID(500,500) );
static const TrcKey tk_298_1200( BinID(298,1200) );
//!< Trace before first missing inline.
static const TrcKey tk_3_200( 3, 200 );
static const TrcKey tk_last( BinID(650,1200) );
static const TrcKey tk_17_170( 17, 170 );
//!< Trace before first missing trace on line.


static void prTrc( const char* start, const SeisTrc& trc, const uiRetVal& uirv,
		   bool withcomps=false, bool withoffs=false )
{
    if ( start )
	od_cout() << start << ": ";

    mRetIfNotOK( uirv );

    od_cout() << trc.info().binID().inl()
	      << '/' << trc.info().binID().crl()
	      << " #samples=" << trc.size();
    if ( withcomps )
	od_cout() << " #nrcomps=" << trc.nrComponents();
    if ( withoffs )
	od_cout() << " O=" << trc.info().offset_;

    od_cout() << od_endl;
}

static void prBuf( const char* start, const SeisTrcBuf& tbuf,
		    const uiRetVal& uirv )
{
    if ( start )
	od_cout() << start << ": ";

    mRetIfNotOK( uirv );

    const int sz = tbuf.size();
    if ( sz < 1 )
	od_cout() << ">Empty buf<" << od_endl;
    else
    {
	const SeisTrc& trc0 = *tbuf.get( 0 );
	const SeisTrc& trc1 = *tbuf.get( sz-1 );
	od_cout() << trc0.info().binID().inl()
		  << '/' << trc0.info().binID().crl();
	od_cout() << " [" << sz << "] O=" << trc0.info().offset_;
	od_cout() << "-" << trc1.info().offset_ << od_endl;
    }
}


static bool testVol()
{
    od_cout() << "\n\n---- 3D Volume ----\n" << od_endl;
    mCreateProvider( dbkeyvol );

    od_cout() << "From disk:\n" << od_endl;
    SeisTrc trc;
    uirv = prov->getNext( trc );
    prTrc( "First next", trc, uirv );
    uirv = prov->getNext( trc );
    prTrc( "Second next", trc, uirv );
    uirv = prov->get( tk_1300_1200, trc );
    prTrc( "tk_1300_1200", trc, uirv );
    od_cout() << "\n";

    TrcKeySampling hs;
    hs.start_ = hs.stop_ = tk_last.binID();
    prov->setSelData( new Seis::RangeSelData( hs ) );
    uirv = prov->getNext( trc );
    prTrc( "First next after subsel to last trc", trc, uirv );
    uirv = prov->getNext( trc );
    prTrc( "Second next after subsel to last trc", trc, uirv );
    od_cout() << "\n";

    hs.start_.inl() = hs.start_.crl() = 1200;
    hs.stop_ = hs.start_;
    prov->setSelData( new Seis::RangeSelData( hs ) );
    uirv = prov->getNext( trc );
    prTrc( "First next after subsel to outside data range", trc, uirv );
    uirv = prov->getNext( trc );
    prTrc( "Second next after subsel to outside data range", trc, uirv );
    od_cout() << "\n";

    Seis::PreLoader pl( dbky );
    TextTaskRunner taskrunner( od_cout() );
    pl.setTaskRunner( taskrunner );
    TrcKeyZSampling tkzs( true );
    tkzs.hsamp_.start_ = tkzs.hsamp_.stop_ = tk_500_500.binID();
    pl.load( tkzs );
    prov->setSelData( new Seis::RangeSelData(tkzs.hsamp_) );
    uirv = prov->getNext( trc );
    prTrc( "First next after subsel to preloaded trc", trc, uirv );
    uirv = prov->getNext( trc );
    prTrc( "Second next after subsel to preloaded trc", trc, uirv );
    uirv = prov->get( tk_1300_1200, trc );
    prTrc( "tk_1300_1200", trc, uirv );

    pl.unLoad();

    od_cout() << "\n\n---- 3D Volume with gaps ----\n" << od_endl;

    prov->setInput( DBKey::getFromString(dbkeyvol_with_missing_trcs) );
    uirv = prov->get( tk_298_1200, trc );
    prTrc( "Trc before first missing inline", trc, uirv );
    uirv = prov->getNext( trc );
    prTrc( "First next", trc, uirv );
    uirv = prov->getNext( trc );
    prTrc( "Second next", trc, uirv );
    od_cout() << "\n";

    od_cout() << "\n\n---- 3D Steering Cube ----\n" << od_endl;

    od_cout() << "Component selection:\n" << od_endl;
    prov->setInput( DBKey::getFromString(dbkeysteer) );
    uirv = prov->getNext( trc );
    prTrc( "First next", trc, uirv, true );

    int comps[2] = { 0, 1 };
    prov->selectComponent( comps[0] );
    uirv = prov->get( trc.info().trckey_, trc );
    prTrc( "1st comp selected", trc, uirv, true );

    prov->selectComponent( comps[1] );
    uirv = prov->get( trc.info().trckey_, trc );
    prTrc( "2nd comp selected", trc, uirv, true );

    prov->selectComponents( TypeSet<int>(comps,2) );
    uirv = prov->get( trc.info().trckey_, trc );
    prTrc( "Both comps selected", trc, uirv, true );

    prov->selectComponent( -1 );
    uirv = prov->get( trc.info().trckey_, trc );
    prTrc( "No comp selected", trc, uirv, true );

    return true;
}


static bool testLine()
{
    od_cout() << "\n\n---- 2D Lines ----\n" << od_endl;
    mCreateProvider( dbkeyline );

    SeisTrc trc;
    uirv = prov->getNext( trc );
    prTrc( "First next", trc, uirv );
    uirv = prov->getNext( trc );
    prTrc( "Second next", trc, uirv );
    const int curlinenr = trc.info().lineNr();
    while ( trc.info().lineNr() == curlinenr )
    {
	uirv = prov->getNext( trc );
	if ( uirv.isError() )
	{
	    od_cout() << uirv << od_endl;
	    break;
	}
    }
    prTrc( "First on following line", trc, uirv );

    uirv = prov->get( tk_3_200, trc );
    prTrc( "tk_3_200", trc, uirv );

    od_cout() << "\n\n---- 2D Line with a gap ----\n" << od_endl;

    prov->setInput( DBKey::getFromString(dbkeyline_with_missing_trcs) );
    uirv = prov->get( tk_17_170, trc );
    prTrc( "Trc before first missing trc", trc, uirv );
    uirv = prov->getNext( trc );
    prTrc( "First next", trc, uirv );
    uirv = prov->getNext( trc );
    prTrc( "Second next", trc, uirv );

    return true;
}


static bool testPS3D()
{
    od_cout() << "\n\n---- 3D Pre-Stack ----\n" << od_endl;
    mCreateProvider( dbkeyps3d );

    SeisTrc trc;
    SeisTrcBuf tbuf( false );
    uirv = prov->getNext( trc );
    prTrc( "First next", trc, uirv, false, true );
    uirv = prov->get( tk_1300_1200, trc );
    prTrc( "tk_1300_1200", trc, uirv, false, true );

    BufferStringSet compnms;
    uirv = prov->getComponentInfo( compnms );
    if ( uirv.isError() )
    {
	od_cout() << uirv << od_endl;
	return true;
    }
    BufferString prstr = compnms.getDispString( 4 );
    od_cout() << prstr << od_endl;
    prov->selectComponent( 1 );
    prov->reset();
    uirv = prov->getNext( trc );
    prTrc( "First next after component sel", trc, uirv, true, true );

    prov->selectComponent( -1 );
    prov->reset();
    uirv = prov->getNextGather( tbuf );
    prBuf( "First next", tbuf, uirv );
    uirv = prov->getGather( tk_1300_1200, tbuf );
    prBuf( "tk_1300_1200", tbuf, uirv );

    return true;
}


int testMain( int argc, char** argv )
{
    mInitTestProg();
    OD::ModDeps().ensureLoaded("Seis");

    if ( !testVol() )
	ExitProgram( 1 );
    if ( !testLine() )
	ExitProgram( 1 );
    if ( !testPS3D() )
	ExitProgram( 2 );

    return ExitProgram( 0 );
}

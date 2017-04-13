/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2016
-*/

#include "seisprovidertester.h"

#include "seisprovider.h"
#include "seisbuf.h"
#include "seisselectionimpl.h"
#include "testprog.h"
#include "moddepmgr.h"

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

// 3D
static const TrcKey tk_before_first_missing_inl( BinID(298,1200) );
static const TrcKey tk_last( BinID(650,1200) );

// 2D
static const TrcKey tk_before_first_missing_trc( 17, 170 );

// 3D Pre-Stack
static const TrcKey tk_1300_1200( BinID(1300,1200) );


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
    od_cout() << "\n---- 3D Volume ----\n" << od_endl;

    Seis::ProviderTester tester;
    tester.setInput( dbkeyvol );
    tester.testGetNext();

    TrcKeySampling tks;
    tks.start_ = tks.stop_ = tk_last.binID();
    tester.testSubselection( new Seis::RangeSelData(tks),
			     "Subselection to last trc:" );

    tks.start_.inl() = tks.start_.crl() = tk_last.binID().crl();
    tks.stop_ = tks.start_;
    tester.testSubselection( new Seis::RangeSelData(tks),
			     "Subselection to outside data range:" );

    tester.testPreLoadTrc( false );

    od_cout() << "\n---- 3D Volume with gaps ----\n" << od_endl;
    tester.setInput( dbkeyvol_with_missing_trcs );
    tester.testGetTrc( tk_before_first_missing_inl,
		       "Trc before first missing inline" );
    tester.testGetNext();

    od_cout() << "\n---- 3D Steering Cube ----\n" << od_endl;
    tester.setInput( dbkeysteer );
    tester.testComponentSelection();
    return true;
}


static bool testLine()
{
    od_cout() << "\n---- 2D Lines ----\n" << od_endl;

    Seis::ProviderTester tester;
    tester.setInput( dbkeyline );
    tester.testGetNext();
    tester.testPreLoadTrc();
    tester.testComponentSelection();

    od_cout() << "\n---- 2D Line with a gap ----\n" << od_endl;

    tester.setInput( dbkeyline_with_missing_trcs );
    tester.testGetTrc( tk_before_first_missing_trc,
		       "Trc before first missing trc" );
    tester.testGetNext();
    return true;
}


static bool testPS3D()
{
    od_cout() << "\n\n---- 3D Pre-Stack ----\n" << od_endl;
    mCreateProvider( dbkeyps3d );

    Seis::ProviderTester tester;
    tester.setInput( dbkeyps3d );
    tester.testGetNext();
    tester.testGetTrc( tk_1300_1200 );

    prov->reset();
    tester.testComponentSelection();

    prov->reset();
    SeisTrcBuf tbuf( false );
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

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2016
-*/

#include "seisprovidertester.h"

#include "seisselectionimpl.h"
#include "testprog.h"
#include "trckeyzsampling.h"
#include "moddepmgr.h"

/*!
Test program for Seis::Provider for testing standard usages of provider
class for all four geom types: Vol, Line, VolPS, LinePS. Uses
Seis::ProviderTester for testing various functionalities.

Required surveys: F3_Test_Survey and Penobscot_Test_survey.

Covered cases: Output of get, getNext functions for preloaded and
non-preloaded data with and without subselection for various data types.
Behaviour of these functions for special cases like end of data, missing and
non-existent traces have also been covered. More functionality tests include
IOPar usage etc., Additional test cases can be added if required. Can be
tested for other datastores simply by changing the static dbkey and/or
trckey members.
*/

// Using F3_Test_Survey
static const char* dbkeyvol = "100010.2";
static const char* dbkeyvol_with_missing_trcs = "100010.14";
static const char* dbkeysteer = "100010.9";
static const char* dbkeyline = "100010.13";
static const char* dbkeyline_with_missing_trcs = "100010.21";

static const TrcKey tk_before_first_missing_inl( BinID(298,1200) );
static const TrcKey tk_last( BinID(650,1200) );
static const TrcKey tk_before_first_missing_trc( 17, 170 );

// Using Penobscot_Test_survey for Pre-Stack
static const char* dbkeyps3d = "100010.5";
static const char* dbkeyps2d = "100010.13";
static const TrcKey tk_1300_1200( BinID(1300,1200) );

// Non-existent trace
static const TrcKey tk_non_existent( 9999999, 9999999 );


static bool testVol()
{
    od_cout() << "\n---- 3D Volume ----\n" << od_endl;

    Seis::ProviderTester tester;
    tester.setInput( dbkeyvol );
    tester.testGetNext();

    TrcKeySampling tks;
    tks.start_ = tks.stop_ = tk_last.binID();
    tester.testSubselection( new Seis::RangeSelData(tks),
			     "Subselection to last trc" );

    tks.stop_ = tks.start_ = tk_non_existent.binID();
    tester.testSubselection( new Seis::RangeSelData(tks),
			     "Subselection to outside data range" );
    tester.testPreLoad( TrcKeyZSampling(true) );
    tester.testPreLoadTrc( false );
    tester.testIOParUsage();

    od_cout() << "\n---- 3D Volume with gaps ----\n" << od_endl;
    tester.setInput( dbkeyvol_with_missing_trcs );
    tester.testGet( tk_before_first_missing_inl,
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
    tester.testIOParUsage();

    od_cout() << "\n---- 2D Line with a gap ----\n" << od_endl;

    tester.setInput( dbkeyline_with_missing_trcs );
    tester.testGet( tk_before_first_missing_trc,
		    "Trc before first missing trc" );
    tester.testGetNext();
    return true;
}


static bool testPS3D()
{
    od_cout() << "\n---- 3D Pre-Stack ----\n" << od_endl;

    Seis::ProviderTester tester;
    tester.setInput( dbkeyps3d );
    tester.testGetNext();

    TrcKeySampling tks;
    tks.start_ = tks.stop_ = tk_1300_1200.binID();
    tester.testSubselection( new Seis::RangeSelData(tks),
			     "Subselection to tk_1300_1200:" );
    tester.testPreLoadTrc();
    tester.testIOParUsage();
    tester.testComponentSelection();
    return true;
}


static bool testPS2D()
{
    od_cout() << "\n---- 2D Pre-Stack ----\n" << od_endl;

    Seis::ProviderTester tester;
    tester.setInput( dbkeyps2d );
    tester.testGetNext();

    TrcKeySampling tks;
    tks.set2DDef();
    tks.start_ = tks.stop_ = tk_non_existent.binID();
    tester.testSubselection( new Seis::RangeSelData(tks),
			     "Subselection to non-existent trc:" );
    tester.testPreLoadTrc();
    tester.testIOParUsage();
    tester.testComponentSelection();
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
	ExitProgram( 1 );
    if ( !testPS2D() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2016
-*/

#include "seisprovidertester.h"

#include "seisselectionimpl.h"
#include "seisioobjinfo.h"
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

// Surveys
static const char* survname = "F3_Test_Survey";
static const char* pssurvname = "Penobscot_Test_Survey";

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
static const char* dbkeyps2d = "100010.9";
static const TrcKey tk_1300_1200( BinID(1300,1200) );

// Non-existent trace
static const TrcKey tk_non_existent( 9999999, 9999999 );

#define mTestGet( tk, txt ) \
if ( !tester.testGet(tk,txt) ) \
    return false;

#define mTestGetNext() \
if ( !tester.testGetNext() ) \
    return false;

#define mTestSubselection( seldata, txt, outsidedatarg ) \
if ( !tester.testSubselection(seldata,txt,outsidedatarg) ) \
    return false;

#define mTestPreLoad( tkzs ) \
if ( !tester.testPreLoad(tkzs) ) \
    return false;

#define mTestPreLoadTrc( currenttrc ) \
if ( !tester.testPreLoadTrc(currenttrc) ) \
    return false;

#define mTestIOParUsage() \
if ( !tester.testIOParUsage() ) \
    return false;

#define mTestComponentSelection() \
if ( !tester.testComponentSelection() ) \
    return false;

static bool testVol()
{
    od_cout() << "\n---- 3D Volume ----\n" << od_endl;

    Seis::ProviderTester tester;
    tester.setSurveyName( survname );
    tester.setInput( dbkeyvol );

    mTestGetNext();
    TrcKeySampling tks;
    tks.start_ = tks.stop_ = tk_last.binID();
    mTestSubselection( new Seis::RangeSelData(tks),
	    	       "Subselection to last trc", false );

    tks.stop_ = tks.start_ = tk_non_existent.binID();
    mTestSubselection( new Seis::RangeSelData(tks),
	    	       "Subselection to outside data range", true );
    mTestPreLoad( TrcKeyZSampling(true) );
    mTestPreLoadTrc( false );
    mTestIOParUsage();

    od_cout() << "\n---- 3D Volume with gaps ----\n" << od_endl;
    tester.setInput( dbkeyvol_with_missing_trcs );
    mTestGet( tk_before_first_missing_inl, "Trc before first missing inline" );
    mTestGetNext();

    od_cout() << "\n---- 3D Steering Cube ----\n" << od_endl;
    tester.setInput( dbkeysteer );
    mTestComponentSelection();
    return true;
}


static bool testLine()
{
    od_cout() << "\n---- 2D Lines ----\n" << od_endl;

    Seis::ProviderTester tester;
    tester.setSurveyName( survname );
    tester.setInput( dbkeyline );

    mTestGetNext();
    mTestPreLoadTrc();
    mTestIOParUsage();
    mTestComponentSelection();
    
    od_cout() << "\n---- 2D Line with a gap ----\n" << od_endl;

    tester.setInput( dbkeyline_with_missing_trcs );
    mTestGet( tk_before_first_missing_trc, "Trc before first missing trc" );
    mTestGetNext();
    return true;
}


static bool testPS3D()
{
    od_cout() << "\n---- 3D Pre-Stack ----\n" << od_endl;

    Seis::ProviderTester tester;
    tester.setSurveyName( pssurvname );
    tester.setInput( dbkeyps3d );
    mTestGetNext();

    SeisIOObjInfo info( DBKey::getFromString(dbkeyps3d) );
    TrcKeyZSampling tkzs;
    info.getRanges( tkzs );
    tkzs.hsamp_.stop_.inl() = tkzs.hsamp_.start_.inl();
    mTestSubselection( new Seis::RangeSelData(tkzs.hsamp_),
	    	       "Subselection to one in-line:", false );

    TrcKeySampling tks;
    tks.start_ = tks.stop_ = tk_1300_1200.binID();
    mTestSubselection( new Seis::RangeSelData(tks),
	    	       "Subselection to tk_1300_1200:", false );
    mTestPreLoadTrc();
    mTestIOParUsage();
    mTestComponentSelection();
    return true;
}


static bool testPS2D()
{
    od_cout() << "\n---- 2D Pre-Stack ----\n" << od_endl;

    Seis::ProviderTester tester;
    tester.setSurveyName( pssurvname );
    tester.setInput( dbkeyps2d );
    mTestGetNext();
    
    TrcKeySampling tks;
    tks.set2DDef();
    tks.start_ = tks.stop_ = tk_non_existent.binID();
    mTestSubselection( new Seis::RangeSelData(tks),
	    	       "Subselection to non-existent trc", true );
    mTestPreLoadTrc();
    mTestIOParUsage();
    mTestComponentSelection();
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

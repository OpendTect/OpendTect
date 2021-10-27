/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman K Singh
 * DATE     : April 2013
 * FUNCTION : Test various functions of HorSampling and CubeSampling.
-*/


#include "testprog.h"

#include "cubesampling.h"
#include "odjson.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

#define mDeclCubeSampling( cs, istart, istop, istep, \
			       cstart, cstop, cstep, \
			       zstart, zstop, zstep) \
    CubeSampling cs( false ); \
    cs.hsamp_.set( StepInterval<int>(istart,istop,istep), \
	        StepInterval<int>(cstart,cstop,cstep) ); \
    cs.zsamp_.set( zstart, zstop, zstep );


#define mRetResult( funcname ) \
    { \
	handleTestResult( false, funcname ); \
	return false; \
    } \
    else \
	handleTestResult( true, funcname ); \
    return true;


static bool testEmpty()
{
    CubeSampling cs0( false );
    cs0.setEmpty();
    if ( cs0.nrInl() || cs0.nrCrl() || !cs0.isEmpty() )
	mRetResult( "testEmpty" );
}


static bool testInclude()
{
    mDeclCubeSampling( cs1, 2, 50, 6,
			10, 100, 9,
			1.0, 3.0, 0.004 );
    mDeclCubeSampling( cs2, 1, 101, 4,
			4, 100, 3,
			-1, 4.0, 0.005 );
    mDeclCubeSampling( expcs, 1, 101, 1,
			  4, 100, 3,
			  -1, 4, 0.004 );
    cs1.include ( cs2 );
    if ( cs1 != expcs )
	mRetResult( "testInclude" );
}


static bool testIncludes()
{
    mDeclCubeSampling( cs1, 2, 50, 6,
			10, 100, 9,
			1.0, 3.0, 0.004 );
    mDeclCubeSampling( cs2, 1, 101, 4,
			4, 100, 3,
			-1, 4.0, 0.005 );
    mDeclCubeSampling( cs3, 1, 101, 1,
			4, 100, 3,
			-1, 4, 0.004 );
    if ( cs2.includes(cs1) || !cs3.includes(cs1) )
	mRetResult( "testIncludes" );
}


static bool testLimitTo()
{
    mDeclCubeSampling( cs1, 3, 63, 6,
			10, 100, 1,
			1.0, 3.0, 0.004 );
    mDeclCubeSampling( cs2, 13, 69, 4,
			4, 100, 1,
			-1.0, 2.0, 0.005 );
    mDeclCubeSampling( cs3, 2, 56, 2,
			10, 100, 1,
			1.0, 2.0, 0.004 );
    mDeclCubeSampling( cs1exp, 15, 63, 6,
			10, 100, 1,
			1.0, 2.0, 0.004 );
    mDeclCubeSampling( cs2exp, 13, 53, 4,
			10, 100, 1,
			1.0, 2.0, 0.005 );
    mDeclCubeSampling( csgeom, 1, 1, 1, 1, 1, 1, 0.006f, 4.994f, 0.004f );
    mDeclCubeSampling( csvol, 1, 1, 1, 1, 1, 1, 0.008f, 4.992f, 0.004f );
    mDeclCubeSampling( cswidevol, 1, 1, 1, 1, 1, 1, -0.116f, 5.116f, 0.004f);
    CubeSampling csvol2( csvol );
    const CubeSampling csgeomexp( csgeom ), csvolexp( csvol );

    cs1.limitTo( cs2 );
    cs2.limitTo( cs3 );
    csvol.limitTo( csgeomexp );
    cswidevol.limitTo( csgeomexp );
    csgeom.limitTo( csgeomexp );
    csvol2.limitTo( csvolexp );
    if ( cs1 != cs1exp || cs2 != cs2exp ||
	 csvol != csvolexp || cswidevol != csvolexp ||
	 csvol2 != csvolexp || csgeom != csgeomexp )
	mRetResult( "testLimitTo" );

    return true;
}


static bool testIsCompatible()
{
    mDeclCubeSampling( cs, 2, 50, 6,
			    10, 100, 9,
			    1.0, 3.0, 0.004 );
    StepInterval<float> zrgextended( cs.zsamp_ );
    StepInterval<float> zrgshrinked( cs.zsamp_ );
    StepInterval<float> zrgshifted( cs.zsamp_ );
    StepInterval<float> zrgstartneg( cs.zsamp_ );
    zrgextended.widen( zrgextended.step * 5 );
    zrgshrinked.widen( -zrgshrinked.step * 5 );
    zrgshifted.shift( zrgshifted.step * 0.01f );
    zrgstartneg.start = -1.f * zrgstartneg.start;

    const StepInterval<double> zrgd( 1.0, 3.0, 0.004 );
    StepInterval<double> zrgdextended( zrgd );
    StepInterval<double> zrgdshrinked( zrgd );
    StepInterval<double> zrgdshifted( zrgd );
    StepInterval<double> zrgdstartneg( zrgd );
    zrgdextended.widen( zrgdextended.step * 5 );
    zrgdshrinked.widen( -zrgdshrinked.step * 5 );
    zrgdshifted.shift( zrgdshifted.step * 0.01 );
    zrgdstartneg.start = -1. * zrgdstartneg.start;

    if ( !zrgextended.isCompatible(cs.zsamp_) ||
	 !zrgshrinked.isCompatible(cs.zsamp_) ||
	 zrgshifted.isCompatible(cs.zsamp_) ||
	 !zrgstartneg.isCompatible(cs.zsamp_) ||
	 !zrgstartneg.isCompatible(zrgstartneg) ||
	 !zrgdextended.isCompatible(zrgd) ||
	 !zrgdshrinked.isCompatible(zrgd) ||
	 zrgdshifted.isCompatible(zrgd) ||
	!zrgdstartneg.isCompatible(zrgd) ||
	!zrgdstartneg.isCompatible(zrgdstartneg) )
	mRetResult( "testIsCompatible()" );
}


bool testIterator()
{
    /*
    HorSampling hrg;
    hrg.survid_ = TrcKey::c3DSurvID();
    hrg.set( StepInterval<int>( 100, 102, 2 ),
	     StepInterval<int>( 300, 306, 3 ) );

    HorSamplingIterator iter( hrg );
    TypeSet<BinID> bids;
    bids += BinID(100,300);
    bids += BinID(100,303);
    bids += BinID(100,306);
    bids += BinID(102,300);
    bids += BinID(102,303);
    bids += BinID(102,306);

    int idx=0;
    do
    {
	const BinID curbid( iter.curBinID() );
	mRunStandardTest( curbid == bids[idx], "do-While loop calls" );
	idx++;
    } while ( iter.next() );

    mRunStandardTest( idx == bids.size(),
		      "All positions processed once using iterator" );

    iter.reset();
    for ( int idy=0; idy<bids.size(); idy++, iter.next() )
    {
	const BinID curbid( iter.curBinID() );
	mRunStandardTest( curbid == bids[idy], "For loop calls" );
    }

    iter.reset();
    BinID curbid = iter.curBinID();
    mRunStandardTest( curbid==bids[0], "Reset");

    iter.setCurrentPos( hrg.globalIdx(bids[4]) );
    curbid = iter.curBinID();
    mRunStandardTest( curbid==bids[4], "setCurrentPos");

    */
    return true;
}


bool testJSON()
{
    mDeclCubeSampling( cs, 2, 50, 6,
	10, 100, 9,
	1.0, 3.0, 0.004 );
    const TrcKeyZSampling tkzs( cs );
    const CubeSubSel css( cs );

    OD::JSON::Object csobj, tkzsobj, cssobj;
    cs.fillJSON( csobj );
    tkzs.fillJSON( tkzsobj );
    css.fillJSON( cssobj );

    CubeSampling sampling;
    sampling.useJSON( csobj );
    TrcKeyZSampling tkzsread;
    tkzsread.useJSON( tkzsobj );
    CubeSubSel cssread;
    cssread.useJSON( cssobj );

    if ( sampling != cs || tkzs != tkzsread )
	mRetResult( "Checking JSON" );
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    CubeSampling initcs, initworkcs;
    SI().getCubeSampling( initcs, OD::FullSurvey );
    SI().getCubeSampling( initworkcs, OD::UsrWork );

    mDeclCubeSampling( survcs, 1, 501, 2,
			    10, 100, 2,
			    1.0, 10.0, 0.004 );
    SurveyInfo& si = const_cast<SurveyInfo&>( SI() );
    si.setRanges( survcs );
    si.setWorkRanges( survcs ); //For the sanity of SI().

    if ( !testInclude()
      || !testIncludes()
      || !testEmpty()
      || !testLimitTo()
      || !testIsCompatible()
      || !testIterator()
      || !testJSON() )
	return 1;

    si.setRanges( initcs );
    si.setWorkRanges( initworkcs ); //For the sanity of SI().

    return 0;
}

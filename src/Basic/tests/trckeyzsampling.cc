/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman K Singh
 * DATE     : April 2013
 * FUNCTION : Test various functions of TrcKeySampling and TrcKeyZSampling.
-*/


#include "testprog.h"

#include "odjson.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

#define mEps 0.0001

#define mDeclTrcKeyZSampling( cs, istart, istop, istep, \
			       cstart, cstop, cstep, \
			       zstart, zstop, zstep) \
    TrcKeyZSampling cs( false ); \
    cs.hsamp_.survid_ = OD::Geom3D; \
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
    TrcKeyZSampling cs0( false );
    cs0.setEmpty();
    if ( cs0.nrInl() || cs0.nrCrl() || !cs0.isEmpty() )
	mRetResult( "testEmpty" );
}


static bool testInclude()
{
    mDeclTrcKeyZSampling( cs1, 2, 50, 6,
			    10, 100, 9,
			    1.0, 3.0, 0.004 );
    mDeclTrcKeyZSampling( cs2, 1, 101, 4,
			    4, 100, 3,
			    -1, 4.0, 0.005 );
    mDeclTrcKeyZSampling( expcs, 1, 101, 1,
			      4, 100, 3,
			      -1, 4, 0.004 );
    cs1.include ( cs2 );
    if ( cs1 != expcs )
	mRetResult( "testInclude" );
}


static bool testIncludes()
{
    mDeclTrcKeyZSampling( cs1, 2, 50, 6,
			10, 100, 9,
			1.0, 3.0, 0.004 );
    mDeclTrcKeyZSampling( cs2, 1, 101, 4,
			4, 100, 3,
			-1, 4.0, 0.005 );
    mDeclTrcKeyZSampling( cs3, 1, 101, 1,
			4, 100, 3,
			-1, 4, 0.004 );
    if ( cs2.includes(cs1) || !cs3.includes(cs1) )
	mRetResult( "testIncludes" );
}


static bool testLimitTo()
{
    mDeclTrcKeyZSampling( cs1, 3, 63, 6,
			10, 100, 1,
			1.0, 3.0, 0.004 );
    mDeclTrcKeyZSampling( cs2, 13, 69, 4,
			4, 100, 1,
			-1.0, 2.0, 0.005 );
    mDeclTrcKeyZSampling( cs3, 2, 56, 2,
			10, 100, 1,
			1.0, 2.0, 0.004 );
    mDeclTrcKeyZSampling( cs1exp, 15, 63, 6,
			10, 100, 1,
			1.0, 2.0, 0.004 );
    mDeclTrcKeyZSampling( cs2exp, 13, 53, 4,
			10, 100, 1,
			1.0, 2.0, 0.005 );
    mDeclTrcKeyZSampling( csgeom, 1, 1, 1, 1, 1, 1, 0.006f, 4.994f, 0.004f );
    mDeclTrcKeyZSampling( csvol, 1, 1, 1, 1, 1, 1, 0.008f, 4.992f, 0.004f );
    mDeclTrcKeyZSampling( cswidevol, 1, 1, 1, 1, 1, 1, -0.116f, 5.116f,0.004f );
    TrcKeyZSampling csvol2( csvol );
    const TrcKeyZSampling csgeomexp( csgeom ), csvolexp( csvol );

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
    mDeclTrcKeyZSampling( cs, 2, 50, 6,
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
    TrcKeySampling hrg;
    hrg.survid_ = OD::Geom3D;
    hrg.set( StepInterval<int>( 100, 102, 2 ),
	     StepInterval<int>( 300, 306, 3 ) );

    TrcKeySamplingIterator iter( hrg );
    TypeSet<BinID> bids;
    bids += BinID(100,300);
    bids += BinID(100,303);
    bids += BinID(100,306);
    bids += BinID(102,300);
    bids += BinID(102,303);
    bids += BinID(102,306);

    int idx=0;
    BinID curbid;
    while ( iter.next(curbid) )
	mRunStandardTest( curbid == bids[idx++], "While loop calls" );
    mRunStandardTest( idx == bids.size(),
		      "All positions processed once using iterato" );

    iter.reset(); iter.next( curbid );
    for ( int idy=0; idy<bids.size(); idy++, iter.next(curbid) )
	mRunStandardTest( curbid == bids[idy], "For loop calls" );

    iter.reset();
    mRunStandardTest( iter.next(curbid)&&curbid==bids.first(), "Reset" );

    iter.setNextPos( hrg.trcKeyAt(hrg.globalIdx(bids[4])) );
    curbid = iter.curTrcKey().position();
    mRunStandardTest( curbid==bids[4], "setCurrentPos" );

    return true;
}


bool testJSON()
{
    mDeclTrcKeyZSampling( cs, 2, 50, 6,
	10, 100, 9,
	1.0, 3.0, 0.004 );

    OD::JSON::Object obj;
    cs.fillJSON( obj );

    TrcKeyZSampling sampling;
    sampling.useJSON( obj );
    if ( cs != sampling )
	mRetResult( "Checking JSON" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    const TrcKeyZSampling initcs = SI().sampling( false );
    const TrcKeyZSampling initworkcs = SI().sampling( true );

    mDeclTrcKeyZSampling( survcs, 1, 501, 2,
			    10, 100, 2,
			    1.0, 10.0, 0.004 );
    eSI().setRange( survcs, false );
    eSI().setRange( survcs, true ); //For the sanity of SI().

    if ( !testInclude()
      || !testIncludes()
      || !testEmpty()
      || !testLimitTo()
      || !testIsCompatible()
      || !testIterator()
      || !testJSON() )
	return 1;

    eSI().setRange( initcs, false );
    eSI().setRange( initworkcs, true );

    return 0;
}

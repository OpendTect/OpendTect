/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman K Singh
 * DATE     : April 2013
 * FUNCTION : Test various functions of HorSampling and CubeSampling.
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "commandlineparser.h"
#include "cubesampling.h"
#include "keystrs.h"
#include "survinfo.h"

#include <iostream>

#define mDeclCubeSampling( cs, istart, istop, istep, \
			       cstart, cstop, cstep, \
			       zstart, zstop, zstep) \
    CubeSampling cs( false ); \
    cs.hrg.set( StepInterval<int>(istart,istop,istep), \
	        StepInterval<int>(cstart,cstop,cstep) ); \
    cs.zrg.set( zstart, zstop, zstep );


#define mRetResult( funcname ) \
    { \
	std::cerr << funcname << " failed" << std::endl; \
	return false; \
    } \
    else if ( !quiet ) \
	std::cerr << funcname << " succeeded" << std::endl; \
    return true;


static bool testEmpty( bool quiet )
{
    CubeSampling cs0( false );
    cs0.setEmpty();
    if ( cs0.nrInl() || cs0.nrCrl() || !cs0.isEmpty() )
	mRetResult( "testEmpty" ); 
}


static bool testInclude( bool quiet )
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


static bool testIncludes( bool quiet )
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


static bool testLimitTo( bool quiet )
{
    mDeclCubeSampling( cs1, 3, 63, 6,
	   		    10, 100, 1,
			    1.0, 3.0, 0.004 );
    mDeclCubeSampling( cs2, 13, 69, 4,
	   		    4, 100, 1,
			    -1, 2.0, 0.005 );
    mDeclCubeSampling( csexp, 21, 57, 12,
	   		    10, 100, 1,
			    1, 2.0, 0.005 );
    mDeclCubeSampling( cs3, 2, 56, 2,
	   		    10, 100, 1,
			    1, 2.0, 0.004 );
    cs1.limitTo( cs2 );
    cs2.limitTo( cs3 );
    if ( cs1 != csexp || !cs2.isEmpty() )
	mRetResult( "testLimitTo" );
}


int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );
    CommandLineParser parser;
    const bool quiet = parser.hasKey( sKey::Quiet() );

    mDeclCubeSampling( survcs, 1, 501, 2,
	   		    10, 100, 2,
			    1.0, 10.0, 0.004 );
    eSI().setRange( survcs, false );
    eSI().setRange( survcs, true ); //For the sanity of SI().

    if ( !testInclude(quiet) )
	ExitProgram( 1 );
    
    if ( !testIncludes(quiet) )
	ExitProgram( 1 );
    
    if ( !testEmpty(quiet) )
	ExitProgram( 1 );
    
    if ( !testLimitTo(quiet) )
	ExitProgram( 1 );
    
    ExitProgram( 0 );
}

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION : 
-*/

static const char* rcsID mUsedVar = "$Id: testsvnversion.cc 27628 2012-11-25 16:04:42Z kristofer.tingdahl@dgbes.com $";

#include "typeset.h"
#include "objectset.h"

#include <iostream>

#define mErrRet( msg ) \
{ \
	std::cerr << msg << " failed.\n"; \
	return 1; \
}


int main( int narg, char** argv )
{
    SetProgramArgs( narg, argv );

    TypeSet<int> ts1;
    ts1 += 1; ts1 += 2; ts1 += 1;

    if ( ts1.count(2)!=1 )
	mErrRet("ts1.count(2)!" );

    if ( ts1.indexOf( 1, true, -1 ) != 0 )
	mErrRet("ts1.indexOf( 1, true, -1 )");

    if ( ts1.indexOf( 1, true, 1 ) != 2 )
	mErrRet("ts1.indexOf( 1, true, 1 )");

    if ( ts1.indexOf( 1, false, -1 ) != 2 )
	mErrRet("ts1.indexOf( 1, false, -1 )");

    if ( ts1.indexOf( 1, false, 1 ) != 0 )
	mErrRet("ts1.indexOf( 1, false, 1 )");
    
    if ( !ts1.isPresent( 1 ) )
	mErrRet("!ts1.isPresent( 1 )" );
    
    if ( ts1.isPresent( 5 ) )
	mErrRet("ts1.isPresent( 5 )");

    return 0;
}

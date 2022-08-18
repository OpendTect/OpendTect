/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "nrbytes2string.h"
#include "testprog.h"
#include "globexpr.h"



int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( quiet_ || argc != 3 )
	return 0;

    BufferString tomatch( argv[1] );
    GlobExpr ge( argv[2] );

    od_cout() << "'" << argv[1]
	<< (ge.matches(argv[1]) ? "' matches '" : "' doesn't match '")
	<< argv[2] << "'" << od_endl;

    return 0;
}

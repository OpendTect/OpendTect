/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
-*/

#include "genc.h"
#include "testprog.h"


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    logStream() << "Exiting with return code 1" << od_endl;
    return 1;
}


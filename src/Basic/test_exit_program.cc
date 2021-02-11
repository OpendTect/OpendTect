/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

#include "prog.h"


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::TestProgCtxt );

    logStream() << "Exiting with return code 1" << od_endl;
    return 1;
}

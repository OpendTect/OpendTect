
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A. Huck
 * DATE     : Jun 2020
 * FUNCTION :
-*/


#include "testprog.h"

#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "odjson.h"
#include "timefun.h"


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    FilePath fp( __FILE__ );
    fp.setExtension( "par" );
    if ( !File::exists(fp.fullPath()) )
    {
	errStream() << "Input file not found\n";
	return 1;
    }

    IOPar inpar;
    inpar.read( fp.fullPath(), "Parameters" );
    mRunStandardTest( inpar.size()==285, "Read IOPar from file" );

    OD::JSON::Object jsonobj;
    inpar.fillJSON( jsonobj, false );

    IOPar outpar( inpar.name() );
    outpar.useJSON( jsonobj );
    mRunStandardTest( outpar.isEqual(inpar), "Retrieve IOPar from JSON" );

    return 0;
}

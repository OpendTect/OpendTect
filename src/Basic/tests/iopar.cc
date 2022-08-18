/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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

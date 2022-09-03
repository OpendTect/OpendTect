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

    IOPar testpar;
    testpar.add( "Key1", "Value1" );
    testpar.add( "Key2", "Value2" );
    testpar.add( "Key3", "Value3" );
    testpar.add( "Key5", "Value5" );

    const BufferString strvw1 = testpar.find( "Key1" );
    const BufferString strvw2 = testpar.find( "Key2" );
    const BufferString strvw3 = testpar.find( "Key3" );
    const BufferString strvw4 = testpar.find( "Key4" );
    mRunStandardTest( strvw4.isEmpty(),
				    "Not assigned key returns empty string" );
    const BufferString strvw5 = testpar.find( "Key5" );
    mRunStandardTest( ((!strvw1.isEqual(strvw2)) ||
		      (!strvw2.isEqual(strvw3)) || (!strvw3.isEqual(strvw5))),
				    "Retrieved string from IOPar correct" );
    mRunStandardTest( ((strvw1.buf() != strvw2.buf()) ||
	(strvw1.buf() != strvw2.buf()) || (strvw1.buf() != strvw2.buf())),
	"Retrieved string do not point to same location" );

    return 0;
}

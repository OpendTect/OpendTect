/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Nov 2013
-*/


#include "webstreamsource.h"
#include "testprog.h"
#include "applicationdata.h"
#include "od_istream.h"


static const char* smallfname = "http://intranet/testing/ctest/test_file";

bool testReadSmallFile()
{
    StreamData sd; sd.setFileName( smallfname );
    WebStreamSource wss;
    tstStream() << "Trying to open: " << sd.fileName() << od_endl;
    wss.fill( sd, StreamProvider::StreamSource::Read );

    if ( !sd.usable() )
    {
	tstStream(true) << "Cannot open: " << sd.fileName() << od_endl;
	return false;
    }

    od_istream strm( *sd.istrm );
    if ( !strm.isOK() )
    {
	tstStream(true) << "Stream not OK: " << sd.fileName() << od_endl;
	return false;
    }

    char c = strm.peek();
    if ( c != 'X' )
	strm.ignore( 2 );
    BufferString rest;
    strm.getWord( rest, false );
    if ( rest != "wnload.opendtect.org" )
    {
	tstStream(true) << "Unexpected read from: " << sd.fileName() << od_endl;
	return false;
    }

    tstStream() << "File seems OK: " << sd.fileName() << od_endl;
    sd.close();
    return true;
}

#include "odnetworkaccess.h"

int main(int argc, char** argv)
{
    mInitTestProg();
    ApplicationData app; // needed for QEventLoop

    bool res = true;

    if ( res && !testReadSmallFile() )
	res = false;

    ExitProgram( res ? 0 : 1 );
}

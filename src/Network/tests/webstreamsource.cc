/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Nov 2013
-*/


#include "webstreamsource.h"
#include "testprog.h"
#include "applicationdata.h"
#include "od_iostream.h"


static const char* smallfname = "http://intranet/testing/ctest/test_file";
/*
static const char* bigfname = "http://opendtect.org/tmp/f3_fake_2d_ps_seis.zip";
static od_stream::Count bigsz = 10556039;
*/

static bool testReadSmallFile()
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


/*
static bool testReadBigFile()
{
    StreamData sd; sd.setFileName( bigfname );
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

    od_ostream out( "/tmp/out.zip" );
    char buf[100000];
    od_stream::Count totsz = 0;
    while ( true )
    {
	od_stream::Count newsz = 100000;
	if ( totsz + newsz > bigsz )
	    newsz = bigsz - totsz;
	if ( newsz < 1 )
	    break;

	if ( !strm.getBin(buf,newsz) )
	{
	    tstStream(true) << "File too small: " << sd.fileName() << od_endl;
	    return false;
	}
	out.addBin( buf, newsz );
	totsz += newsz;
    }

    strm.setPosition( 123453 );
    char c = strm.peek(); // 'Q'
    strm.setPosition( 1234560 );
    c = strm.peek(); // 'Z'

    sd.close();
    return true;
}
*/


#include "odnetworkaccess.h"

int main(int argc, char** argv)
{
    mInitTestProg();
    ApplicationData app; // needed for QEventLoop

    bool res = true;

    if ( res && !testReadSmallFile() )
	res = false;

/*
    if ( res && !testReadBigFile() )
	res = false;
*/

    ExitProgram( res ? 0 : 1 );
}

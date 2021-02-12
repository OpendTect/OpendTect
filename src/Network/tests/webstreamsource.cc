/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Nov 2013
-*/


#include "testprog.h"

#include "applicationdata.h"
#include "file.h"
#include "filepath.h"
#include "od_iostream.h"
#include "moddepmgr.h"


static const char* smallfname = "http://intranet/testing/ctest/test_file";
static const char* bigfname = "http://opendtect.org/tmp/f3_fake_2d_ps_seis.zip";
static od_stream::Count bigsz = 10556039;

static bool testReadSmallFile()
{
    od_istream strm( smallfname );
    if ( !strm.isOK() )
    {
	tstStream(true) << "Stream not OK: " << smallfname << od_endl;
	return false;
    }

    char c = strm.peek();
    if ( c != 'X' )
	strm.ignore( 2 );
    BufferString rest;
    strm.getWord( rest, false );
    if ( rest != "wnload.opendtect.org" )
    {
	tstStream(true) << "Unexpected read from: " << smallfname << od_endl;
	return false;
    }

    tstStream() << "File seems OK: " << smallfname << od_endl;
    return true;
}


static bool testReadBigFile()
{
    od_istream strm( bigfname );
    mRunStandardTest( strm.isOK(), BufferString(bigfname," stream is OK"));

    const BufferString fnm = File::Path::getTempFullPath( "webstream", "zip" );
    od_ostream out( fnm );
    if ( !out.isOK() )
	return false;

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
	    tstStream(true) << "File too small: " << bigfname << od_endl;
	    return false;
	}
	out.addBin( buf, newsz );
	totsz += newsz;
    }

    strm.setReadPosition( 123453 );
    char c = strm.peek(); // 'Q'
    mRunStandardTest( c=='Q', "Value at pre-defined position 1" );
    strm.setReadPosition( 1234560 );
    c = strm.peek(); // 'Z'
    mRunStandardTest( c=='Z', "Value at pre-defined position 2" );

    out.close();
    File::remove( fnm );

    return true;
}

bool testIsLocalFlag()
{
    od_istream dnstrm( "/dev/null" );
    mRunStandardTest( dnstrm.isLocal(), "Stream should be local" );

    od_istream wbstrm( "http://opendtect.org/dlsites.txt" );
    mRunStandardTest( !wbstrm.isLocal(), "Stream should not be local" );

    return true;
}

int mTestMainFnName(int argc, char** argv)
{
    mInitTestProg();
    ApplicationData app; // needed for QEventLoop
    OD::ModDeps().ensureLoaded( "Network" ); //Init factories

    if ( !testReadSmallFile() || !testIsLocalFlag() )
	return 1;

    if ( !testReadBigFile() )
	return 1;

    return 0;
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: oddlsite.cc,v 1.5 2011-11-23 11:35:55 cvsbert Exp $";

#include "oddlsite.h"
#include "odhttp.h"
#include "databuf.h"
#include "file.h"
#include "filepath.h"
#include "httptask.h"
#include "settings.h"
#include "thread.h"
#include "timefun.h"

static const char* sKeyTimeOut = "Download.Timout";


ODDLSite::ODDLSite( const char* h, float t )
    : odhttp_(*new ODHttp)
    , timeout_(t)
    , host_(h)
    , databuf_(0)
{
    if ( timeout_ <= 0 )
    {
	Settings::common().get( sKeyTimeOut, timeout_ );
	if ( timeout_ <= 0 )
	    timeout_ = 5; // internet is quick these days
    }

    if ( host_.isEmpty() )
	host_ = "opendtect.org";
    odhttp_.requestFinished.notify( mCB(this,ODDLSite,reqFinish) );
    reConnect();
}


ODDLSite::~ODDLSite()
{
    delete databuf_;
    delete &odhttp_;
}


void ODDLSite::setTimeOut( float t, bool sett )
{
    timeout_ = t;
    if ( sett )
    {
	Settings::common().set( sKeyTimeOut, timeout_ );
	Settings::common().write();
    }
}


bool ODDLSite::reConnect()
{
    if ( odhttp_.state() == ODHttp::Unconnected )
	odhttp_.setHost( host_ );

    Time::Counter tc; tc.start();
    while ( odhttp_.state() < ODHttp::Sending )
    {
	Threads::sleep( 0.1 );
	if ( tc.elapsed() > 1000 * timeout_ )
	{
	    errmsg_ = "Cannot open connection to ";
	    errmsg_.add( host_ ).add ( ":\n" );
	    if ( odhttp_.state() == ODHttp::HostLookup )
		errmsg_.add ( "Host name lookup timeout" );
	    if ( odhttp_.state() == ODHttp::Connecting )
		errmsg_.add ( "Host doesn't respond" );
	    else
		errmsg_.add ( "Internet connection not available" );
	    isfailed_ = true;
	    return false;
	}
    }
    isfailed_ = false;
    return true;
}


bool ODDLSite::getFile( const char* relfnm, const char* outfnm )
{
    delete databuf_; databuf_ = 0;

    HttpTask task( odhttp_ );
    const int reqid = odhttp_.get( getFileName(relfnm), outfnm );
    // TODO reqid can be used in reqFinish.
    if ( !task.execute() )
	return false;

    if ( outfnm && *outfnm )
    {
	if ( !File::exists(outfnm) )
	{
	    errmsg_ = BufferString( "No output file: ", outfnm );
	    return false;
	}

	return true;
    }

    const od_int64 nrbytes = odhttp_.bytesAvailable();
    databuf_ = new DataBuffer( nrbytes, 1, true );
    const char* buffer = odhttp_.readCharBuffer();
    memcpy( databuf_->data(), buffer, nrbytes );
    return true;
}


DataBuffer* ODDLSite::obtainResultBuf()
{
    DataBuffer* ret = databuf_;
    databuf_ = 0;
    return ret;
}


bool ODDLSite::getFiles( const BufferStringSet& fnms, const char* outputdir,
			 TaskRunner& tr )
{
    HttpTask task( odhttp_ );
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	FilePath outputfp( outputdir );
	outputfp.add( fnms.get(idx) );
	odhttp_.get( getFileName(fnms.get(idx)), outputfp.fullPath() );
	// TODO get() returns requestid. Can be used in reqFinish.
    }

    return tr.execute( task );
}


void ODDLSite::reqFinish( CallBacker* )
{
    //TODO implement
}


BufferString ODDLSite::getFileName( const char* relfnm ) const
{
    BufferString ret( "/" );
    if ( subdir_.isEmpty() )
	ret.add( relfnm );
    else
	ret.add( FilePath( subdir_ ).add( relfnm ).fullPath(FilePath::Unix) );
    return ret;
}


BufferString ODDLSite::fullURL( const char* relfnm ) const
{
    BufferString ret( "http://" );
    ret.add( host_ ).add( "/" ).add( getFileName(relfnm) );
    return ret;
}

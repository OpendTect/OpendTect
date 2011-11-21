/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: oddlsite.cc,v 1.2 2011-11-21 13:25:04 cvsbert Exp $";

#include "oddlsite.h"
#include "odhttp.h"
#include "timefun.h"
#include "databuf.h"
#include "filepath.h"
#include "thread.h"
#include "settings.h"

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
		errmsg_.add ( ":\nHost name lookup timeout" );
	    if ( odhttp_.state() == ODHttp::Connecting )
		errmsg_.add ( ":\nHost doesn't respond" );
	    else
		errmsg_.add ( ":\nInternet connection not available" );
	    return false;
	}
    }
    return true;
}


bool ODDLSite::getFile( const char* relfnm, const char* outfnm )
{
    delete databuf_; databuf_ = 0;

    //TODO
    // if outfnm non-null and not empty, open it for write
    // otherwise, fill the buffer

    errmsg_ = "TODO: getFile not implemented yet";
    return false;
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
    errmsg_ = "TODO: getFiles not implemented yet";
    return false;
}


void ODDLSite::reqFinish( CallBacker* )
{
    //TODO implement
}


BufferString ODDLSite::getFileName( const char* relfnm ) const
{
    BufferString ret;
    if ( subdir_.isEmpty() )
	ret = relfnm;
    else
	ret = FilePath( subdir_ ).add( relfnm ).fullPath();
    return ret;
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/

#include "oddlsite.h"
#include "odnetworkaccess.h"
#include "databuf.h"
#include "file.h"
#include "filepath.h"
#include "od_istream.h"
#include "settings.h"
#include "uistrings.h"

static const char* sKeyTimeOut = "Download.Timout";

ODDLSite::ODDLSite( const char* h, float t )
    : timeout_(t)
    , databuf_(0)
    , host_(h)
    , isfailed_(false)
    , issecure_(false)
{
    islocal_ = host_.startsWith( "DIR=" );
    if ( host_.isEmpty() )
	host_ = "opendtect.org";
    int stroffs = 0;
    if ( islocal_ )
	stroffs = 4;
    else if ( host_.startsWith("http://") )
	stroffs = 7;
    else if ( host_.startsWith("https://") )
	{ stroffs = 8; issecure_ = true; }
    host_ = h + stroffs;

    // TODO handle timeout
    if ( timeout_ <= 0 )
    {
	Settings::common().get( sKeyTimeOut, timeout_ );
	if ( timeout_ <= 0 )
	    timeout_ = 5; // internet is quick these days
    }

    if ( host_.isEmpty() )
	host_ = "opendtect.org";

}


ODDLSite::~ODDLSite()
{
    delete databuf_;
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


bool ODDLSite::getFile( const char* relfnm, const char* outfnm,
			    TaskRunner* taskrunner,
			    const char* nicename )
{
    delete databuf_; databuf_ = 0;

    if ( islocal_ )
	return getLocalFile( relfnm, outfnm );

    if ( !outfnm )
    {
	databuf_ = new DataBuffer( 0, 1, true );
	return Network::downloadToBuffer(fullURL(relfnm),databuf_,errmsg_,
					 taskrunner);
    }

    return Network::downloadFile( fullURL(relfnm), outfnm, errmsg_, taskrunner);
}


bool ODDLSite::getLocalFile( const char* relfnm, const char* outfnm )
{
    const BufferString inpfnm( getFileName(relfnm) );

    if ( outfnm && *outfnm )
	return File::copy( inpfnm, outfnm );

    od_istream strm( inpfnm );
    if ( !strm.isOK() )
	{ errmsg_ = uiStrings::phrCannotOpenForRead( inpfnm ); return false; }

    BufferString bs;
    const bool isok = strm.getAll( bs );
    if ( isok )
    {
	databuf_ = new DataBuffer( bs.size(), 1 );
	OD::memCopy( (char*)databuf_->data(), bs.buf(), databuf_->size() );
    }
    return isok;
}


DataBuffer* ODDLSite::obtainResultBuf()
{
    DataBuffer* ret = databuf_;
    databuf_ = 0;
    return ret;
}


bool ODDLSite::getFiles( const BufferStringSet& fnms, const char* outputdir,
			 TaskRunner& taskrunner )
{
    errmsg_.setEmpty();
    BufferStringSet fullurls;
    for ( int idx=0; idx<fnms.size(); idx++ )
	fullurls.add( fullURL(fnms.get(idx)) );

    return Network::downloadFiles( fullurls, outputdir, errmsg_, &taskrunner );
}


od_int64 ODDLSite::getFileSize( const char* relfilenm )
{
    od_int64 ret = 0;
    const BufferString fullurl = fullURL( relfilenm );
    Network::getRemoteFileSize( fullurl.buf(), ret, errmsg_ );
    return ret;
}


void ODDLSite::reqFinish( CallBacker* )
{
    //TODO implement
}


BufferString ODDLSite::getFileName( const char* relfnm ) const
{
    if ( islocal_ )
	return FilePath( host_, subdir_, relfnm ).fullPath();

    BufferString ret( "/" );
    if ( subdir_.isEmpty() )
	ret.add( relfnm );
    else
	ret.add( FilePath( subdir_, relfnm ).fullPath(FilePath::Unix) );
    return ret;
}


BufferString ODDLSite::fullURL( const char* relfnm ) const
{
    if ( islocal_ )
	return getFileName( relfnm );

    BufferString ret( issecure_ ? "https://" : "http://" );
    ret.add( host_ ).add( getFileName(relfnm) );
    return ret;
}

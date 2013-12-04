/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "oddlsite.h"
#include "odnetworkaccess.h"
#include "databuf.h"
#include "file.h"
#include "filepath.h"
#include "strmoper.h"
#include "strmprov.h"
#include "settings.h"
#include "timefun.h"

static const char* sKeyTimeOut = "Download.Timout";

ODDLSite::ODDLSite( const char* h, float t )
    : timeout_(t)
    , databuf_(0)
    , islocal_(h && matchString("DIR=",h))
    , isfailed_(false)
    , issecure_(false)
{
    if ( !h ) h = "opendtect.org";
    int stroffs = 0;
    if ( islocal_ )
	stroffs = 4;
    else if ( matchString("http://",h) )
	stroffs = 7;
    else if ( matchString("https://",h) )
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


bool ODDLSite::getFile( const char* relfnm, const char* outfnm, TaskRunner* tr,
			    const char* nicename )
{
    delete databuf_; databuf_ = 0;

    if ( islocal_ )
	return getLocalFile( relfnm, outfnm );

    if ( !outfnm )
    {
	databuf_ = new DataBuffer( 0, 1, true );
	return Network::downloadToBuffer(fullURL(relfnm),databuf_,errmsg_,tr);
    }

    return Network::downloadFile( fullURL(relfnm), outfnm, errmsg_, tr );
}


bool ODDLSite::getLocalFile( const char* relfnm, const char* outfnm )
{
    const BufferString inpfnm( getFileName(relfnm) );

    if ( outfnm && *outfnm )
	return File::copy( inpfnm, outfnm );

    StreamData sd( StreamProvider(inpfnm).makeIStream() );
    if ( !sd.usable() )
	{ errmsg_ = "Cannot open "; errmsg_ += inpfnm; return false; }
    BufferString bs;
    const bool isok = StrmOper::readFile( *sd.istrm, bs );
    sd.close();
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
			 TaskRunner& tr )
{
    errmsg_.setEmpty();
    BufferStringSet fullurls;
    for ( int idx=0; idx<fnms.size(); idx++ )
	fullurls.add( fullURL(fnms.get(idx)) );

    return Network::downloadFiles( fullurls, outputdir, errmsg_, &tr );
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

    BufferString ret( "http://" );
    ret.add( host_ ).add( getFileName(relfnm) );
    return ret;
}

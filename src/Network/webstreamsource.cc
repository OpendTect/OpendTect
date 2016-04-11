/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "webstreamsource.h"
#include "netfilecache.h"
#include "odnetworkaccess.h"
#ifndef OD_NO_QT
# include <QUrl>
#endif


namespace Network
{

class FileDownloadMgr : public FileCache
{
public:

    			FileDownloadMgr(const char* fnm);

    bool		goTo(FilePosType);

    const QUrl		url_;
    FilePosType		curpos_;
    uiString		errmsg_;

private:

    od_int64		getSz(const char*);
    bool		fillBlock(BlockIdxType);
    bool		fillSingle(FileChunkType);
    bool		fillMulti(FileChunkSetType);

};

} // namespace Network


Network::FileDownloadMgr::FileDownloadMgr( const char* fnm )
    : FileCache(getSz(fnm))
    , url_(fnm)
    , curpos_(0)
{
    //TODO if ( url_.isBad() ...
}


od_int64 Network::FileDownloadMgr::getSz( const char* fnm )
{
    od_int64 sz = 0;
    uiString emsg; //TODO how do we get this into errmsg_?
    return getRemoteFileSize( fnm, sz, emsg ) ? sz : 0;
}


bool Network::FileDownloadMgr::goTo( FilePosType pos )
{
    BlockIdxType bidx = blockIdx( pos );
    if ( !hasBlock(bidx) && !fillBlock( bidx ) )
	return false;
    curpos_ = pos;
    return true;
}


bool Network::FileDownloadMgr::fillBlock( BlockIdxType bidx )
{
    FileChunkType pintv( blockStart(bidx), 0 );
    pintv.stop = pintv.start + blockSize(bidx) - 1;
    return fillSingle( pintv );
}


bool Network::FileDownloadMgr::fillSingle( FileChunkType pintv )
{
    //TODO
    return false;
}


bool Network::FileDownloadMgr::fillMulti( FileChunkSetType pintvs )
{
    const FileChunkSetType::size_type nrintv = pintvs.size();
    if ( nrintv < 2 )
	return nrintv == 1 ? fillSingle( pintvs[0] ) : true;

    //TODO
    return false;
}


// webstreambuf

// WebStreamSource

void WebStreamSource::initClass()
{
    StreamProvider::addStreamSource( new WebStreamSource );
}


bool WebStreamSource::willHandle( const char* fnm )
{
    if ( !fnm || !*fnm )
	return false;
    mSkipBlanks( fnm );
    const FixedString url( fnm );
#define mUrlStartsWith(s) url.startsWith( s, CaseInsensitive )
    return mUrlStartsWith( "http://" ) || mUrlStartsWith( "https://" )
	|| mUrlStartsWith( "ftp://" );
}


bool WebStreamSource::canHandle( const char* fnm ) const
{
    return willHandle( fnm );
}


WebStreamSource::WebStreamSource()
{
}

bool WebStreamSource::fill( StreamData& sd, StreamSource::Type typ ) const
{
    /*
    if ( typ == Read )
    {
	std::webstreambuf* wsb = new std::webstreambuf( sd.fileName(), true );
	if ( !wsb->isBad() )
	    sd.istrm = new std::istream( wsb );
	else
	    { delete wsb; wsb = 0; }
    }
    else if ( typ == Write )
    {
	std::webstreambuf* wsb = new std::webstreambuf( sd.fileName(), false );
	if ( !wsb->isBad() )
	    sd.ostrm = new std::ostream( wsb );
	else
	    { delete wsb; wsb = 0; }
    }
    */

    return sd.usable();
}

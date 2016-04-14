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
#include "odnetworkreply.h"
#include "file.h"
#include <streambuf>
#ifndef OD_NO_QT
# include <QEventLoop>
# include <QNetworkAccessManager>
# include <QNetworkReply>
# include <QNetworkRequest>
# include <QUrl>
#endif


namespace Network
{

class FileDownloadMgr : public FileCache
{
public:

			FileDownloadMgr(const char* fnm);

    bool		goTo(FilePosType&,BlockIdxType&);

    uiString		errmsg_;

    bool		fill(const FileChunkSetType&);

private:

#ifndef OD_NO_QT
    const QUrl		url_;
#endif


    od_int64		getSz(const char*);
    bool		fillBlock(BlockIdxType);
    bool		fillSingle(FileChunkType);

};

} // namespace Network


Network::FileDownloadMgr::FileDownloadMgr( const char* fnm )
    : FileCache(getSz(fnm))
    , url_(fnm)
{
}


od_int64 Network::FileDownloadMgr::getSz( const char* fnm )
{
    od_int64 sz = 0;
    uiString emsg; //TODO how do we get this into errmsg_?
    return getRemoteFileSize( fnm, sz, emsg ) ? sz : 0;
}


bool Network::FileDownloadMgr::goTo( FilePosType& pos, BlockIdxType& bidx )
{
    bidx = blockIdx( pos );
    if ( pos >= size() )
	pos = size();

    if ( !isLiveBlock(bidx) && !fillBlock( bidx ) )
	return false;

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
    if ( size() < 1 )
	return false;

    FileChunkSetType pintvs;
    pintvs += pintv;
    return fill( pintvs );
}


bool Network::FileDownloadMgr::fill( const FileChunkSetType& pintvs )
{
    const FileChunkSetType::size_type nrintv = pintvs.size();
    if ( nrintv < 1 )
	return true;

#ifndef OD_NO_QT

    QEventLoop qevloop;
    ObjectSet<ODNetworkReply> replies;
    ChunkSizeType maxsz = 0;
    for ( int ichunk=0; ichunk<nrintv; ichunk++ )
    {
	const FileChunkType pintv = pintvs[ichunk];
	const ChunkSizeType intvsz = (ChunkSizeType)pintv.width() + 1;
	if ( intvsz > maxsz )
	    maxsz = intvsz;

	QNetworkRequest qnr( url_ );
	qnr.setAttribute( QNetworkRequest::HttpPipeliningAllowedAttribute,
			  true );
	BufferString hdrstr( "bytes=", pintv.start, "-" );
	hdrstr.add( pintv.stop );
	qnr.setRawHeader( "Range", hdrstr.str() );

	replies += new ODNetworkReply( ODNA().get(qnr), &qevloop );
    }

    bool haveerr = false;
    while ( true )
    {
	qevloop.exec();
	bool allfinished = true;
	for ( int ichunk=0; ichunk<nrintv; ichunk++ )
	{
	    const QNetworkReply& reply = *replies[ichunk]->qNetworkReply();
	    if ( reply.error() != QNetworkReply::NoError )
		{ haveerr = true; break; }
	    else if ( !reply.isFinished() )
		{ allfinished = false; break; }
	}
	if ( haveerr || allfinished )
	    break;
    }
    if ( haveerr )
	return false;

    char* databuf = new char[ maxsz ];
    for ( int ichunk=0; ichunk<nrintv; ichunk++ )
    {
	QNetworkReply& reply = *replies[ichunk]->qNetworkReply();
	ChunkSizeType nrbytes = (ChunkSizeType)reply.bytesAvailable();
	FileChunkType pintv = pintvs[ichunk];
	const int maxnrbytes = pintv.width() + 1;
	if ( nrbytes == 0 )
	    { pErrMsg("Reply finished but no bytes available" ); continue; }

	if ( nrbytes != maxnrbytes )
	{
	    if ( nrbytes > maxnrbytes )
	    {
		pErrMsg("Reply nr bytes > requested. May be normal." );
		nrbytes = maxnrbytes;
	    }
	    else
	    {
		pErrMsg("Reply nr bytes < requested" );
	        pintv.stop = pintv.start + nrbytes - 1;
	    }
	}

	reply.read( databuf, nrbytes );
	setData( pintv, (BufType*)databuf );
    }
    delete [] databuf;

#endif // !OD_NO_QT

    return true;
}


// webstreambufs

namespace std
{

/*!\brief Adapter to use web services to access files */

class webistreambuf : public streambuf
{
public:

    typedef Network::FileCache		FileCache;
    typedef FileCache::FileSizeType	FileSizeType;
    typedef FileCache::FilePosType	FilePosType;
    typedef FileCache::BufType		BufType;
    typedef FileCache::BlockIdxType	BlockIdxType;
    typedef FileCache::BlockSizeType	BlockSizeType;

webistreambuf( const char* url )
    : mgr_(url)
    , curbidx_(-1)
{
    setGPtrs( 0, false );
}

~webistreambuf()
{
}

inline FilePosType curPos() const
{
    return curbidx_ < 0 ? 0 : (mgr_.blockStart( curbidx_ ) + gptr() - eback());
}

void setGPtrs( FilePosType newpos, bool isvalid )
{
    if ( !isvalid )
	setg( 0, 0, 0 );
    else
    {
	char* cbuf = (char*)mgr_.getBlock( curbidx_ );
	setg( cbuf,
	      cbuf + (newpos - mgr_.blockStart(curbidx_) ),
	      cbuf + mgr_.blockSize(curbidx_) );
    }
}

bool goTo( FilePosType newpos )
{
    if ( newpos < 0 )
	newpos = 0;
    if ( newpos >= mgr_.size() )
	newpos = mgr_.size();

    bool isvalid = mgr_.goTo(newpos,curbidx_) && mgr_.validBlockIdx(curbidx_);
    setGPtrs( newpos, isvalid );
    return isvalid;
}

virtual pos_type seekoff( off_type offs, ios_base::seekdir sd,
			  ios_base::openmode which )
{
    pos_type newpos = offs;
    if ( sd == ios_base::cur )
	newpos = curPos() + offs;
    else if ( sd == ios_base::end )
	newpos = mgr_.size() - offs;

    return seekpos( newpos, which );
}

virtual pos_type seekpos( pos_type newpos, ios_base::openmode )
{
    if ( goTo(newpos) )
	return curPos();
    return -1;
}

virtual int underflow()
{
    char* curgptr = gptr();
    if ( !curgptr )
    {
	if ( curbidx_ < 0 )
	    curbidx_ = -1;
	else if ( !mgr_.validBlockIdx(curbidx_) )
	    return char_traits<char>::eof();
    }
    else if ( curgptr < egptr() )
    {
	// underflow should not have been called?
	return (int)*curgptr;
    }

    int newbidx = curbidx_ + 1;
    if ( !mgr_.validBlockIdx(newbidx) || !goTo(mgr_.blockStart(newbidx)) )
    {
	setGPtrs( 0 , false );
	return char_traits<char>::eof();
    }

    return (int)(*gptr());
}

virtual streamsize xsgetn( char_type* buftofill, streamsize nrbytes )
{
    if ( nrbytes < 1 )
	return nrbytes;

    mgr_.setMinCacheSize( nrbytes );

    FilePosType curpos = curPos();
    FileCache::FileChunkSetType pintvs = mgr_.stillNeededDataFor( curpos,
								  nrbytes );
    if ( !mgr_.fill(pintvs) )
	return 0;

    nrbytes = mgr_.getAt( curpos, (BufType*)buftofill, nrbytes );
    curpos += nrbytes;
    curbidx_ = mgr_.blockIdx( curpos );
    setGPtrs( curpos, true );
    return nrbytes;
}

    Network::FileDownloadMgr	mgr_;
    BlockIdxType		curbidx_;

};


// webstreams


/*!\brief Adapter to use web services to access files */

class webistream : public istream
{
public:

webistream( webistreambuf* sb )
    : istream(sb)
{}

~webistream()
{ delete rdbuf(); }

};

} // namespace std


// WebStreamSource

void WebStreamSource::initClass()
{ StreamProvider::addStreamSource( new WebStreamSource ); }
bool WebStreamSource::willHandle( const char* fnm )
{ return File::isURI( fnm ); }
bool WebStreamSource::canHandle( const char* fnm ) const
{ return willHandle( fnm ); }


bool WebStreamSource::fill( StreamData& sd, StreamSource::Type typ ) const
{
    if ( typ == Read )
	sd.istrm = new std::webistream( new std::webistreambuf(sd.fileName()) );
    /*
    else if ( typ == Write )
	sd.ostrm = new std::webostream( new std::webostreambuf(sd.fileName()) );
    */

    return sd.usable();
}

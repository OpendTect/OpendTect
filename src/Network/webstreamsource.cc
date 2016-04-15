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

class FileDownloadMgr : public ReadCache
{
public:

			FileDownloadMgr(const char* fnm);

    bool		goTo(FilePosType&,BlockIdxType&);
    bool		fill(const FileChunkSetType&);

    uiString		errmsg_;

private:

#ifndef OD_NO_QT
    const QUrl		url_;
#endif
    typedef ObjectSet<ODNetworkReply> ReplySet;


    od_int64		getSz(const char*);
    bool		fillBlock(BlockIdxType);
    bool		fillSingle(FileChunkType);
    ChunkSizeType	getReplies(FileChunkSetType&,ReplySet&,QEventLoop&);
    bool		waitForFinish(ReplySet&,QEventLoop&);
    void		getDataFromReplies(const FileChunkSetType&,ReplySet&,
					   ChunkSizeType);

};

} // namespace Network


Network::FileDownloadMgr::FileDownloadMgr( const char* fnm )
    : ReadCache(getSz(fnm))
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
    FileChunkType chunk( blockStart(bidx), 0 );
    chunk.stop = chunk.start + blockSize(bidx) - 1;
    return fillSingle( chunk );
}


bool Network::FileDownloadMgr::fillSingle( FileChunkType chunk )
{
    if ( size() < 1 )
	return false;

    FileChunkSetType chunks;
    chunks += chunk;
    return fill( chunks );
}


bool Network::FileDownloadMgr::fill( const FileChunkSetType& reqchunks )
{
    if ( reqchunks.size() < 1 )
	return true;

#ifndef OD_NO_QT

    FileChunkSetType chunks = reqchunks; ReplySet replies;
    QEventLoop qevloop;
    const ChunkSizeType maxsz = getReplies( chunks, replies, qevloop );
    if ( maxsz < 1 )
	return true;
    else if ( !waitForFinish(replies,qevloop) )
	return false;

    getDataFromReplies( chunks, replies, maxsz );

#endif // !OD_NO_QT

    return true;
}


Network::FileDownloadMgr::ChunkSizeType Network::FileDownloadMgr::getReplies(
	FileChunkSetType& chunks, ReplySet& replies, QEventLoop& qevloop )
{
    ChunkSizeType maxsz = 0;

#ifndef OD_NO_QT
    for ( int ichunk=0; ichunk<chunks.size(); ichunk++ )
    {
	const FileChunkType chunk = chunks[ichunk];
	const ChunkSizeType intvsz = (ChunkSizeType)chunk.width() + 1;
	if ( intvsz < 1 )
	    { chunks.removeSingle(ichunk); ichunk--; continue; }
	else if ( intvsz > maxsz )
	    maxsz = intvsz;

	QNetworkRequest qnr( url_ );
	qnr.setAttribute( QNetworkRequest::HttpPipeliningAllowedAttribute,
			  true );
	BufferString hdrstr( "bytes=", chunk.start, "-" );
	hdrstr.add( chunk.stop );
	qnr.setRawHeader( "Range", hdrstr.str() );

	replies += new ODNetworkReply( ODNA().get(qnr), &qevloop );
    }
#endif // !OD_NO_QT

    return maxsz;
}


bool Network::FileDownloadMgr::waitForFinish( ReplySet& replies,
					      QEventLoop& qevloop )
{
    bool haveerr = false;

#ifndef OD_NO_QT
    const int nrreplies = replies.size();
    while ( true )
    {
	qevloop.exec();
	bool allfinished = true;
	for ( int ireply=0; ireply<nrreplies; ireply++ )
	{
	    QNetworkReply& reply = *replies[ireply]->qNetworkReply();
	    if ( reply.error() != QNetworkReply::NoError )
		{ haveerr = true; break; }
	    else if ( !reply.isFinished() )
		{ allfinished = false; break; }
	}
	if ( haveerr || allfinished )
	    break;
    }
#endif // !OD_NO_QT

    return !haveerr;
}


void Network::FileDownloadMgr::getDataFromReplies(
	const FileChunkSetType& chunks, ReplySet& replies, ChunkSizeType maxsz )
{
#ifndef OD_NO_QT
    char* databuf = new char[ maxsz ];
    for ( int ichunk=0; ichunk<chunks.size(); ichunk++ )
    {
	QNetworkReply& reply = *replies[ichunk]->qNetworkReply();
	ChunkSizeType nrbytes = (ChunkSizeType)reply.bytesAvailable();
	FileChunkType chunk = chunks[ichunk];
	const int maxnrbytes = chunk.width() + 1;
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
	        chunk.stop = chunk.start + nrbytes - 1;
	    }
	}

	reply.read( databuf, nrbytes );
	setData( chunk, (BufType*)databuf );
    }
    delete [] databuf;
#endif // !OD_NO_QT
}


// webstreambufs

namespace Network
{

/*!\brief Adapter to use web services to access files */

class webistreambuf : public std::streambuf
{
public:

    typedef ReadCache::FileSizeType	FileSizeType;
    typedef ReadCache::FilePosType	FilePosType;
    typedef ReadCache::BufType		BufType;
    typedef ReadCache::BlockIdxType	BlockIdxType;
    typedef ReadCache::BlockSizeType	BlockSizeType;

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

virtual std::ios::pos_type seekoff( std::ios::off_type offs,
				    std::ios_base::seekdir sd,
				    std::ios_base::openmode which )
{
    std::ios::pos_type newpos = offs;
    if ( sd == std::ios_base::cur )
	newpos = curPos() + offs;
    else if ( sd == std::ios_base::end )
	newpos = mgr_.size() - offs;

    return seekpos( newpos, which );
}

virtual std::ios::pos_type seekpos( std::ios::pos_type newpos,
				    std::ios_base::openmode )
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
	    return std::char_traits<char>::eof();
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
	return std::char_traits<char>::eof();
    }

    return (int)(*gptr());
}

virtual std::streamsize xsgetn( std::ios::char_type* buftofill,
				std::streamsize nrbytes )
{
    if ( nrbytes < 1 )
	return nrbytes;

    mgr_.setMinCacheSize( nrbytes );

    FilePosType curpos = curPos();
    ReadCache::FileChunkSetType chunks = mgr_.stillNeededDataFor( curpos,
								  nrbytes );
    if ( !mgr_.fill(chunks) )
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

class webistream : public std::istream
{
public:

webistream( webistreambuf* sb )
    : std::istream(sb)
{}

~webistream()
{ delete rdbuf(); }

};

} // namespace Network


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
	sd.istrm = new Network::webistream(
			new Network::webistreambuf(sd.fileName()) );
    /*
    else if ( typ == Write )
	sd.ostrm = new Network::webostream(
			new Network::webostreambuf(sd.fileName()) );
    */

    return sd.usable();
}

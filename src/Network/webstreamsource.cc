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
#include "odhttp.h"
#include "file.h"
#include <streambuf>
#ifndef OD_NO_QT
# include <QEventLoop>
# include <QNetworkAccessManager>
# include <QNetworkReply>
# include <QNetworkRequest>
# include <QUrl>
#endif

#include <iostream>


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

    const BufferString	url_;

    typedef RefObjectSet<Network::HttpRequestProcess> ReplySet;


    od_int64		getSz(const char*);
    bool		fillBlock(BlockIdxType);
    bool		fillSingle(FileChunkType);
    ChunkSizeType	getReplies(FileChunkSetType&,ReplySet&,QEventLoop&);
    bool		waitForFinish(ReplySet&,QEventLoop&);
    void		getDataFromReplies(const FileChunkSetType&,ReplySet&,
					   ChunkSizeType);

};

class FileUploadMgr : public WriteCache
{
public:

			FileUploadMgr(const char* fnm);

    uiString		errmsg_;

private:

#ifndef OD_NO_QT
    const QUrl		url_;
#endif
    typedef ObjectSet<ODNetworkProcess> ReplySet;

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

	Network::HttpRequest req( url_ );
	BufferString hdrstr( "bytes=", chunk.start, "-" );
	hdrstr.add( chunk.stop );
	req.setRawHeader( "Range", hdrstr.str() );

	replies += Network::HttpRequestManager::instance().get(req);
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
	    Network::HttpRequestProcess& reply = *replies[ireply];
	    if ( reply.isError() )
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
	Network::HttpRequestProcess& reply = *replies[ichunk];
	ChunkSizeType nrbytes = (ChunkSizeType)reply.downloadBytesAvailable();
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


Network::FileUploadMgr::FileUploadMgr( const char* fnm )
    : url_(fnm)
{
}


// webstreambufs

namespace Network
{

/*!\brief base streambuf class for the web stream bufs */

class webstreambuf : public std::streambuf
{
public:

    typedef FileCache::FileSizeType	FileSizeType;
    typedef FileCache::FilePosType	FilePosType;
    typedef FileCache::BufType		BufType;
    typedef FileCache::BlockIdxType	BlockIdxType;
    typedef FileCache::BlockSizeType	BlockSizeType;

webstreambuf()
    : curbidx_(-1)
{
}

virtual std::ios::pos_type seekoff( std::ios::off_type offs,
				    std::ios_base::seekdir sd,
				    std::ios_base::openmode which )
{
    std::ios::pos_type newpos = offs;
    if ( sd == std::ios_base::cur )
	newpos = curPos() + offs;
    else if ( sd == std::ios_base::end )
	newpos = cache().size() - offs;

    return seekpos( newpos, which );
}

virtual std::ios::pos_type seekpos( std::ios::pos_type newpos,
				    std::ios_base::openmode )
{
    if ( goTo(newpos) )
	return curPos();
    return -1;
}

inline int eofVal() const
{
    return std::char_traits<char>::eof();
}

    BlockIdxType		curbidx_;

    virtual FileCache&		cache()			= 0;
    virtual FilePosType		curPos() const		= 0;
    virtual bool		goTo(FilePosType)	= 0;

};


/*!\brief std::streambuf to access files on web servers */

class webistreambuf : public webstreambuf
{
public:

webistreambuf( const char* url )
    : mgr_(url)
{
    setGPtrs( 0, false );
}

~webistreambuf()
{
    // keep for easier debugging
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

virtual FilePosType curPos() const
{
    return curbidx_ < 0 ? 0 : (mgr_.blockStart( curbidx_ ) + gptr() - eback());
}

virtual bool goTo( FilePosType newpos )
{
    if ( newpos < 0 )
	newpos = 0;
    if ( mgr_.size() > 0 && newpos >= mgr_.size() )
	newpos = mgr_.size();

    bool isvalid = mgr_.goTo(newpos,curbidx_) && mgr_.validBlockIdx(curbidx_);
    setGPtrs( newpos, isvalid );
    return isvalid;
}

virtual int underflow()
{
    char* curgptr = gptr();
    if ( !curgptr )
    {
	if ( curbidx_ < 0 )
	    curbidx_ = -1;
	else if ( !mgr_.validBlockIdx(curbidx_) )
	    return eofVal();
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
	return eofVal();
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
    virtual FileCache&		cache()			{ return mgr_; }

};

/*!\brief std::streambuf to access files on web servers */

class webostreambuf : public webstreambuf
{
public:

webostreambuf( const char* url )
    : mgr_(url)
{
    setPPtrs( 0, false );
}

~webostreambuf()
{
    // keep for easier debugging
}

void setPPtrs( FilePosType newpos, bool isvalid )
{
    if ( !isvalid )
	setp( 0, 0 );
    else
    {
	char* cbuf = (char*)mgr_.getBlock( curbidx_ );
	setp( cbuf, cbuf + mgr_.blockSize(curbidx_) );
	const FilePosType blockstart = mgr_.blockStart( curbidx_ );
	if ( newpos > blockstart )
	    pbump( newpos - blockstart );
    }
}

virtual FilePosType curPos() const
{
    return curbidx_ < 0 ? 0 : (mgr_.blockStart( curbidx_ ) + pptr() - pbase());
}

virtual bool goTo( FilePosType newpos )
{
    if ( newpos < 0 )
	newpos = 0;
    //TODO
    return false;
}

virtual int overflow( int toput )
{
    char* curpptr = pptr();
    if ( !curpptr )
    {
	if ( curbidx_ < 0 )
	    curbidx_ = -1;
	else if ( !mgr_.validBlockIdx(curbidx_) )
	{
	    //TODO add block
	    return eofVal();
	}
    }
    else if ( curpptr < epptr() )
    {
	// overflow should not have been called?
	*curpptr = (BufType)toput;
	return toput;
    }

    int newbidx = curbidx_ + 1;
    if ( !mgr_.validBlockIdx(newbidx) || !goTo(mgr_.blockStart(newbidx)) )
    {
	setPPtrs( 0, false );
	return eofVal();
    }

    *pptr() = (BufType)toput;
    return toput;
}

virtual std::streamsize xsputn( const std::ios::char_type* buftoput,
				std::streamsize nrbytes )
{
    if ( nrbytes < 1 )
	return nrbytes;

    //TODO
    return 0;
}

    Network::FileUploadMgr	mgr_;
    virtual FileCache&		cache()			{ return mgr_; }

};


// webstreams

/*!\brief Adapter using web services to access files as streams */

class webistream : public std::istream
{
public:

webistream( webistreambuf* sb )
    : std::istream(sb)
{}

~webistream()
{ delete rdbuf(); }

};


/*!\brief Adapter using web services to write files as streams */

class webostream : public std::ostream
{
public:

webostream( webostreambuf* sb )
    : std::ostream(sb)
{}

~webostream()
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
    else if ( typ == Write )
	sd.ostrm = new Network::webostream(
			new Network::webostreambuf(sd.fileName()) );

    return sd.usable();
}

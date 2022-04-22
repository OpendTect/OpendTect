/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/


#include "filesystemaccess.h"
#include "netfilecache.h"
#include "odnetworkaccess.h"
#include "odhttp.h"
#include "genc.h"
#include "timefun.h"
#include "uistrings.h"

#include <streambuf>

#ifndef OD_NO_QT
# include <QNetworkAccessManager>
# include <QNetworkReply>
# include <QNetworkRequest>
# include <QUrl>
#endif

#include <iostream>

static const int remote_file_exist_cache_time = 10000; // 10 seconds


namespace Network
{

class FileDownloadMgr : public ReadCache
{
public:

			FileDownloadMgr(const char* fnm);

    bool		goTo(FilePosType&,block_idx_type&);
    bool		fill(const FileChunkSetType&);

    uiString		errmsg_;

private:

    const BufferString	url_;

    typedef RefObjectSet<Network::HttpRequestProcess> ReplySet;


    od_int64		getSz(const char*);
    bool		fillBlock(block_idx_type);
    bool		fillSingle(FileChunkType);
    ChunkSizeType	getReplies(FileChunkSetType&,ReplySet&);
    bool		waitForFinish(ReplySet&);
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


bool Network::FileDownloadMgr::goTo( FilePosType& pos, block_idx_type& bidx )
{
    bidx = blockIdx( pos );
    if ( pos >= size() )
	pos = size();

    if ( !isLiveBlock(bidx) && !fillBlock( bidx ) )
	return false;

    return true;
}


bool Network::FileDownloadMgr::fillBlock( block_idx_type bidx )
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
    const ChunkSizeType maxsz = getReplies( chunks, replies );
    if ( maxsz < 1 )
	return true;
    else if ( !waitForFinish(replies) )
	return false;

    getDataFromReplies( chunks, replies, maxsz );

#endif // !OD_NO_QT

    return true;
}


Network::FileDownloadMgr::ChunkSizeType Network::FileDownloadMgr::getReplies(
	FileChunkSetType& chunks, ReplySet& replies )
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

	RefMan<Network::HttpRequest> req =
	    new Network::HttpRequest( url_, Network::HttpRequest::Get );
	BufferString hdrstr( "bytes=", chunk.start, "-" );
	hdrstr.add( chunk.stop );
	req->setRawHeader( "Range", hdrstr.str() );

	replies += Network::HttpRequestManager::instance().request(req);
    }
#endif // !OD_NO_QT

    return maxsz;
}


bool Network::FileDownloadMgr::waitForFinish( ReplySet& replies )
{
    bool haveerr = false;

#ifndef OD_NO_QT
    const int nrreplies = replies.size();
    while ( true )
    {
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

    typedef FileCache::file_size_type	file_size_type;
    typedef FileCache::FilePosType	FilePosType;
    typedef FileCache::BufType		BufType;
    typedef FileCache::block_idx_type	block_idx_type;
    typedef FileCache::BlockSizeType	BlockSizeType;

webstreambuf()
    : curbidx_(-1)
{
}

std::ios::pos_type seekoff( std::ios::off_type offs,
				    std::ios_base::seekdir sd,
				    std::ios_base::openmode which ) override
{
    std::ios::pos_type newpos = offs;
    if ( sd == std::ios_base::cur )
	newpos = curPos() + offs;
    else if ( sd == std::ios_base::end )
	newpos = cache().size() - offs;

    return seekpos( newpos, which );
}

std::ios::pos_type seekpos( std::ios::pos_type newpos,
				    std::ios_base::openmode ) override
{
    if ( goTo(newpos) )
	return curPos();
    return -1;
}

inline int eofVal() const
{
    return std::char_traits<char>::eof();
}

    block_idx_type		curbidx_;

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

FilePosType curPos() const override
{
    return curbidx_ < 0 ? 0 : (mgr_.blockStart( curbidx_ ) + gptr() - eback());
}

bool goTo( FilePosType newpos ) override
{
    if ( newpos < 0 )
	newpos = 0;
    if ( mgr_.size() > 0 && newpos >= mgr_.size() )
	newpos = mgr_.size();

    bool isvalid = mgr_.goTo(newpos,curbidx_) && mgr_.validBlockIdx(curbidx_);
    setGPtrs( newpos, isvalid );
    return isvalid;
}

int underflow() override
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

std::streamsize xsgetn( std::ios::char_type* buftofill,
				std::streamsize nrbytes ) override
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
    FileCache&			cache() override	{ return mgr_; }

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

FilePosType curPos() const override
{
    return curbidx_ < 0 ? 0 : (mgr_.blockStart( curbidx_ ) + pptr() - pbase());
}

bool goTo( FilePosType newpos ) override
{
    if ( newpos < 0 )
	newpos = 0;
    //TODO
    return false;
}

int overflow( int toput ) override
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

std::streamsize xsputn( const std::ios::char_type* buftoput,
				std::streamsize nrbytes ) override
{
    if ( nrbytes < 1 )
	return nrbytes;

    //TODO
    return 0;
}

    Network::FileUploadMgr	mgr_;
    FileCache&			cache() override	{ return mgr_; }

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


class HttpFileSystemAccess : public OD::FileSystemAccess
{ mODTextTranslationClass(HttpFileSystemAccess)
public:

    static const char*	sFactoryKeyword() { return "http"; }
    static uiString	sFactoryUserName() { return uiStrings::sWeb(); }

    const char*		protocol() const override { return sFactoryKeyword(); }
    uiString		userName() const override { return sFactoryUserName(); }
    bool		readingSupported() const override	{ return true; }
    bool		writingSupported() const override
			    { return false; }
    bool		queriesSupported() const override
			    { return false; }
    bool		operationsSupported() const override
			    { return false; }

    bool		isReadable(const char*) const override;
    od_int64		getFileSize(const char*,bool) const override;
    StreamData		createIStream(const char*,bool) const override;
    StreamData		createOStream(const char*,bool,bool) const override;

    static void		initClass();
    static OD::FileSystemAccess* createInstance()
			    { return new HttpFileSystemAccess; }

    mutable std::map<std::string,int>	existcache_;

};

} // namespace Network


bool Network::HttpFileSystemAccess::isReadable( const char* uri ) const
{
    if ( !uri || !*uri )
	return false;

    // Find entry and clean up old entries in one go
    const std::string uristr( uri );
    const int curmsecs = Time::getMilliSeconds();
    bool okfromcache = false;
    for ( auto it=existcache_.cbegin(); it!=existcache_.cend(); )
    {
	const int tdiff = curmsecs - it->second;
	if ( tdiff > remote_file_exist_cache_time )
	    it = existcache_.erase( it );
	else
	{
	    if ( it->first == uristr )
		okfromcache = true;
	    ++it;
	}
    }

    if ( okfromcache )
	return true;

    if ( !Network::exists(uri) )
	return false;

    existcache_[uristr] = curmsecs;
    return true;
}


od_int64 Network::HttpFileSystemAccess::getFileSize( const char* uri,
						     bool ) const
{
    return Network::getFileSize( uri );
}


StreamData Network::HttpFileSystemAccess::createIStream( const char* fnm,
							 bool binary ) const
{
    StreamData sd;
    StreamData::StreamDataImpl* impl = new StreamData::StreamDataImpl;
    impl->fname_ = fnm;
    impl->istrm_ = new Network::webistream( new Network::webistreambuf( fnm ) );
    if ( !impl->istrm_->good() )
	deleteAndZeroPtr( impl->istrm_ );

    sd.setImpl( impl );
    return sd;
}


StreamData Network::HttpFileSystemAccess::createOStream( const char* fnm,
				      bool binary,bool editmode) const
{
    StreamData sd;
    if ( editmode )
	return sd;

    StreamData::StreamDataImpl* impl = new StreamData::StreamDataImpl;
    impl->fname_ = fnm;
    impl->ostrm_ = new Network::webostream( new Network::webostreambuf( fnm ) );
    if ( !impl->ostrm_->good() )
	deleteAndZeroPtr( impl->ostrm_ );

    sd.setImpl( impl );
    return sd;
}


void Network::HttpFileSystemAccess::initClass()
{
    OD::FileSystemAccess::factory().addCreator( createInstance,
					      sFactoryKeyword(),
					      sFactoryUserName() );
}

void NetworkHttpFileSystemAccessinitClass()
{
    Network::HttpFileSystemAccess::initClass();
}


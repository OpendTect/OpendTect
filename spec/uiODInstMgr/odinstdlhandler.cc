/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: odinstdlhandler.cc 7930 2013-05-31 11:40:13Z ranojay.sen@dgbes.com $";

#include "odinstdlhandler.h"
#include "odinstlogger.h"
#include "oddlsite.h"
#include "databuf.h"


const BufferStringSet& ODInst::DLHandler::seedSites()
{
    static const char* nms[] =
    {
	"opendtect.org",
	"opendtect.s3.amazonaws.com",
	0
    };
    static BufferStringSet* ret = 0;
    if ( !ret ) ret = new BufferStringSet( nms );
    return *ret;
}



ODInst::DLHandler::DLHandler( const DataBuffer& dbuf, const char* subdir,
			      float timeout )
    : dlsite_(0)
    , databuf_(0)
    , failhndlr_(0)
    , lastremotestatus_(0)
{
    const int buflen = dbuf.size();
    char* str = new char [buflen + 1];
    memcpy( str, dbuf.data(), buflen );
    str[ buflen ] = '\0';

    char* startptr = str;
    while ( startptr && *startptr )
    {
	char* ptr = strchr( startptr, '\n' );
	if ( ptr ) *ptr++ = '\0';
	mTrimBlanks(startptr);
	if ( *startptr )
	    avsites_.add( startptr );
	startptr = ptr;
    }
    delete [] str;

    if ( !avsites_.isEmpty() )
    {
	dlsite_ = new ODDLSite( avsites_.get(0), timeout );
	dlsite_->setSubDir( subdir&&*subdir ? subdir : "relman" );
    }
}


ODInst::DLHandler::~DLHandler()
{
    delete databuf_;
    delete dlsite_;
}


BufferString ODInst::DLHandler::fullURL( const char* reldirnm ) const
{
    return dlsite_->fullURL( reldirnm );
}


void ODInst::DLHandler::setFailHandler( ODInst::DLHandler::FailHndlr* fh )
{
    failhndlr_ = fh;
}


void ODInst::DLHandler::setSite( const char* st, float tmout )
{
    if ( !st || !*st )
	st = dlsite_->host();

    const BufferString newsite( st );
    if ( newsite == dlsite_->host() )
    {
	if ( tmout > 0 && tmout != dlsite_->timeout() )
	    dlsite_->setTimeOut( tmout, true );
	return;
    }

    delete dlsite_;
    dlsite_ = 0;
    dlsite_ = new ODDLSite( newsite, tmout );
    const BufferString subdir( dlsite_->subDir() );
    dlsite_->setSubDir( !subdir.isEmpty() ? subdir : "relman" );
    reConnectSite();
}


DataBuffer* ODInst::DLHandler::getSites( const BufferStringSet& seedsites,
		ODInst::DLHandler::FailHndlr& failhndlr, float& tmout )
{
    BufferString seedsite( seedsites.get(0) );
    DataBuffer* dbuf = 0; failhndlr.isfatal_ = true;
    int counter = 0;
    while ( counter++ < 1000 )
    {
	mODInstLog() << "Seed site: " << seedsite << std::endl;
	ODDLSite dlsite( seedsite, tmout );
	bool isok = dlsite.isOK();
	if ( isok )
	    isok = dlsite.getFile( "dlsites.txt" );
	if ( !isok )
	    failhndlr.handle( dlsite, seedsite, tmout );
	else
	{
	    mODInstToLog( "Seed site OK, got Download sites" );
	    tmout = dlsite.timeout();
	    dbuf = dlsite.obtainResultBuf();
	    break;
	}
    }
    return dbuf;
}


bool ODInst::DLHandler::remoteStatusOK( bool chkprev )
{
    if ( !fetchFile("status") )
	return false;

    const DataBuffer& dbuf = fileData();
    char* res = new char [ dbuf.size() + 1 ];
    memcpy( res, dbuf.data(), dbuf.size() );
    res[dbuf.size()] = '\0';
    const int prevrs = lastremotestatus_;
    lastremotestatus_ = toInt( res );
    delete [] res;
    if ( lastremotestatus_ < 1 )
	return false;

    return !chkprev || lastremotestatus_ == prevrs;
}


bool ODInst::DLHandler::fetchFile( const char* fnm, const char* outfnm,
				    TaskRunner* tr, const char* nicename )
{
    delete databuf_; databuf_ = 0;
    mODInstLog() << "Downloading file " << fnm << " into "
		<< (outfnm && *outfnm ? outfnm : "buffer.") << std::endl;
    while ( !dlsite_->getFile(fnm,outfnm,tr,nicename) )
    {
	mODInstLog() << "Failed to download " << fnm << std::endl;
	if ( !useFailHandler() )
	    return false;
    }
    return true;
}


bool ODInst::DLHandler::fetchFiles( const BufferStringSet& fnms,
			const char* outdirnm, TaskRunner& tr )
{
    if ( fnms.isEmpty() )
	return true;

    mODInstLog() << "Downloading:\n";
    for ( int idx=0; idx<fnms.size(); idx++ )
	mODInstLog() << '\t' << fnms.get(idx) << std::endl;

    while ( !dlsite_->getFiles(fnms,outdirnm,tr) )
    {
	mODInstToLog( "Failed or user stop" );
	if ( !useFailHandler() )
	    return false;
    }

    return true;
}


bool ODInst::DLHandler::useFailHandler()
{
    if ( !failhndlr_ )
	{ mODInstToLog( "No fail handler, giving up" ); return false; }

    BufferString newsite; float tmout = dlsite_->timeout();
    if ( !failhndlr_->handle( *dlsite_, newsite, tmout ) )
	{ mODInstToLog( "Giving up on user request" ); return false; }

    mODInstLog() << "Trying again using: " << newsite
		    << " ; timeout " << tmout << std::endl;
    setSite( newsite, tmout );
    return true;
}


const DataBuffer& ODInst::DLHandler::fileData() const
{
    if ( !databuf_ )
    {
	databuf_ = dlsite_->obtainResultBuf();
	if ( !databuf_ )
	    databuf_ = new DataBuffer( 0, 1, false );
    }
    return *databuf_;
}


void ODInst::DLHandler::getFileData( const DataBuffer& dbuf,
				     BufferStringSet& bss, int maxlen )
{
    if ( dbuf.isEmpty() )
	return;

    char* bufstartptr = (char*)dbuf.data();
    char* bufstopptr = bufstartptr + dbuf.size();
    char* str = new char [maxlen+1]; *str = '\0';
    char* strptr = str; char* bufptr = bufstartptr;
    char* strstopptr = str+maxlen;
    while ( bufptr != bufstopptr )
    {
	if ( *bufptr == '\n' )
	    { *strptr = '\0'; bss.add( str ); strptr = str; }
	else if ( strptr < strstopptr )
	    *strptr++ = *bufptr;

	bufptr++;
	if ( bufptr >= bufstopptr )
	    { *strptr = '\0'; bss.add( str ); }
    }
    delete [] str;
}


bool ODInst::DLHandler::isOK() const
{
    return dlsite_ ? dlsite_->isOK() : false;
}


const char* ODInst::DLHandler::errMsg() const
{
    return dlsite_ ? dlsite_->errMsg() : "No connection";
}


bool ODInst::DLHandler::reConnectSite()
{
    return dlsite_->reConnect();
}

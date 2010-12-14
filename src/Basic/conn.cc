/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Connections
-*/

static const char* rcsID = "$Id: conn.cc,v 1.37 2010-12-14 15:53:16 cvsbert Exp $";

#include "streamconn.h"
#include "strmprov.h"
#include "strmoper.h"
#include "oddirs.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "timefun.h"
#include "errh.h"
#include <iostream>
#include <fstream>


bool ErrMsgClass::printProgrammerErrs =
# ifdef __debug__
    true;
# else
    false;
# endif

static BufferString logmsgfnm;

mBasicGlobal const char* logMsgFileName()
{
    return logmsgfnm.buf();
}


mBasicGlobal int gLogFilesRedirectCode = -1;
// Not set. 0 = stderr, 1 = log file

#define mErrRet(s) \
    { \
	strm = &std::cerr; \
	*strm << "Cannot open file for log messages:\n\t" << s << std::endl; \
	return *strm; \
    }

mBasicGlobal std::ostream& logMsgStrm()
{
    if ( gLogFilesRedirectCode < 1 )
	return std::cerr;

    static std::ostream* strm = 0;
    if ( strm ) return *strm;

    if ( GetEnvVarYN("OD_LOG_STDERR") )
	{ strm = &std::cerr; return *strm; }

    const char* basedd = GetBaseDataDir();
    if ( !File::isDirectory(basedd) )
	mErrRet( "Directory for data storage is invalid" )

    FilePath fp( basedd );
    fp.add( "LogFiles" );
    const BufferString dirnm = fp.fullPath();
    if ( !File::exists(dirnm) )
	File::createDir( dirnm );
    if ( !File::isDirectory(dirnm) )
	mErrRet( "Cannot create proper directory for log file" )

    const FilePath pfp( GetPersonalDir() );
    BufferString fnm( pfp.fileName() );
    const char* odusr = GetSoftwareUser();
    if ( odusr && *odusr )
	{ fnm += "_"; fnm += odusr; }
    BufferString datestr = Time::getDateTimeString();
    replaceCharacter( datestr.buf(), ' ', '-' );
    replaceCharacter( datestr.buf(), ':', '.' );
    fnm += "_"; fnm += datestr.buf();
    fnm += ".txt";

    fp.add( fnm );
    logmsgfnm = fp.fullPath();
    StreamData sd = StreamProvider( logmsgfnm ).makeOStream( false );
    if ( !sd.usable() )
    {
	BufferString msg( "Cannot create log file '" );
	msg += logmsgfnm; msg += "'";
	logmsgfnm = "";
	mErrRet( msg );
    }

    strm = sd.ostrm;
    return *strm;
}


void UsrMsg( const char* msg, MsgClass::Type t )
{
    if ( !MsgClass::theCB().willCall() )
	logMsgStrm() << msg << std::endl;
    else
    {
	MsgClass obj( msg, t );
	MsgClass::theCB().doCall( &obj );
    }
}


void ErrMsg( const char* msg, bool progr )
{
    if ( !ErrMsgClass::printProgrammerErrs && progr ) return;

    if ( !MsgClass::theCB().willCall() )
    {
	if ( progr )
	    std::cerr << "(PE) " << msg << std::endl;
	else if ( msg && *msg )
	{
	    const char* start = *msg == '[' ? "" : "Err: ";
	    logMsgStrm() << start << msg << std::endl;
	}
    }
    else
    {
	ErrMsgClass obj( msg, progr );
	MsgClass::theCB().doCall( &obj );
    }
}


CallBack& MsgClass::theCB( const CallBack* cb )
{
    static CallBack thecb;
    if ( cb ) thecb = *cb;
    return thecb;
}


const char* MsgClass::nameOf( MsgClass::Type typ )
{
    static const char* strs[] =
    	{ "Information", "Message", "Warning", "Error", "PE", 0 };
    return strs[ (int)typ ];
}


StreamConn::StreamConn()
	: mine_(true)
	, state_(Bad)
	, closeondel_(false)
{
}


StreamConn::StreamConn( std::istream* s )
	: mine_(true)
	, state_(Read)
	, closeondel_(false)
{
    sd_.istrm = s;
    (void)bad();
}


StreamConn::StreamConn( std::ostream* s )
	: mine_(true)
	, state_(Write)
	, closeondel_(false)
{
    sd_.ostrm = s;
    (void)bad();
}


StreamConn::StreamConn( StreamData& strmdta )
	: mine_(true)
	, closeondel_(false)
{
    strmdta.transferTo(sd_);

    if		( !sd_.usable() )	state_ = Bad;
    else if	( sd_.istrm )		state_ = Read;
    else if	( sd_.ostrm )		state_ = Write;
}


StreamConn::StreamConn( std::istream& s, bool cod )
	: mine_(false)
	, state_(Read)
	, closeondel_(cod)
{
    sd_.istrm = &s;
    (void)bad();
}


StreamConn::StreamConn( std::ostream& s, bool cod )
	: mine_(false)
	, state_(Write)
	, closeondel_(cod)
{
    sd_.ostrm = &s;
    (void)bad();
}


StreamConn::StreamConn( const char* nm, State s )
	: mine_(true)
	, state_(s)
	, closeondel_(false)
{
    switch ( state_ )
    {
    case Read:
	if ( !nm || !*nm ) sd_.istrm = &std::cin;
	else
	{
	    StreamProvider sp( nm );
	    sd_ = sp.makeIStream();
	}
    break;
    case Write:
	if ( !nm || !*nm ) sd_.ostrm = &std::cout;
	else
	{
	    StreamProvider sp( nm );
	    sd_ = sp.makeOStream();
	}
    break;
    default:
    break;
    }

    (void)bad();
}


StreamConn::~StreamConn()
{
    close();
}


bool StreamConn::bad() const
{
    if ( state_ == Bad )
	return true;
    else if ( !sd_.usable() )
	const_cast<StreamConn*>(this)->state_ = Bad;
    return state_ == Bad;
}


void StreamConn::clearErr()
{
    if ( forWrite() ) { oStream().flush(); oStream().clear(); }
    if ( forRead() ) iStream().clear();
}


void StreamConn::close()
{
    if ( mine_ )
	sd_.close();
    else if ( closeondel_ )
    {
	if ( state_ == Read && sd_.istrm && sd_.istrm != &std::cin )
	{
	    mDynamicCastGet(std::ifstream*,s,sd_.istrm)
	    if ( s ) s->close();
	}
	else if ( state_ == Write && sd_.ostrm
	       && sd_.ostrm != &std::cout && sd_.ostrm != &std::cerr )
	{
	    mDynamicCastGet(std::ofstream*,s,sd_.ostrm)
	    if ( s ) s->close();
	}
    }
}


bool StreamConn::doIO( void* ptr, unsigned int nrbytes )
{
    if ( forWrite() )
	return StrmOper::writeBlock( oStream(), ptr, nrbytes );

    return StrmOper::readBlock( iStream(), ptr, nrbytes );
}

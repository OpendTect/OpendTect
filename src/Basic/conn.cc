/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Connections
-*/

static const char* rcsID = "$Id: conn.cc,v 1.18 2004-04-27 15:51:15 bert Exp $";

#include "errh.h"
#include "strmprov.h"
#include "strmoper.h"
#include <iostream>
#include <fstream>

bool ErrMsgClass::printProgrammerErrs =
# ifdef __debug__
    true;
# else
    false;
# endif
UsrIoMsg* UsrIoMsg::theUsrIoMsg_ = 0;

DefineEnumNames(MsgClass,Type,1,"Message type")
	{ "Information", "Message", "Warning", "Error", "PE", 0 };

void UsrMsg( const char* msg, MsgClass::Type t )
{
    if ( !MsgClass::theCB().willCall() )
	std::cerr << msg << std::endl;
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
	std::cerr << (progr?"(PE) ":"") << msg << std::endl;
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


DefineEnumNames(StreamConn,Type,0,"Type")
	{ "File", "Device", "Command", 0 };

const char* XConn::sType = "X-Group";
const char* StreamConn::sType = "Stream";


StreamConn::StreamConn()
	: mine(true)
	, nrretries(0)
	, retrydelay(0)
	, state_(Bad)
	, closeondel(false)
{
}


StreamConn::StreamConn( std::istream* s )
	: mine(true)
	, nrretries(0)
	, retrydelay(0)
	, state_(Read)
	, closeondel(false)
{
    sd.istrm = s;
    (void)bad();
}


StreamConn::StreamConn( std::ostream* s )
	: mine(true)
	, nrretries(0)
	, retrydelay(0)
	, state_(Write)
	, closeondel(false)
{
    sd.ostrm = s;
    (void)bad();
}


StreamConn::StreamConn( StreamData& strmdta )
	: mine(true)
	, nrretries(0)
	, retrydelay(0)
	, closeondel(false)
{
    strmdta.transferTo(sd);

    if		( !sd.usable() )	state_ = Bad;
    else if	( sd.istrm )		state_ = Read;
    else if	( sd.ostrm )		state_ = Write;
}


StreamConn::StreamConn( std::istream& s, bool cod )
	: mine(false)
	, nrretries(0)
	, retrydelay(0)
	, state_(Read)
	, closeondel(cod)
{
    sd.istrm = &s;
    (void)bad();
}


StreamConn::StreamConn( std::ostream& s, bool cod )
	: mine(false)
	, nrretries(0)
	, retrydelay(0)
	, state_(Write)
	, closeondel(cod)
{
    sd.ostrm = &s;
    (void)bad();
}


StreamConn::StreamConn( const char* nm, State s )
	: mine(true)
	, nrretries(0)
	, retrydelay(0)
	, state_(s)
	, closeondel(false)
{
    switch ( state_ )
    {
    case Read:
	if ( !nm || !*nm ) sd.istrm = &std::cin;
	else
	{
	    StreamProvider sp( nm );
	    sd = sp.makeIStream();
	}
    break;
    case Write:
	if ( !nm || !*nm ) sd.ostrm = &std::cout;
	else
	{
	    StreamProvider sp( nm );
	    sd = sp.makeOStream();
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
    else if ( !sd.usable() )
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
    if ( mine )
	sd.close();
    else if ( closeondel )
    {
	if ( state_ == Read && sd.istrm && sd.istrm != &std::cin )
	{
	    mDynamicCastGet(std::ifstream*,s,sd.istrm)
	    if ( s ) s->close();
	}
	else if ( state_ == Write && sd.ostrm
	       && sd.ostrm != &std::cout && sd.ostrm != &std::cerr )
	{
	    mDynamicCastGet(std::ofstream*,s,sd.ostrm)
	    if ( s ) s->close();
	}
    }
}


bool StreamConn::doIO( void* ptr, unsigned int nrbytes )
{
    if ( forWrite()
      && !writeWithRetry( oStream(), ptr, nrbytes, nrRetries(), retryDelay() ) )
	return false;

    if ( forRead() )
	return readWithRetry( iStream(), ptr,nrbytes,nrRetries(),retryDelay() );

    return true;
}

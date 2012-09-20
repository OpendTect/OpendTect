/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Connections
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "streamconn.h"
#include "strmprov.h"
#include "strmoper.h"
#include <iostream>
#include <fstream>


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

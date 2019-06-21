/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer / Bert
 Date:		Aug 2014
________________________________________________________________________

-*/

#include "netreqpacket.h"

#include "atomic.h"
#include "datainterp.h"
#include "odjson.h"
#include "ptrman.h"
#include "settings.h"
#include "string.h"


static Threads::Atomic<int> curreqid_;

static const char* sKeySizeLimit = "Network.Limit Packet Size";

od_int32 Network::RequestPacket::systemSizeLimit()
{
    const char* res = Settings::common().find( sKeySizeLimit );
    return toInt( res );
}

void Network::RequestPacket::setSystemSizeLimit( od_int32 val )
{
    const int curval = systemSizeLimit();
    if ( val < 0 ) val = 0;
    if ( curval == val )
	return;

    Settings& setts = Settings::common();
    setts.set( sKeySizeLimit, val );
    setts.write();
}


#define mRequestID_	(header_.int32s_[1])
#define mPayloadSize_	(header_.int32s_[0])
#define mSubID_		(header_.int16s_[4])


Network::RequestPacket::RequestPacket( const Network::RequestPacket& b )
    : header_( b.header_ )
    , payload_( 0 )
{
    const od_int32 payloadsize = b.payloadSize();
    if ( payloadsize )
    {
	void* newpayload = allocPayload( payloadsize );
	memcpy( newpayload, b.payload(), payloadsize );
	setPayload( newpayload, payloadsize );
    }
}



Network::RequestPacket::RequestPacket( od_int32 payloadsize )
    : payload_( 0 )
{
    setPayload( allocPayload(payloadsize), payloadsize );
    setSubID( cMoreSubID() );
    setRequestID( -1 );
}


Network::RequestPacket::~RequestPacket()
{ deleteAndZeroArrPtr( payload_ ); }


bool Network::RequestPacket::isOK() const
{
    return payloadSize()>=0 && requestID()>=0;
}


od_int32 Network::RequestPacket::getPayloadSize( const void* buf )
{
    return buf ? ((Header*)buf)->int32s_[0] : 0;
}


int Network::RequestPacket::setIsNewRequest()
{
    const int reqid = ++curreqid_;
    setRequestID( reqid );
    setSubID( cBeginSubID() );
    return reqid;
}


od_int32 Network::RequestPacket::payloadSize() const
{
    return mPayloadSize_;
}


od_int32 Network::RequestPacket::requestID() const
{
    return mRequestID_;
}


void Network::RequestPacket::setRequestID( od_int32 id )
{
    mRequestID_ = id;
}


od_int16 Network::RequestPacket::subID() const
{
    return mSubID_;
}


void Network::RequestPacket::setSubID( od_int16 sid )
{
    mSubID_ = sid;
}


void* Network::RequestPacket::allocPayload( od_int32 size )
{
    if ( size < 1 )
	return 0;

    mDeclareAndTryAlloc( char*, pl, char[size] );
    return pl;
}


void Network::RequestPacket::setPayload( void* ptr, od_int32 size )
{
    deleteAndZeroArrPtr( payload_ );

    payload_ = (char*)ptr;
    mPayloadSize_ = payload_ ? size : 0;
}


void Network::RequestPacket::setStringPayload( const char* str )
{
    const int sz = FixedString( str ).size();
    if ( sz < 1 )
	setPayload( 0, 0 );
    else
    {
	int newplsz = sz + sizeof(int);
	void* newpl = allocPayload( newplsz );
	if ( !newpl )
	    newplsz = 0;
	else
	{
	    *((int*)newpl) = sz;
	    OD::memCopy( ((char*)newpl)+sizeof(int), str, sz );
	}

	setPayload( newpl, newplsz );
    }
}


void Network::RequestPacket::setPayload( const OD::JSON::Object& req )
{
    BufferString reqstr;
    req.dumpJSon( reqstr );

    OD::JSON::Object jsonhdr;
    jsonhdr.set( "byteorder", "little" );
    jsonhdr.set( "content-type", "text/json" );
    jsonhdr.set( "content-encoding", "utf-8" );
    jsonhdr.set( "content-length", reqstr.size() );
    jsonhdr.set( "source", "OpendTect" );
    BufferString messagestr;
    jsonhdr.dumpJSon( messagestr );
    const int jsonhdrsz = messagestr.size();

    messagestr.add( reqstr );
    const DataInterpreter<OD::String::size_type> interp( OD::UI16 );
    const int hdrsz = interp.nrBytes();
    const od_int32 payloadsz = hdrsz + messagestr.size();
    void* payloadptr = allocPayload( payloadsz );
    od_uint16* payloadiptr = (od_uint16*)payloadptr;

    interp.put( (void*)payloadiptr++, 0, jsonhdrsz );
    OD::memCopy( payloadiptr, messagestr.str(), messagestr.size() );

    setPayload( payloadptr, payloadsz );
}


void Network::RequestPacket::setPayload( const IOPar& req )
{
    //TODO
}


const void* Network::RequestPacket::payload() const
{
    return payload_;
}


void* Network::RequestPacket::payload( bool takeover )
{
    void* res = payload_;
    if ( takeover )
	payload_ = 0;
    return res;
}


void Network::RequestPacket::getStringPayload( BufferString& str ) const
{
    str.setEmpty();
    if ( !payload_ )
	return;

    const int plsz = mPayloadSize_;
    if ( plsz <= sizeof(int) )
	return;

    int nrchars = *((int*)payload_);
    if ( nrchars > plsz-sizeof(int) )
    {
	pErrMsg("Payload not consistent with string");
	nrchars = plsz-sizeof(int);
    }

    if ( nrchars < 1 )
	return;

    str.setBufSize( nrchars+1 );
    char* cstr = str.getCStr();
    if ( cstr )
    {
	OD::memCopy( cstr, payload_ + sizeof(int), nrchars );
	*(cstr+nrchars) = '\0';
    }
}


uiRetVal Network::RequestPacket::getPayload( OD::JSON::Object& json ) const
{
    uiRetVal ret;

    const void* recpayloadptr = payload_;
    const od_uint16* payloadiptr = (od_uint16*)recpayloadptr;

    const DataInterpreter<OD::String::size_type> interp( OD::UI16 );
    const int jsonhdrsz = interp.get( payloadiptr++, 0 );
    BufferString jsonhdrstr( jsonhdrsz+1, true );
    OD::memCopy( jsonhdrstr.getCStr(), payloadiptr, jsonhdrsz );
    OD::JSON::Object hdr;
    ret = hdr.parseJSon( jsonhdrstr.getCStr(), jsonhdrstr.size() );
    if ( !ret.isOK() )
	return ret;

    const BufferString type( hdr.getStringValue("content-type") );
    const od_int64 paysz = hdr.getIntValue( "content-length" );
    const BufferString endianess( hdr.getStringValue("byteorder") );
    const BufferString encoding( hdr.getStringValue("content-encoding") );
    recpayloadptr = (const char*)payloadiptr + jsonhdrsz;

    if ( type != "text/json" )
    {
	ret = tr("Incorrect return type");
	ret.add( tr("Expected: %1").arg("text/json") );
	ret.add( tr("Received: %1").arg(type) );
	return ret;
    }

    BufferString messagestr( paysz+1, true );
    OD::memCopy( messagestr.getCStr(), recpayloadptr, paysz );
    ret = json.parseJSon( messagestr.getCStr(), messagestr.size() );

    return ret;
}


uiRetVal Network::RequestPacket::getPayload( IOPar& par ) const
{
    //TODO
    uiRetVal ret;
    return ret;
}


void Network::RequestPacket::addErrMsg( BufferString& msg ) const
{
    if ( !isError() )
    {
	pErrMsg( "addErrMsg for non-error packet requested" );
	return;
    }

    BufferString mymsg; getStringPayload( mymsg );
    if ( mymsg.isEmpty() )
    {
	pErrMsg( "No error message in error packet" );
	mymsg = "Unknown network error occurred";
    }
    msg.add( ":\n" ).add( mymsg );

}

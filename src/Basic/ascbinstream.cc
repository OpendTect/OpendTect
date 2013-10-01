/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "ascbinstream.h"
#include "od_iostream.h"


//--- ascbinostream

#define mImplConstr(var,ismine) : strm_(var), binary_(bin), strmmine_(ismine) {}

ascbinostream::ascbinostream( od_ostream& strm, bool bin )
    mImplConstr( strm, false )
ascbinostream::ascbinostream( od_ostream* strm, bool bin )
    mImplConstr( strm ? *strm : od_ostream::nullStream(), true )
ascbinostream::~ascbinostream()
    { if ( strmmine_ ) delete &strm_; }
bool ascbinostream::isOK() const
    { return strm_.isOK(); }

#define mImplSingleAdd(typ,posttyp) \
ascbinostream& ascbinostream::add( typ t, posttyp post ) \
{ \
    if ( binary_ ) \
       return addBin( &t, sizeof(typ) ); \
\
    strm_.add( t ).add( post ); \
    return *this; \
}
#define mImplAdd(typ) mImplSingleAdd(typ,char) mImplSingleAdd(typ,const char*)

mImplAdd(char)
mImplAdd(unsigned char)
mImplAdd(od_int16)
mImplAdd(od_uint16)
mImplAdd(od_int32)
mImplAdd(od_uint32)
mImplAdd(od_int64)
mImplAdd(od_uint64)
mImplAdd(float)
mImplAdd(double)


ascbinostream& ascbinostream::addBin( const void* buf, od_stream_Count nrbytes )
{
    strm_.addBin( buf, nrbytes );
    return *this;
}


//--- ascbinistream


ascbinistream::ascbinistream( od_istream& strm, bool bin )
    mImplConstr( strm, false )
ascbinistream::ascbinistream( od_istream* strm, bool bin )
    mImplConstr( strm ? *strm : od_istream::nullStream(), true )
ascbinistream::~ascbinistream()
    { if ( strmmine_ ) delete &strm_; }
bool ascbinistream::isOK() const
    { return strm_.isOK(); }

#define mImplGet(typ) \
ascbinistream& ascbinistream::get( typ& t ) \
{ \
    if ( binary_ ) \
	return getBin( &t, sizeof(typ) ); \
\
    strm_.get( t ); \
    return *this; \
}

mImplGet(char)
mImplGet(unsigned char)
mImplGet(od_int16)
mImplGet(od_uint16)
mImplGet(od_int32)
mImplGet(od_uint32)
mImplGet(od_int64)
mImplGet(od_uint64)
mImplGet(float)
mImplGet(double)


ascbinistream& ascbinistream::getBin( void* buf, od_stream_Count nrbytes )
{
    strm_.getBin( buf, nrbytes );
    return *this;
}

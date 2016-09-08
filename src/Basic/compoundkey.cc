/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 8-9-1995
 * FUNCTION : Unit IDs
-*/


#include "dbkey.h"
#include "perthreadrepos.h"


char* CompoundKey::fromKey( IdxType keynr ) const
{
    return fetchKeyPart( keynr, false );
}


char* CompoundKey::fetchKeyPart( IdxType keynr, bool parttobuf ) const
{
    char* ptr = const_cast<char*>( impl_.buf() );
    if ( !ptr || (keynr<1 && !parttobuf) )
	return ptr;

    while ( keynr > 0 )
    {
	ptr = firstOcc( ptr, '.' );
	if ( !ptr ) return 0;
	ptr++; keynr--;
    }

    if ( parttobuf )
    {
	mDeclStaticString(bufstr);
	bufstr = ptr; char* bufptr = bufstr.getCStr();
	char* ptrend = firstOcc( bufptr, '.' );
	if ( ptrend ) *ptrend = '\0';
	ptr = bufptr;
    }

    return ptr;
}


CompoundKey::IdxType CompoundKey::nrKeys() const
{
    if ( impl_.isEmpty() ) return 0;

    IdxType nrkeys = 1;
    const char* ptr = impl_.buf();
    while ( true )
    {
	ptr = firstOcc(ptr,'.');
	if ( !ptr )
	    break;
	nrkeys++;
	ptr++;
    }

    return nrkeys;
}


BufferString CompoundKey::key( IdxType idx ) const
{
    return BufferString( fetchKeyPart(idx,true) );
}


void CompoundKey::setKey( IdxType ikey, const char* s )
{
					// example: "X.Y.Z", ikey=1, s="new"

    char* ptr = fromKey( ikey );	// ptr="Y.Z"
    if ( !ptr ) return;

    const BufferString lastpart( firstOcc(ptr,'.') ); // lastpart=".Z"
    *ptr = '\0';			// impl_="X."

    if ( s && *s )
	impl_.add( s );			// impl_="X.new"
    else if ( ptr != impl_.buf() )
	*(ptr-1) = '\0';		// impl_="X"

    impl_.add( lastpart );		// impl_="X.new.Z" or "X.Z" if s==0
}


CompoundKey CompoundKey::upLevel() const
{
    if ( impl_.isEmpty() )
	return CompoundKey("");

    CompoundKey ret( *this );

    const IdxType nrkeys = nrKeys();
    if ( nrkeys <= 1 )
	ret.setEmpty();
    else
    {
	char* ptr = ret.fromKey( nrkeys-1 );
	if ( ptr ) *(ptr-1) = '\0';
    }

    return ret;
}


bool CompoundKey::isUpLevelOf( const CompoundKey& ky ) const
{
    return nrKeys() < ky.nrKeys() && impl_.isStartOf( ky.impl_ );
}


DBKey::DBKey( SubID id1, SubID id2 )
{
    if ( id1 == 0 && id2 == 0 )
	add( id1 );
    else
    {
	if ( id1 != 0 )
	    add( id1 );
	if ( id2 != 0 )
	    add( id2 );
    }
}


DBKey::SubID DBKey::getIDAt( int lvl ) const
{
    const BufferString idstr( key(lvl) );
    return idstr.isEmpty() ? 0 : idstr.toInt();
}


DBKey::SubID DBKey::leafID() const
{
    if ( isEmpty() )
	return 0;

    const char* dotptr = lastOcc( impl_.str(), '.' );
    const char* ptr = dotptr ? dotptr + 1 : str();
    return ptr && *ptr ? ::toInt( ptr ) : 0;
}


DBKey DBKey::parent() const
{
    return DBKey( upLevel().buf() );
}


const DBKey& DBKey::getInvalid()
{
   mDefineStaticLocalObject( DBKey, _invalid, (-1) );
   return _invalid;
}


bool DBKey::isInvalid() const
{
    return impl_.isEmpty() || impl_ == getInvalid().impl_;
}


od_int64 DBKey::toInt64() const
{
    if ( isEmpty() )
	return 0;

    const SubID ileaf = leafID();
    const DBKey par( parent() );
    const SubID ipar = par.isEmpty() ? 0 : parent().leafID();
    return (((od_uint64)ipar) << 32) + (((od_uint64)ileaf) & 0xFFFFFFFF);
}


DBKey DBKey::fromInt64( od_int64 i64 )
{
    const SubID ipar = (SubID)(i64 >> 32);
    const SubID ileaf = (SubID)(i64 & 0xFFFFFFFF);
    return DBKey( ipar, ileaf );
}

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 8-9-1995
 * FUNCTION : Unit IDs
-*/


#include "multiid.h"
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
    const char* ptr = impl_;
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


MultiID::SubID MultiID::getIDAt( int lvl ) const
{
    const BufferString idstr( key(lvl) );
    return idstr.isEmpty() ? 0 : idstr.toInt();
}


MultiID::SubID MultiID::leafID() const
{
    const char* ptr = lastOcc( impl_, '.' );
    return ::toInt( ptr ? ptr+1 : str() );
}


MultiID MultiID::parent() const
{
    return MultiID( upLevel().buf() );
}


const MultiID& MultiID::udf()
{
   mDefineStaticLocalObject( MultiID, _udf, (-1) );
   return _udf;
}


bool MultiID::isUdf() const
{
    return impl_.isEmpty() || impl_ == udf().impl_;
}

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "linekey.h"
#include "iopar.h"
#include "ioman.h"
#include "keystrs.h"
#include "posinfo2dsurv.h"


LineKey::LineKey( const char* lnm, const char* attrnm )
    : BufferString(lnm)
{
    const StringView attribname( attrnm );
    if ( !attribname.isEmpty() )
    { *this += "|"; *this += attrnm; }
}


bool LineKey::operator ==( const LineKey& lk ) const
{
    return lk.lineName() == lineName() && lk.attrName() == attrName();
}


BufferString LineKey::lineName() const
{
    BufferString ret( *this );
    ret.trimBlanks();
    if ( ret.isEmpty() )
	return ret;

    char* ptr = ret.find( '|' );
    if ( ptr ) *ptr = '\0';
    return ret;
}


BufferString LineKey::attrName() const
{
    const char* ptr = sKeyDefAttrib();
    if ( !isEmpty() )
    {
	ptr = firstOcc( buf(), '|' );
	if ( ptr )
	    { ptr++; mSkipBlanks(ptr); }
	else
	    ptr = sKeyDefAttrib();
    }

    return BufferString( ptr );
}


void LineKey::fillPar( IOPar& iop, bool iopnm ) const
{
    if ( !iopnm )
	iop.set( sKey::LineKey(), *this );
    else
    {
	iop.setName( lineName() );
	iop.set( sKey::Attribute(), attrName() );
    }
}


bool LineKey::usePar( const IOPar& iop, bool iopnm )
{
    const BufferString res = iop.find(
			    iopnm ? sKey::Attribute() : sKey::LineKey() );
    if ( (!iopnm && res.isEmpty()) || (iopnm && iop.name().isEmpty() ) )
	return false;

    if ( !iopnm )
	*this = res.buf();
    else
	*this = LineKey( iop.name(), res );

    return true;
}

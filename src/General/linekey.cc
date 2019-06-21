/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2004
-*/


#include "linekey.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo2dsurv.h"


LineKey::LineKey( const char* lnm, const char* attrnm )
    : BufferString(lnm)
{
    const FixedString attribname( attrnm );
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


void LineKey::setLineName( const char* lnm )
{ set( attrName() ); *this += "|"; *this += lnm; }

void LineKey::setAttrName( const char* anm )
{ set( lineName() ); *this += "|"; *this += anm; }

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
    const char* res = iop.find( iopnm ? sKey::Attribute() : sKey::LineKey() );
    if ( (!iopnm && !res) || (iopnm && iop.name().isEmpty() ) )
	return false;

    if ( !iopnm )
	set( res );
    else
    {
	setLineName( iop.name() );
	setAttrName( res );
    }

    return true;
}

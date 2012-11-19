/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2004
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "linekey.h"
#include "iopar.h"
#include "ioman.h"
#include "keystrs.h"
#include "surv2dgeom.h"


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
    if ( ret.isEmpty() ) return ret;

    char* ptr = strchr( ret.buf(), '|' );
    if ( ptr ) *ptr = '\0';
    removeTrailingBlanks( ret.buf() );
    return ret;
}


BufferString LineKey::attrName() const
{
    const char* ptr = sKeyDefAttrib();
    if ( !isEmpty() )
    {
	ptr = strchr( buf(), '|' );
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
    const char* res = iop.find( iopnm ? sKey::Attribute() : sKey::LineKey() );
    if ( (!iopnm && !res) || (iopnm && iop.name().isEmpty() ) )
	return false;

    if ( !iopnm )
	*this = res;
    else
	*this = LineKey( iop.name(), res );

    return true;
}


BufferString LineKey::defKey2DispName( const char* defkey, const char* ioobjnm,
					bool embed )
{
    if ( !IOM().isKey(defkey) )
	return BufferString( defkey );

    const BufferString keynm = IOM().nameOf( defkey );
    const BufferString retnm = ioobjnm && *ioobjnm ? ioobjnm : keynm.buf();

    const bool is2d = S2DPOS().hasLineSet( retnm );
    BufferString ret = embed ? "[" : "";
    if ( !is2d )
	ret.add( retnm );
    else
    {
	LineKey lk( defkey );
	lk.setLineName( retnm );
	ret.add( lk );
    }

    if ( embed ) ret += "]";

    return ret;
}

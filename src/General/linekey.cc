/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2004
-*/

static const char* rcsID = "$Id: linekey.cc,v 1.17 2011/05/30 03:21:38 cvsnanne Exp $";

#include "linekey.h"
#include "iopar.h"
#include "ioobj.h"
#include "ptrman.h"
#include "ioman.h"
#include "keystrs.h"


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
	iop.set( sKey::LineKey, *this );
    else
    {
	iop.setName( lineName() );
	iop.set( sKey::Attribute, attrName() );
    }
}


bool LineKey::usePar( const IOPar& iop, bool iopnm )
{
    const char* res = iop.find( iopnm ? sKey::Attribute : sKey::LineKey );
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
    BufferString ret;
    if ( !IOObj::isKey(defkey) )
	ret = defkey;
    else
    {
	LineKey lk( defkey );
	ret = embed ? "[" : "";
	if ( ioobjnm && *ioobjnm )
	    lk.setLineName( ioobjnm );
	else
	{
	    PtrMan<IOObj> ioobj = IOM().get( MultiID(defkey) );
	    if ( ioobj )
		lk.setLineName( ioobj->name() );
	    else
	    {
		BufferString nm( "<" ); nm += lk.lineName(); nm += ">";
		lk.setLineName( nm );
	    }
	}
	ret += lk;
	if ( embed ) ret += "]";
    }
    return ret;
}

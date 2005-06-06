/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2004
-*/

static const char* rcsID = "$Id: linekey.cc,v 1.4 2005-06-06 10:43:57 cvsbert Exp $";

#include "linekey.h"
#include "iopar.h"
#include "ioobj.h"
#include "ptrman.h"
#include "ioman.h"
#include "keystrs.h"

const char* LineKey::sKeyDefAttrib = "Seis";


LineKey::LineKey( const char* lnm, const char* attrnm )
    	: BufferString(lnm)
{
    if ( attrnm && *attrnm && strcmp(attrnm,sKeyDefAttrib) )
	{ *this += "|"; *this += attrnm; }
}


BufferString LineKey::lineName() const
{
    BufferString ret( *this );
    char* ptr = strchr( ret.buf(), '|' );
    if ( ptr ) *ptr = '\0';
    return ret;
}


BufferString LineKey::attrName() const
{
    BufferString ret;
    char* ptr = strchr( buf(), '|' );
    if ( ptr )	ret = ptr + 1;
    else	ret = sKeyDefAttrib;
    return ret;
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
    if ( (!iopnm && !res) || (iopnm && iop.name() == "" ) )
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

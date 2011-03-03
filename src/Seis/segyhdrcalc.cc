/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2011
-*/

static const char* rcsID = "$Id: segyhdrcalc.cc,v 1.2 2011-03-03 15:13:16 cvsbert Exp $";


#include "segyhdrcalc.h"
#include "mathexpression.h"


SEGY::HdrCalcSet::HdrCalcSet( const SEGY::HdrDef& hd )
    : hdef_(hd)
{
    trcidxhe_.setUdf();
    trcidxhe_.setName("INDEXNR");
    trcidxhe_.setDescription("Trace index (sequence) number in file");
}


SEGY::HdrCalcSet::~HdrCalcSet()
{
    deepErase( exprs_ );
    deepErase( *this );
}


int SEGY::HdrCalcSet::indexOf( const char* nm ) const
{
    const BufferString henm( nm );
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( henm == (*this)[idx]->he_.name() )
	    return idx;
    }
    return -1;
}


bool SEGY::HdrCalcSet::add( const SEGY::HdrEntry& he, const char* def,
       			    BufferString* emsg )
{
    MathExpressionParser mep( def );
    MathExpression* me = mep.parse();
    if ( !me )
    {
	if ( emsg ) *emsg = mep.errMsg();
	return false;
    }

    *this += new SEGY::HdrCalc( he, def );
    exprs_ += me;
    return true;
}


void SEGY::HdrCalcSet::discard( int idx )
{
    delete remove( idx );
    delete exprs_.remove( idx );
}


bool SEGY::HdrCalcSet::apply( void* buf ) const
{
    /*
    for ( int idx=0; idx<size(); idx++ )
    {
	vals += idx;
    }
    */
    return true;
}

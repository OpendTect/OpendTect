/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2011
-*/

static const char* rcsID = "$Id: segyhdrcalc.cc,v 1.1 2011-03-02 16:11:04 cvsbert Exp $";


#include "segyhdrcalc.h"
#include "mathexpression.h"


SEGY::HdrCalcSet::HdrCalcSet( const SEGY::HdrDef& hd )
    : hdef_(hd)
{
    seqnr_.setUdf();
    seqnr_.setName("<SEQNR>");
    seqnr_.setDescription("Sequence number in file");
}


SEGY::HdrCalcSet::~HdrCalcSet()
{
    deepErase( exprs_ );
    deepErase( *this );
}


void SEGY::HdrCalcSet::add( SEGY::HdrCalc* hc )
{
    *this += hc;
    //TODO add to exprs_
}


void SEGY::HdrCalcSet::discard( int idx )
{
    delete ObjectSet<HdrCalc>::remove( idx );
    //TODO delete exprs_.remove( idx );
}


bool SEGY::HdrCalcSet::getValues( const void* buf, TypeSet<int>& vals ) const
{
    vals.erase();
    //TODO implement
    for ( int idx=0; idx<size(); idx++ )
	vals += idx;
    return true;
}

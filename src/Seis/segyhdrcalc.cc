/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2011
-*/

static const char* rcsID = "$Id: segyhdrcalc.cc,v 1.3 2011-03-04 14:40:21 cvsbert Exp $";


#include "segyhdrcalc.h"
#include "mathexpression.h"


SEGY::HdrCalcSet::HdrCalcSet( const SEGY::HdrDef& hd )
    : hdef_(hd)
{
    trcidxhe_.setUdf();
    trcidxhe_.setName("INDEXNR");
    trcidxhe_.setDescription("Trace index (sequence) number in file");
    reSetSeqNr();
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

    const int nrvars = me->nrVariables();
    TypeSet<int>* heidxs = new TypeSet<int>;

    for ( int ivar=0; ivar<nrvars; ivar++ )
    {
	const MathExpression::VarType vt = me->getType( ivar );
	const BufferString varnm( me->fullVariableExpression(ivar) );
	if ( vt != MathExpression::Variable )
	{
	    if ( emsg )
	    {
		*emsg = "'";
		emsg->add( varnm ).add( "' is a ")
		    .add( vt == MathExpression::Constant
			? "named constant" : "recursive expression" )
		    .add( "\nThese are not supported." );
	    }
	    delete me; delete heidxs; return false;
	}

	int heidx = hdef_.indexOf( varnm );
	if ( heidx < 0 )
	{
	    if ( varnm == trcidxhe_.name() )
		heidx = -1;
	    else
	    {
		if ( emsg )
		{
		    *emsg = " Found variable: '";
		    emsg->add( varnm ).add( "', which is not a header field");
		}
		delete me; delete heidxs; return false;
	    }
	}
	*heidxs += heidx;
    }

    *this += new SEGY::HdrCalc( he, def );
    exprs_ += me;
    heidxs_ += heidxs;
    return true;
}


void SEGY::HdrCalcSet::discard( int idx )
{
    delete remove( idx );
    delete exprs_.remove( idx );
    delete heidxs_.remove( idx );
}


void SEGY::HdrCalcSet::apply( void* buf ) const
{
    for ( int iexpr=0; iexpr<exprs_.size(); iexpr++ )
    {
	MathExpression& me = *(const_cast<MathExpression*>(exprs_[iexpr]));
	const TypeSet<int>& heidxs = *heidxs_[iexpr];
	for ( int ivar=0; ivar<heidxs.size(); ivar++ )
	{
	    float val = seqnr_;
	    const int heidx = heidxs[ivar];
	    if ( heidx >= 0 )
		val = hdef_[heidx]->getValue( buf );
	    me.setVariableValue( ivar, val );
	}

	const float meval = me.getValue();
	(*this)[iexpr]->he_.putValue( buf, mNINT(meval) );
    }

    seqnr_++;
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "mathformula.h"
#include "mathexpression.h"
#include "unitofmeasure.h"
#include "iopar.h"


Math::Formula::Formula( const char* txt )
    : expr_(0)
    , outputunit_(0)
{
    setText( txt );
}


Math::Formula::Formula( const Math::Formula& oth )
    : expr_(0)
{
    *this = oth;
}


Math::Formula& Math::Formula::operator =( const Math::Formula& oth )
{
    if ( this != &oth )
    {
	delete expr_;
	expr_ = oth.expr_ ? oth.expr_->clone() : 0;
	text_ = oth.text_;
	inps_ = oth.inps_;
	outputunit_ = oth.outputunit_;
	recstartvals_ = oth.recstartvals_;
	prevvals_ = oth.prevvals_;
	inpidxs_ = oth.inpidxs_;
	varshifts_ = oth.varshifts_;
	maxshift_ = oth.maxshift_;
    }
    return *this;
}


Math::Formula::~Formula()
{
    delete expr_;
}


int Math::Formula::indexOf( const char* varnm ) const
{
    for ( int idx=0; idx<inps_.size(); idx++ )
	if ( inps_[idx].name_ == varnm )
	    return idx;
    return -1;
}


void Math::Formula::setText( const char* inp )
{
    delete expr_;
    inps_.setEmpty();
    inpidxs_.setEmpty();
    varshifts_.setEmpty();
    recstartvals_.setEmpty();
    maxshift_ = 0;

    text_ = inp;
    ExpressionParser mep( inp );
    expr_ = mep.parse();
    if ( !expr_ )
    {
	errmsg_.set( mep.errMsg() );
	if ( errmsg_.isEmpty() )
	    errmsg_.set( "Invalid mathematical expression" );
	return;
    }

    const int nrvars = expr_->nrVariables();
    for ( int ivar=0; ivar<nrvars; ivar++ )
    {
	const BufferString varnm = expr_->fullVariableExpression( ivar );
	int inpidx = indexOf( varnm );
	int shft = 0;
	if ( inpidx < 0 )
	{
	    const Expression::VarType vtyp = expr_->getType( ivar );
	    if ( vtyp == Expression::Recursive )
	    {
		ExpressionParser::varNameOf( varnm, &shft );
		if ( shft < 0 )
		    shft = -shft;
	    }
	    else
	    {
		inps_ += InpDef( varnm, vtyp == Expression::Constant );
		inpidx = inps_.size() - 1;
	    }
	}
	inpidxs_ += inpidx;
	varshifts_ += shft;
	if ( maxshift_ < shft )
	    maxshift_ = shft;
    }

    for ( int idx=0; idx<maxshift_; idx++ )
	recstartvals_ += 0;
}


void Math::Formula::setInputName( int idx, const char* nm )
{
    if ( inps_.validIdx(idx) )
	inps_[idx].name_ = nm;
}


void Math::Formula::setInputUnit( int idx, const UnitOfMeasure* uom )
{
    if ( inps_.validIdx(idx) )
	inps_[idx].unit_ = uom;
}


void Math::Formula::startNewSeries() const
{
    prevvals_ = recstartvals_;
}


float Math::Formula::getValue( const float* vals, bool internuns ) const
{
    TypeSet<double> dvals;
    for ( int idx=0; idx<inps_.size(); idx++ )
	dvals += vals[idx];
    return (float)getValue( dvals.arr(), internuns );
}


double Math::Formula::getValue( const double* vals, bool internuns ) const
{
    if ( prevvals_.size() < maxshift_ )
    {
	startNewSeries();
	for ( int idx=prevvals_.size(); idx<maxshift_; idx++ )
	    prevvals_ += 0;

    }

    for ( int ivar=0; ivar<inpidxs_.size(); ivar++ )
    {
	const int inpidx = inpidxs_[ivar];
	if ( inpidx >= 0 )
	{
	    double val = vals[inpidx];
	    if ( inps_[inpidx].unit_ && !mIsUdf(val) )
		val = inps_[inpidx].unit_->getUserValueFromSI( val );

	    expr_->setVariableValue( ivar, val );
	}
	else
	{
	    double val = 0;
	    if ( varshifts_[ivar] > 0 )
	    {
		const int idx = prevvals_.size() - varshifts_[ivar];
		if ( idx >= 0 )
		    val = prevvals_[idx];
	    }
	    expr_->setVariableValue( ivar, val );
	}
    }

    const double formval = expr_->getValue();
    prevvals_ += formval;

    return outputunit_ && internuns ? outputunit_->getSIValue( formval )
				    : formval;
}


void Math::Formula::fillPar( IOPar& iop ) const
{
    iop.update( sKeyExpression(), text_ );
    iop.update( sKey::Output(), outputunit_ ? outputunit_->name().buf() : 0 );

    if ( recstartvals_.isEmpty() )
	iop.removeWithKey( sKeyRecStartVals() );
    else
	iop.set( sKeyRecStartVals(), recstartvals_ );

    iop.removeWithKeyPattern( BufferString(sKey::Input(),".*") );
    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const InpDef& id = inps_[idx];
	BufferString unstr;
	if ( id.unit_ ) unstr = id.unit_->name();
	iop.set( IOPar::compKey(sKey::Input(),idx), id.name_, unstr );
    }


}


void Math::Formula::usePar( const IOPar& iop )
{
    setText( iop.find(sKeyExpression()) );
    if ( !isOK() )
	return;

    BufferString unstr;
    iop.get( sKey::Output(), unstr );
    outputunit_ = unstr.isEmpty() ? 0 : UoMR().get( unstr );

    iop.get( sKeyRecStartVals(), recstartvals_ );

    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	InpDef& id = inps_[idx];
	iop.get( IOPar::compKey(sKey::Input(),idx), id.name_, unstr );
	id.unit_ = UoMR().get( unstr );
    }
}

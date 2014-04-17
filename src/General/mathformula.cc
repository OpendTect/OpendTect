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


int Math::SpecVarSet::getIndexOf( const char* varnm ) const
{
    if ( !varnm || !*varnm )
	return -1;

    for ( int idx=0; idx<size(); idx++ )
	if ( varName(idx).isEqual(varnm,CaseInsensitive) )
	    return idx;

    return -1;
}


void Math::SpecVarSet::getNames( BufferStringSet& nms, bool usrdisp ) const
{
    for ( int idx=0; idx<size(); idx++ )
	nms.add( usrdisp ? dispName(idx) : varName(idx) );
}


Interval<int> Math::Formula::InpDef::shftRg() const
{
    const int sz = shifts_.size();
    if ( sz < 1 )
	return Interval<int>(0,0);
    Interval<int> rg( shifts_[0], shifts_[0] );
    for ( int idx=1; idx<sz; idx++ )
	rg.include( shifts_[idx], false );
    return rg;
}


static const Math::SpecVarSet emptysvs;


Math::Formula::Formula( const SpecVarSet* svs )
    : expr_(0)
    , outputunit_(0)
    , specvars_(svs ? *svs : emptysvs)
{
    setText( 0 );
}


Math::Formula::Formula( const char* txt, const SpecVarSet* svs )
    : expr_(0)
    , outputunit_(0)
    , specvars_(svs ? *svs : emptysvs)
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
	validxs_ = oth.validxs_;
	recshifts_ = oth.recshifts_;
	const_cast<SpecVarSet&>(specvars_) = oth.specvars_;
    }
    return *this;
}


Math::Formula::~Formula()
{
    delete expr_;
}


int Math::Formula::varNameIdx( const char* varnm ) const
{
    for ( int idx=0; idx<inps_.size(); idx++ )
	if ( inps_[idx].varname_ == varnm )
	    return idx;
    return -1;
}


void Math::Formula::addShift( int iinp, int ivar, int& shft,
			TypeSet< TypeSet<int> >& shiftvaridxs )
{
    inps_[iinp].shifts_ += shft;
    shiftvaridxs[iinp] += ivar;
    shft = 0;
}


void Math::Formula::setText( const char* inp )
{
    delete expr_;
    inps_.setEmpty();
    inpidxs_.setEmpty();
    validxs_.setEmpty();
    recshifts_.setEmpty();
    recstartvals_.setEmpty();

    int maxrecshift = 0;
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
    TypeSet< TypeSet<int> > shiftvaridxs;
    for ( int ivar=0; ivar<nrvars; ivar++ )
    {
	int shft = 0;
	const BufferString varnm = ExpressionParser::varNameOf(
				expr_->fullVariableExpression(ivar), &shft );
	int inpidx = varNameIdx( varnm );
	if ( inpidx >= 0 )
	    addShift( inpidx, ivar, shft, shiftvaridxs );
	else
	{
	    const Expression::VarType vtyp = expr_->getType( ivar );
	    if ( vtyp == Expression::Recursive )
	    {
		if ( shft < 0 )
		    shft = -shft;
		if ( maxrecshift < shft )
		    maxrecshift = shft;
	    }
	    else
	    {
		InpDef::Type typ = vtyp == Expression::Constant	? InpDef::Const
			    : (specvars_.isPresent(varnm)	? InpDef::Spec
								: InpDef::Var);
		InpDef id( varnm, typ );
		inps_ += id;
		inpidx = inps_.size() - 1;
		shiftvaridxs += TypeSet<int>();
		addShift( inpidx, ivar, shft, shiftvaridxs );
	    }
	}
	inpidxs_ += inpidx;
	recshifts_ += shft;
    }

    validxs_.setSize( nrvars, -1 );
    int ivalidx = 0;
    for ( int iinp=0; iinp<inps_.size(); iinp++ )
    {
	const TypeSet<int>& inpshiftvar = shiftvaridxs[iinp];
	for ( int ishft=0; ishft<inpshiftvar.size(); ishft++ )
	{
	    validxs_[ inpshiftvar[ishft] ] = ivalidx;
	    ivalidx++;
	}
    }

    for ( int idx=0; idx<maxrecshift; idx++ )
	recstartvals_ += 0;
}


void Math::Formula::setInputDef( int idx, const char* def )
{
    if ( inps_.validIdx(idx) )
	inps_[idx].inpdef_ = def;
}


void Math::Formula::setInputUnit( int idx, const UnitOfMeasure* uom )
{
    if ( inps_.validIdx(idx) )
	inps_[idx].unit_ = uom;
}


void Math::Formula::clearInputDefs()
{
    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	inps_[idx].inpdef_.setEmpty();
	inps_[idx].unit_ = 0;
    }
}


int Math::Formula::specIdx( int iinp ) const
{
    return isSpec(iinp) ? specvars_.getIndexOf( variableName(iinp) ) : -1;
}


double Math::Formula::getConstVal( int iinp ) const
{
    return isConst( iinp ) ? toDouble( inputDef(iinp) ) : mUdf(double);
}


int Math::Formula::nrValues2Provide() const
{
    int nrvals = 0;
    for ( int ivar=0; ivar<inpidxs_.size(); ivar++ )
	if ( inpidxs_[ivar] >= 0 )
	    nrvals++;
    return nrvals;
}


void Math::Formula::startNewSeries() const
{
    prevvals_ = recstartvals_;
}


float Math::Formula::getValue( const float* vals, bool internuns ) const
{
    const int nrinpvals = nrValues2Provide();
    TypeSet<double> dvals;
    for ( int idx=0; idx<nrinpvals; idx++ )
	dvals += vals[idx];
    return (float)getValue( dvals.arr(), internuns );
}


double Math::Formula::getValue( const double* vals, bool internuns ) const
{
    if ( prevvals_.size() < maxRecShift() )
    {
	startNewSeries();
	for ( int idx=prevvals_.size(); idx<maxRecShift(); idx++ )
	    prevvals_ += 0;

    }

    for ( int ivar=0; ivar<inpidxs_.size(); ivar++ )
    {
	const int inpidx = inpidxs_[ivar];
	double val = 0;
	if ( inpidx >= 0 )
	{
	    val = vals[ validxs_[ivar] ];
	    if ( inps_[inpidx].unit_ && !mIsUdf(val) )
		val = inps_[inpidx].unit_->getUserValueFromSI( val );
	}
	else
	{
	    if ( recshifts_[ivar] > 0 )
	    {
		const int idx = prevvals_.size() - recshifts_[ivar];
		if ( idx >= 0 )
		    val = prevvals_[idx];
	    }
	    expr_->setVariableValue( ivar, val );
	}
	expr_->setVariableValue( ivar, val );
    }

    const double formval = expr_->getValue();
    prevvals_ += formval;

    return outputunit_ && internuns ? outputunit_->getSIValue( formval )
				    : formval;
}


#define mDefInpKeybase \
    const BufferString inpkybase( IOPar::compKey(sKey::Input(),idx) )
#define mOutUnKy IOPar::compKey(sKey::Output(),sKey::Unit())
#define mInpDefKy IOPar::compKey(inpkybase,"Def")
#define mInpUnKy IOPar::compKey(inpkybase,sKey::Unit())


void Math::Formula::fillPar( IOPar& iop ) const
{
    iop.update( sKeyExpression(), text_ );
    iop.update( mOutUnKy, outputunit_ ? outputunit_->name().buf() : 0 );

    if ( recstartvals_.isEmpty() )
	iop.removeWithKey( sKeyRecStartVals() );
    else
	iop.set( sKeyRecStartVals(), recstartvals_ );

    iop.removeWithKeyPattern( BufferString(sKey::Input(),".*") );
    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const InpDef& id = inps_[idx];
	mDefInpKeybase;

	iop.set( mInpDefKy, id.inpdef_ );
	BufferString unstr;
	if ( id.unit_ ) unstr = id.unit_->name();
	iop.update( mInpUnKy, unstr );
    }
}


void Math::Formula::usePar( const IOPar& iop )
{
    setText( iop.find(sKeyExpression()) );
    if ( !isOK() )
	return;

    BufferString unstr;
    iop.get( mOutUnKy, unstr );
    outputunit_ = unstr.isEmpty() ? 0 : UoMR().get( unstr );

    iop.get( sKeyRecStartVals(), recstartvals_ );

    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	InpDef& id = inps_[idx];
	mDefInpKeybase;

	iop.get( mInpDefKy, id.inpdef_ );
	iop.get( mInpUnKy, unstr );
	id.unit_ = UoMR().get( unstr );
    }
}

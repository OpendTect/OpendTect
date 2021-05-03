/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2014
________________________________________________________________________

-*/

#include "mathformula.h"
#include "keystrs.h"
#include "mathspecvars.h"
#include "mathformulatransl.h"
#include "mathexpression.h"
#include "unitofmeasure.h"
#include "iopar.h"

mDefSimpleTranslators(MathFormula,Math::Formula::sKeyFileType(),od,Misc);

static Math::SpecVarSet emptyspecvarset;


const Math::SpecVarSet& Math::SpecVarSet::getEmpty()
{
    return emptyspecvarset;
}


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


Math::Formula::Formula( bool inpseries, const char* txt )
    : expr_(0)
    , specvars_(&SpecVarSet::getEmpty())
    , inputsareseries_(inpseries)
    , outputunit_(0)
{
    setText( txt );
}


Math::Formula::Formula( bool inpseries, const SpecVarSet& svs, const char* txt )
    : expr_(0)
    , specvars_(&svs)
    , inputsareseries_(inpseries)
    , outputunit_(0)
{
    setText( txt );
}


Math::Formula::Formula( const Math::Formula& oth )
    : expr_(0)
    , inputsareseries_(true)
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
	specvars_ = oth.specvars_;
	const_cast<bool&>(inputsareseries_) = oth.inputsareseries_;
    }
    return *this;
}


Math::Formula::~Formula()
{
    delete expr_;
}


const char* Math::Formula::userDispText() const
{
    mDeclStaticString( ret );
    ret.set( text_ );
    for ( int iinp=0; iinp<inps_.size(); iinp++ )
    {
	if ( !isConst(iinp) && !isSpec(iinp) )
	    ret.replace( variableName(iinp), inputDef(iinp) );
    }
    return ret;
}


int Math::Formula::varNameIdx( const char* varnm ) const
{
    for ( int iinp=0; iinp<inps_.size(); iinp++ )
	if ( inps_[iinp].varname_ == varnm )
	    return iinp;
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
    ExpressionParser mep( inp, inputsareseries_ );
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
			    : (specvars_->isPresent(varnm)	? InpDef::Spec
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


void Math::Formula::setInputDef( int iinp, const char* def )
{
    if ( inps_.validIdx(iinp) )
	inps_[iinp].inpdef_ = def;
}


void Math::Formula::setInputUnit( int iinp, const UnitOfMeasure* uom )
{
    if ( inps_.validIdx(iinp) )
	inps_[iinp].unit_ = uom;
}


void Math::Formula::clearInputDefs()
{
    for ( int iinp=0; iinp<inps_.size(); iinp++ )
    {
	inps_[iinp].inpdef_.setEmpty();
	inps_[iinp].unit_ = 0;
    }
}


int Math::Formula::specIdx( int iinp ) const
{
    return isSpec(iinp) ? specvars_->getIndexOf( variableName(iinp) ) : -1;
}


double Math::Formula::getConstVal( int iinp ) const
{
    return isConst( iinp ) ? toDouble( inputDef(iinp) ) : mUdf(double);
}


int Math::Formula::nrConsts() const
{
    int nr = 0;
    for ( int iinp=0; iinp<inps_.size(); iinp++ )
	if ( isConst(iinp) )
	    nr++;
    return nr;
}


int Math::Formula::nrExternalInputs() const
{
    int nrinputs = 0;
    for ( int idx=0; idx<nrInputs(); idx++ )
	if ( !isConst(idx) && !isSpec(idx) )
	    nrinputs++;

    return nrinputs;
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
    for ( int ival=0; ival<nrinpvals; ival++ )
	dvals += vals[ival];
    return (float)getValue( dvals.arr(), internuns );
}


double Math::Formula::getValue( const double* vals, bool internuns ) const
{
    if ( !expr_ )
	return mUdf(double);

    Threads::Locker lckr( formlock_ );
    if ( inputsareseries_ && prevvals_.size() < maxRecShift() )
    {
	startNewSeries();
	for ( int ishift=prevvals_.size(); ishift<maxRecShift(); ishift++ )
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
	else if ( inputsareseries_ )
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
    if ( inputsareseries_ )
	prevvals_ += formval;

    return outputunit_ && internuns ? outputunit_->getSIValue( formval )
				    : formval;
}


#define mDefInpKeybase \
    const BufferString inpkybase( IOPar::compKey(sKey::Input(),iinp) )
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
    for ( int iinp=0; iinp<inps_.size(); iinp++ )
    {
	const InpDef& id = inps_[iinp];
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

    for ( int iinp=0; iinp<inps_.size(); iinp++ )
    {
	InpDef& id = inps_[iinp];
	mDefInpKeybase;

	iop.get( mInpDefKy, id.inpdef_ );
	iop.get( mInpUnKy, unstr );
	id.unit_ = UoMR().get( unstr );
    }
}

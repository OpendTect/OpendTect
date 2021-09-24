/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2014
________________________________________________________________________

-*/

#include "mathformula.h"

#include "iopar.h"
#include "keystrs.h"
#include "mathexpression.h"
#include "mathformulatransl.h"
#include "mathspecvars.h"
#include "unitofmeasure.h"

mDefSimpleTranslators(MathFormula,Math::Formula::sKeyFileType(),od,Misc);


const Math::SpecVarSet& Math::SpecVarSet::getEmpty()
{
    static ConstPtrMan<Math::SpecVarSet> emptyspecvarset =
						new Math::SpecVarSet();
    return *emptyspecvarset.ptr();
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
    : specvars_(&SpecVarSet::getEmpty())
    , inputsareseries_(inpseries)
{
    setText( txt );
}


Math::Formula::Formula( bool inpseries, const SpecVarSet& svs, const char* txt )
    : specvars_(&svs)
    , inputsareseries_(inpseries)
{
    setText( txt );
}


Math::Formula::Formula( const Math::Formula& oth )
    : inputsareseries_(true)
{
    *this = oth;
}


Math::Formula& Math::Formula::operator =( const Math::Formula& oth )
{
    if ( this != &oth )
    {
	delete expr_;
	expr_ = oth.expr_ ? oth.expr_->clone() : nullptr;
	text_ = oth.text_;
	inps_ = oth.inps_;
	outputformunit_ = oth.outputformunit_;
	outputvalunit_ = oth.outputvalunit_;
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


void Math::Formula::setInputMnemonic( int iinp, const Mnemonic* mn )
{
    if ( !inps_.validIdx(iinp) )
	return;

    InpDef& inp = inps_[iinp];
    inp.formmn_ = mn;
    if ( !mn )
	return;

    if ( !inp.formunit_ ||
	 (inp.formunit_ && mn->unit() &&
	  !inp.formunit_->isCompatibleWith(*mn->unit())) )
	inp.formunit_ = mn->unit();
}


void Math::Formula::setInputFormUnit( int iinp, const UnitOfMeasure* uom )
{
    if ( inps_.validIdx(iinp) )
	inps_[iinp].formunit_ = uom;
}


void Math::Formula::setInputValUnit( int iinp, const UnitOfMeasure* uom )
{
    if ( inps_.validIdx(iinp) )
	inps_[iinp].valunit_ = uom;
}


void Math::Formula::clearInputDefs()
{
    for ( auto& inp : inps_ )
    {
	inp.inpdef_.setEmpty();
	inp.formmn_ = nullptr;
	inp.formunit_ = nullptr;
	inp.valunit_ = nullptr;
    }
}


void Math::Formula::clearAllDefs()
{
    clearInputDefs();
    outputformmn_ = nullptr;
    outputformunit_ = nullptr;
    outputvalunit_ = nullptr;
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


double Math::Formula::getValue( const double* vals ) const
{
    if ( !expr_ )
	return mUdf(double);

    Threads::Locker lckr( formlock_ );
    if ( inputsareseries_ && prevvals_.size() < maxRecShift() )
    {
	startNewSeries();
	for ( int ishift=prevvals_.size(); ishift<maxRecShift(); ishift++ )
	    prevvals_ += 0.;

    }

    for ( int ivar=0; ivar<inpidxs_.size(); ivar++ )
    {
	const int inpidx = inpidxs_[ivar];
	double val = 0.;
	if ( inpidx >= 0 )
	{
	    val = vals[ validxs_[ivar] ];
	    if ( inps_[inpidx].valunit_ )
		convValue( val, inps_[inpidx].valunit_,inps_[inpidx].formunit_);
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

    double retval = expr_->getValue();
    if ( inputsareseries_ )
	prevvals_ += retval;

    if ( outputvalunit_ )
	convValue( retval, outputformunit_, outputvalunit_ );

    return retval;
}


#define mDefInpKeybase \
    const BufferString inpkybase( IOPar::compKey(sKey::Input(),iinp) )
#define mOutUnKy IOPar::compKey(sKey::Output(),sKey::Unit())
#define mInpDefKy IOPar::compKey(inpkybase,"Def")
#define mInpUnKy IOPar::compKey(inpkybase,sKey::Unit())


void Math::Formula::fillPar( IOPar& iop ) const
{
    iop.update( sKeyExpression(), text_ );
    iop.update( mOutUnKy, outputformunit_ ? outputformunit_->name().buf()
					  : nullptr );

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
	const BufferString unstr = UnitOfMeasure::getUnitLbl( id.formunit_ );
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
    outputformunit_ = unstr.isEmpty() ? nullptr : UoMR().get( unstr );

    iop.get( sKeyRecStartVals(), recstartvals_ );

    for ( int iinp=0; iinp<inps_.size(); iinp++ )
    {
	InpDef& id = inps_[iinp];
	mDefInpKeybase;

	iop.get( mInpDefKy, id.inpdef_ );
	iop.get( mInpUnKy, unstr );
	id.formunit_ = UoMR().get( unstr );
    }
}


//Legacy:
mStartAllowDeprecatedSection
float Math::Formula::getValue( const float* vals, bool internuns ) const
{
    const int nrinpvals = nrValues2Provide();
    TypeSet<double> dvals;
    for ( int ival=0; ival<nrinpvals; ival++ )
	dvals += vals[ival];

    return sCast(float,getValue( dvals.arr(), internuns ) );
}
mStopAllowDeprecatedSection


double Math::Formula::getValue( const double* vals, bool internuns ) const
{
    if ( internuns )
    {
	auto& thisform = const_cast<Formula&>( *this );
	TypeSet<InpDef>& inps = thisform.inps_;
	for ( auto& inp : inps )
	{
	    if ( inp.formunit_ )
		inp.valunit_ = UoMR().getInternalFor(inp.formunit_->propType());
	}
	if ( outputformunit_ )
	    thisform.outputvalunit_ =
			UoMR().getInternalFor( outputformunit_->propType() );
    }

    return getValue( vals );
}

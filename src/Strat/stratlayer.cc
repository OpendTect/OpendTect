/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2010
-*/


#include "stratlayer.h"

#include "mathformula.h"
#include "mathproperty.h"
#include "stratreftree.h"


static const char* sKeyXPos = "XPos";

//------ LayerValue ------

BufferString Strat::LayerValue::dumpStr() const
{
    BufferString ret;
    if ( isSimple() )
	ret = toString( value() );
    else
    {
	mDynamicCastGet(const FormulaLayerValue&,flv,*this);
	IOPar iop; flv.fillPar( iop );
	iop.putTo( ret );
    }
    return ret;
}


Strat::FormulaLayerValue::FormulaLayerValue( const Math::Formula& form,
	const Layer& lay, const PropertyRefSelection& prs, float xpos )
    : form_(form)
    , lay_(lay)
    , myform_(false)
{
    setXPos( xpos );
    useForm( prs );
}


Strat::FormulaLayerValue::FormulaLayerValue( const IOPar& iop,
		const Layer& lay, const PropertyRefSelection& prs )
    : form_(*new Math::Formula(false,MathProperty::getSpecVars()))
    , lay_(lay)
    , myform_(true)
    , xpos_(0.f)
{
    const_cast<Math::Formula&>(form_).usePar( iop );

    const char* res = iop.find( sKeyXPos );
    if ( res )
	setXPos( toFloat(res) );

    useForm( prs );
}


Strat::FormulaLayerValue::FormulaLayerValue( const Math::Formula& form,
			    const Layer& lay, float xpos, bool cpform )
    : form_(cpform ? *new Math::Formula(form) : form)
    , myform_(cpform)
    , lay_(lay)
{
    setXPos( xpos );
}

void Strat::FormulaLayerValue::setXPos( float xpos )
{
    if ( xpos < 0.f ) xpos = 0.f; if ( xpos > 1.f ) xpos = 1.f;
    xpos_ = xpos;
}


void Strat::FormulaLayerValue::useForm( const PropertyRefSelection& prs )
{
    const int nrinps = form_.nrInputs();

    for ( int iinp=0; iinp<nrinps; iinp++ )
    {
	int inpidx = -1;
	float inpval = 0.f;
	if ( form_.isConst(iinp) )
	   inpval = float(form_.getConstVal(iinp));
	else if ( !form_.isSpec(iinp) )
	{
	    const char* pnm = form_.inputDef( iinp );
	    inpidx = prs.indexOf( pnm );
	    if ( inpidx < 0 )
	    {
		errmsg_ = tr( "%1 - Formula cannot be resolved:\n'%2'"
			      "\nCannot find '%3'")
			.arg(lay_.name()).arg( form_.text() ).arg( pnm );

		return;
	    }
	}

	inpidxs_ += inpidx; // not more than one because no shifts allowed
	inpvals_ += inpval;
    }
}


Strat::FormulaLayerValue::~FormulaLayerValue()
{
    if ( myform_ )
	delete &form_;
}


Strat::FormulaLayerValue* Strat::FormulaLayerValue::clone(
					const Layer* lay ) const
{
    auto* ret = new FormulaLayerValue( form_, lay ? *lay : lay_,
				       xpos_, myform_ );
    ret->inpidxs_ = inpidxs_;
    ret->inpvals_ = inpvals_;
    ret->errmsg_ = errmsg_;
    return ret;
}


float Strat::FormulaLayerValue::value() const
{
    if ( isBad() )
	return mUdf(float);

    const int nrinps = form_.nrInputs();
    for ( int iinp=0; iinp<nrinps; iinp++ )
    {
	const int inpidx = inpidxs_[iinp];
	if ( inpidx >= 0 )
	    inpvals_[iinp] = lay_.value( inpidx );
	else
	{
	    if ( form_.isSpec(iinp) )
		inpvals_[iinp] = form_.specIdx(iinp)<2 ? lay_.depth() : xpos_;
	    // consts are already filled
	}
    }

    return form_.getValue( inpvals_.arr() );
}


void Strat::FormulaLayerValue::fillPar( IOPar& iop ) const
{
    form_.fillPar( iop );
    iop.set( sKeyXPos, xpos_ );
}


//------ Layer ------

const PropertyRef& Strat::Layer::thicknessRef()
{
    return PropertyRef::thickness();
}


Strat::Layer::Layer( const LeafUnitRef& r )
    : ref_(&r)
{
    vals_.allowNull( true );
    setThickness( 0.0f );
}


Strat::Layer::Layer( const Strat::Layer& oth )
{
    vals_.allowNull( true );
    setThickness( 0.0f );
    *this = oth;
}


Strat::Layer::~Layer()
{
    deepErase( vals_ );
}


Strat::Layer& Strat::Layer::operator =( const Strat::Layer& oth )
{
    if ( this != &oth )
    {
	content_ = oth.content_;
	ref_ = oth.ref_;
	ztop_ = oth.ztop_;
	deepErase( vals_ );
	for ( int ival=0; ival<oth.vals_.size(); ival++ )
	{
	    const LayerValue* lv = oth.vals_[ival];
	    vals_ += lv ? lv->clone(this) : nullptr;
	}
    }

    return *this;
}


BufferString Strat::Layer::name() const
{
    return BufferString( unitRef().fullCode().buf() );
}


bool Strat::Layer::isMath( int ival ) const
{
    const LayerValue* lv = getLayerValue( ival );
    return lv ? !lv->isSimple() : false;
}


const Strat::LayerValue* Strat::Layer::getLayerValue( int ival ) const
{
    return vals_.validIdx( ival ) ? vals_[ival] : nullptr;
}


OD::Color Strat::Layer::dispColor( bool lith ) const
{
    return unitRef().dispColor( lith );
}


void Strat::Layer::getValues( TypeSet<float>& out ) const
{
    const int nrvals = nrValues();
    out.setSize( nrvals );
    for ( int ival=0; ival<nrvals; ival++ )
	out[ival] = value( ival );
}


const Strat::RefTree& Strat::Layer::refTree() const
{
    return unitRef().refTree();
}


Strat::Layer::ID Strat::Layer::id() const
{
    return unitRef().fullCode();
}


float Strat::Layer::value( int ival ) const
{
    const LayerValue* lv = vals_.validIdx(ival) ? vals_[ival] : nullptr;
    return lv ? lv->value() : mUdf(float);
}


#define mEnsureEnoughVals() while ( vals_.size() <= ival ) vals_ += nullptr


void Strat::Layer::setValue( int ival, float val )
{
    mEnsureEnoughVals();

    LayerValue* lv = vals_[ival];
    if ( lv && lv->isSimple() )
	static_cast<SimpleLayerValue*>(lv)->setValue( val );
    else
	setLV( ival, new SimpleLayerValue(val) );
}


void Strat::Layer::setValue( int ival, const Math::Formula& form,
			     const PropertyRefSelection& prs, float xpos )
{
    mEnsureEnoughVals();

    setLV( ival, new FormulaLayerValue(form,*this,prs,xpos) );
}


void Strat::Layer::setValue( int ival, const IOPar& iop,
				const PropertyRefSelection& prs )
{
    mEnsureEnoughVals();

    if ( iop.size() == 1 && iop.getKey(0) == sKey::Value() )
	setValue( ival, toFloat(iop.getValue(0)) );
    else
	setLV( ival, new FormulaLayerValue(iop,*this,prs) );
}


void Strat::Layer::setValue( int ival, LayerValue* lv )
{
    mEnsureEnoughVals();
    setLV( ival, lv );
}


void Strat::Layer::setLV( int ival, LayerValue* lv )
{
    delete vals_.replace( ival, lv );
}


float Strat::Layer::thickness() const
{
    float val = value( 0 );
    if ( val < 0 )
	{ pErrMsg("thickness < 0 found" ); val = 0.f; }
    return val;
}


void Strat::Layer::setXPos( float xpos )
{
    const int nrvals = vals_.size();
    for ( int ival=0; ival<nrvals; ival++ )
    {
	LayerValue* lv = vals_[ival];
	if ( lv )
	    lv->setXPos( xpos );
    }
}


void Strat::Layer::setThickness( float v )
{
    setValue( 0, v );
}


const Strat::Lithology& Strat::Layer::lithology() const
{
    return unitRef().getLithology();
}


const Strat::Content& Strat::Layer::content() const
{
    return content_ ? *content_ : Content::unspecified();
}


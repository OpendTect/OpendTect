/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratlayer.h"

#include "mathformula.h"
#include "mathproperty.h"
#include "stratreftree.h"
#include "unitofmeasure.h"


static const char* sKeyXPos = "XPos";
static const char* sKeyRelZ = "RelZ";

namespace Strat
{

//------ LayerValue ------

LayerValue::LayerValue()
{}


LayerValue::~LayerValue()
{}


// Temporary class for v7.0
class FormulaLayerValueWithRelZ : public FormulaLayerValue
{
public:
FormulaLayerValueWithRelZ( const Math::Formula& form,
	const Layer& lay, const PropertyRefSelection& prs, int outpridx,
	const Property::EvalOpts& eo )
    : FormulaLayerValue(form,lay,prs,outpridx,eo.relpos_)
{
    setRelZ( eo.relz_ );
}

FormulaLayerValueWithRelZ( const IOPar& iop,
		const Layer& lay, const PropertyRefSelection& prs,
		int outpridx )
    : FormulaLayerValue(iop,lay,prs,outpridx)
{
    const BufferString res = iop.find( sKeyRelZ );
    if ( !res.isEmpty() )
	setRelZ( res.toFloat() );
}

FormulaLayerValueWithRelZ( const Math::Formula& form,
			   const Layer& lay, float xpos, bool cpform )
    : FormulaLayerValue(form,lay,xpos,cpform)
{
}

FormulaLayerValueWithRelZ* clone( const Layer* lay ) const override
{
    auto* ret = new FormulaLayerValueWithRelZ( form_, lay ? *lay : lay_,
				       xpos_, myform_ );
    ret->inpidxs_ = inpidxs_;
    ret->inpvals_ = inpvals_;
    ret->errmsg_ = errmsg_;
    ret->relz_ = relz_;
    return ret;
}

void setRelZ( float relz )
{
    if ( relz < 0.f )
	relz = 0.f;
    if ( relz > 1.f )
	relz = 1.f;

    relz_ = relz;
}

float value() const override
{
    if ( isBad() )
	return mUdf(float);

    const int nrinps = form_.nrInputs();
    double* inpvals = inpvals_.arr();
    for ( int iinp=0; iinp<nrinps; iinp++ )
    {
	const int inpidx = inpidxs_[iinp];
	if ( inpidx >= 0 )
	    inpvals[iinp] = lay_.value( inpidx );
	else
	{
	    // consts are already filled
	    if ( form_.isSpec(iinp) )
		inpvals[iinp] = form_.specIdx(iinp)<2 ? lay_.depth()
				: (form_.specIdx(iinp)<4 ? relz_ : xpos_);
	}
    }

    return sCast(float,form_.getValue( inpvals ) );
}

void fillPar( IOPar& iop ) const
{
    FormulaLayerValue::fillPar( iop );
    iop.set( sKeyRelZ, relz_ );
}

protected:
    float		relz_ = 0.f;
};


BufferString LayerValue::dumpStr() const
{
    BufferString ret;
    if ( isSimple() )
	ret = toString( value() );
    else
    {
	IOPar iop;
	mDynamicCastGet(const FormulaLayerValueWithRelZ*,flvwithrelz,this);
	if ( flvwithrelz )
	    flvwithrelz->fillPar( iop );
	else
	{
	    mDynamicCastGet(const FormulaLayerValue&,flv,*this);
	    flv.fillPar( iop );
	}

	iop.putTo( ret );
    }

    return ret;
}


SimpleLayerValue::SimpleLayerValue( float val )
    : LayerValue()
    , val_ (val)
{}


SimpleLayerValue::~SimpleLayerValue()
{}


FormulaLayerValue::FormulaLayerValue( const Math::Formula& form,
	const Layer& lay, const PropertyRefSelection& prs, int outpridx,
	float xpos )
    : LayerValue()
    , form_(form)
    , lay_(lay)
    , myform_(false)
{
    setXPos( xpos );
    useForm( prs, outpridx );
}


FormulaLayerValue::FormulaLayerValue( const IOPar& iop,
		const Layer& lay, const PropertyRefSelection& prs,
		int outpridx )
    : LayerValue()
    , form_(*new Math::Formula(false,MathProperty::getSpecVars()))
    , lay_(lay)
    , myform_(true)
    , xpos_(0.f)
{
    const_cast<Math::Formula&>(form_).usePar( iop );

    const BufferString res = iop.find( sKeyXPos );
    if ( !res.isEmpty() )
	setXPos( res.toFloat() );

    useForm( prs, outpridx );
}


FormulaLayerValue::FormulaLayerValue( const Math::Formula& form,
			    const Layer& lay, float xpos, bool cpform )
    : LayerValue()
    , form_(cpform ? *new Math::Formula(form) : form)
    , myform_(cpform)
    , lay_(lay)
{
    setXPos( xpos );
}


void FormulaLayerValue::setXPos( float xpos )
{
    if ( xpos < 0.f )
	xpos = 0.f;
    if ( xpos > 1.f )
	xpos = 1.f;

    xpos_ = xpos;
}


void FormulaLayerValue::useForm( const PropertyRefSelection& prs,
					int outidx )
{
    const int nrinps = form_.nrInputs();
    auto& form = const_cast<Math::Formula&>( form_ );

    for ( int iinp=0; iinp<nrinps; iinp++ )
    {
	const PropertyRef* pr = nullptr;
	double inpval = 0.;
	if ( form.isConst(iinp) )
	   inpval = form_.getConstVal( iinp );
	else if ( !form_.isSpec(iinp) )
	{
	    const char* pnm = form_.inputDef( iinp );
	    pr = prs.getByName( pnm, false );
	    if ( pr )
		form.setInputValUnit( iinp, pr->unit() );
	    else
	    {
		const Mnemonic* mn = form.inputMnemonic( iinp );
		if ( mn )
		{
		    PropertyRefSelection inpprs( false );
		    for ( int iprop=0; iprop<prs.size(); iprop++ )
		    {
			const PropertyRef* inppr = prs.get(iprop);
			if ( iprop != outidx && inppr->isCompatibleWith(*mn) )
			    inpprs.add( inppr );
		    }
		    if ( !inpprs.isEmpty() )
		    {
			pr = inpprs.first();
			form.setInputValUnit( iinp, pr->unit() );
			if ( inpprs.size() > 1 )
			{
			    pErrMsgOnce("Found duplicated variable for form");
			}
		    }
		}

		if ( !pr )
		{
		    errmsg_ = tr( "%1 - Formula cannot be resolved:\n'%2'"
				  "\nCannot find '%3'")
			    .arg(lay_.name()).arg( form_.text() ).arg( pnm );

		    return;
		}
	    }
	}

	// not more than one because no shifts allowed
	inpidxs_ += pr ? prs.indexOf( pr ) : -1;
	inpvals_ += inpval;
    }

    const PropertyRef* pr = prs.validIdx(outidx) ? prs.get( outidx ) : nullptr;
    if ( pr )
	form.setOutputValUnit( pr->unit() );
}


FormulaLayerValue::~FormulaLayerValue()
{
    if ( myform_ )
	delete &form_;
}


FormulaLayerValue* FormulaLayerValue::clone(
					const Layer* lay ) const
{
    auto* ret = new FormulaLayerValue( form_, lay ? *lay : lay_,
				       xpos_, myform_ );
    ret->inpidxs_ = inpidxs_;
    ret->inpvals_ = inpvals_;
    ret->errmsg_ = errmsg_;
    return ret;
}


float FormulaLayerValue::value() const
{
    if ( isBad() )
	return mUdf(float);

    const int nrinps = form_.nrInputs();
    double* inpvals = inpvals_.arr();
    for ( int iinp=0; iinp<nrinps; iinp++ )
    {
	const int inpidx = inpidxs_[iinp];
	if ( inpidx >= 0 )
	    inpvals[iinp] = lay_.value( inpidx );
	else
	{
	    // consts are already filled
	    if ( form_.isSpec(iinp) )
		inpvals[iinp] = form_.specIdx(iinp)<4 ? lay_.depth() : xpos_;
	    //TODO: Implement relative depth, it is same as depth now.
	}
    }

    return sCast(float,form_.getValue( inpvals ) );
}


void FormulaLayerValue::fillPar( IOPar& iop ) const
{
    form_.fillPar( iop );
    iop.set( sKeyXPos, xpos_ );
}


//------ Layer ------

const PropertyRef& Layer::thicknessRef()
{
    return PropertyRef::thickness();
}


Layer::Layer( const LeafUnitRef& r )
    : ref_(&r)
{
    vals_.allowNull( true );
    setThickness( 0.0f );
}


Layer::Layer( const Layer& oth )
{
    vals_.allowNull( true );
    setThickness( 0.0f );
    *this = oth;
}


Layer::~Layer()
{
    deepErase( vals_ );
}


Layer& Layer::operator =( const Layer& oth )
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


BufferString Layer::name() const
{
    return BufferString( unitRef().fullCode().buf() );
}


bool Layer::isMath( int ival ) const
{
    const LayerValue* lv = getLayerValue( ival );
    return lv ? !lv->isSimple() : false;
}


const LayerValue* Layer::getLayerValue( int ival ) const
{
    return vals_.validIdx( ival ) ? vals_[ival] : nullptr;
}


OD::Color Layer::dispColor( bool lith ) const
{
    return unitRef().dispColor( lith );
}


void Layer::getValues( TypeSet<float>& out ) const
{
    const int nrvals = nrValues();
    out.setSize( nrvals );
    getValues( out.arr(), nrvals );
}


void Layer::getValues( float* out, int sz ) const
{
    const int nrvals = nrValues();
    if ( sz > nrvals )
	return;

    for ( int ival=0; ival<nrvals; ival++ )
	out[ival] = value( ival );
}


const RefTree& Layer::refTree() const
{
    return unitRef().refTree();
}


Layer::ID Layer::id() const
{
    return unitRef().fullCode();
}


float Layer::value( int ival ) const
{
    const LayerValue* lv = vals_.validIdx(ival) ? vals_[ival] : nullptr;
    return lv ? lv->value() : mUdf(float);
}


#define mEnsureEnoughVals() while ( vals_.size() <= ival ) vals_ += nullptr


void Layer::setValue( int ival, float val )
{
    mEnsureEnoughVals();

    LayerValue* lv = vals_[ival];
    if ( lv && lv->isSimple() )
	static_cast<SimpleLayerValue*>(lv)->setValue( val );
    else
    {
	lv = new SimpleLayerValue( val );
	setLV( ival, lv );
    }
}


void Layer::setValue( int ival, const Math::Formula& form,
			     const PropertyRefSelection& prs, float xpos )
{
    mEnsureEnoughVals();

    setLV( ival, new FormulaLayerValue(form,*this,prs,ival,xpos) );
}


void Layer::setValue( int ival, const Math::Formula& form,
			     const PropertyRefSelection& prs,
			     const Property::EvalOpts& eo )
{
    mEnsureEnoughVals();

    setLV( ival, new FormulaLayerValueWithRelZ(form,*this,prs,ival,eo) );
}


void Layer::setValue( int ival, const IOPar& iop,
				const PropertyRefSelection& prs )
{
    mEnsureEnoughVals();

    if ( iop.size() == 1 && iop.hasKey(sKey::Value()) )
	setValue( ival, iop.find(sKey::Value()).toFloat() );
    else
	setLV( ival, new FormulaLayerValue(iop,*this,prs,ival) );
}


void Layer::setValue( int ival, LayerValue* lv )
{
    mEnsureEnoughVals();
    setLV( ival, lv );
}


void Layer::setLV( int ival, LayerValue* lv )
{
    delete vals_.replace( ival, lv );
}


float Layer::thickness() const
{
    float val = value( 0 );
    if ( val < 0 )
	{ pErrMsg("thickness < 0 found" ); val = 0.f; }
    return val;
}


void Layer::setXPos( float xpos )
{
    const int nrvals = vals_.size();
    for ( int ival=0; ival<nrvals; ival++ )
    {
	LayerValue* lv = vals_[ival];
	if ( lv )
	    lv->setXPos( xpos );
    }
}


void Layer::setThickness( float v )
{
    setValue( 0, v );
}


const Lithology& Layer::lithology() const
{
    return unitRef().getLithology();
}


const Content& Layer::content() const
{
    return content_ ? *content_ : Content::unspecified();
}

} // namespace Strat

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ascstream.h"
#include "iopar.h"
#include "ioman.h"
#include "keystrs.h"
#include "mathspecvars.h"
#include "mathproperty.h"
#include "rockphysics.h"
#include "separstr.h"
#include "safefileio.h"
#include "unitofmeasure.h"
#include <typeinfo>

#include "hiddenparam.h"

//------- Property -------

mImplFactory1Param(Property,const PropertyRef&,Property::factory)

Property::Property( const PropertyRef& pr )
    : ref_(pr)
{
    mAttachCB( pr.unitChanged, Property::unitChangedCB );
}


Property::~Property()
{
    detachAllNotifiers();
}


const char* Property::name() const
{
    return ref_.name().buf();
}


bool Property::isEqualTo( const Property& oth ) const
{
    if ( typeid(*this) != typeid(oth) )
	return false;

    const BufferString mydef( def() ), othdef( oth.def() );
    return mydef == othdef;
}


bool Property::matches( const char* nm, bool matchaliases ) const
{
    return matches( nm, matchaliases, nullptr );
}


bool Property::matches( const char* nm, bool matchaliases,
			float* matchval ) const
{
    return ref().matches( nm, matchaliases, matchval );
}


void Property::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Name(), name() );
    iop.set( sKey::Type(), type() );
    iop.set( sKey::Value(), def() );
}


void Property::usePar( const IOPar& iop )
{
    const BufferString res = iop.find( sKey::Value() );
    if ( !res.isEmpty() )
	setDef( res );
}


Property* Property::get( const IOPar& iop )
{
    const BufferString nm = iop.find( sKey::Name() );
    if ( nm.isEmpty() )
	return nullptr;

    const PropertyRef* ref = PropertyRef::thickness().name() == nm
			   ? &PropertyRef::thickness()
			   : PROPS().getByName( nm, false );
    if ( !ref )
	ref = PROPS().getByName( nm, true );

    if ( !ref )
	return nullptr;

    BufferString typ = iop.find( sKey::Type() );
    if ( typ.isEmpty() )
	typ = ValueProperty::typeStr();

    Property* prop = factory().create( typ, *ref );
    if ( prop )
	prop->usePar( iop );

    return prop;
}


bool Property::init( const PropertySet& ) const
{
    mSelf().reset();
    return true;
}


void Property::unitChangedCB( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpack(const UnitOfMeasure*,olduom,cb);
    doUnitChange( olduom, ref_.unit() );
}


void Property::doUnitChange( const UnitOfMeasure* olduom,
			     const UnitOfMeasure* newuom )
{
    convValue( lastval_, olduom, newuom );
}


//------- ValueProperty ----------

ValueProperty::ValueProperty( const PropertyRef& pr )
    : Property(pr)
    , val_(pr.disp_.typicalrange_.center())
{
}


ValueProperty::ValueProperty( const PropertyRef& pr, float val )
    : Property(pr)
    , val_(val)
{
}


ValueProperty::~ValueProperty()
{
}


const char* ValueProperty::def() const
{
    static FileMultiString fms;
    fms = ::toString( val_ );
    const BufferString unitlbl( ref_.disp_.getUnitLbl() );
    if ( !unitlbl.isEmpty() )
	fms += unitlbl;

    return fms.buf();
}


void ValueProperty::setDef( const char* defstr )
{
    const FileMultiString fms( defstr );
    const int sz = fms.size();
    if ( sz > 0 )
	val_ = toFloat( fms[0] );

    const UnitOfMeasure* inpuom = sz > 1 ? UoMR().get( fms[1] ) : nullptr;
    convValue( val_, inpuom, ref_.unit() );
}


bool ValueProperty::isUdf() const
{
    return mIsUdf(val_);
}


float ValueProperty::gtVal( Property::EvalOpts ) const
{
    return val_;
}


void ValueProperty::doUnitChange( const UnitOfMeasure* olduom,
				  const UnitOfMeasure* newuom )
{
    Property::doUnitChange( olduom, newuom );
    convValue( val_, olduom, newuom );
}


//------- RangeProperty ----------

RangeProperty::RangeProperty( const PropertyRef& pr )
    : Property(pr)
    , rg_(pr.disp_.typicalrange_)
{
}

RangeProperty::RangeProperty( const PropertyRef& pr, const Interval<float>& rg )
    : Property(pr)
    , rg_(rg)
{
}


RangeProperty::~RangeProperty()
{
}


const char* RangeProperty::def() const
{
    if ( isUdf() )
	return "1e30`0.f";

    static FileMultiString fms;
    fms = ::toString( rg_.start );
    fms += ::toString( rg_.stop );
    const BufferString unitlbl( ref_.disp_.getUnitLbl() );
    if ( !unitlbl.isEmpty() )
	fms += unitlbl;

    return fms.buf();
}


void RangeProperty::setDef( const char* defstr )
{
    const FileMultiString fms( defstr );
    const int sz = fms.size();
    rg_.start = sz > 0 ? fms.getFValue( 0 ) : 1e30f;
    rg_.stop = sz > 1 ? fms.getFValue( 1 ) : 0.f;

    const UnitOfMeasure* inpuom = sz > 2 ? UoMR().get( fms[2] ) : nullptr;
    const UnitOfMeasure* targetuom = ref_.unit();
    convValue( rg_.start, inpuom, targetuom );
    convValue( rg_.stop, inpuom, targetuom );
}


bool RangeProperty::isUdf() const
{
    return mIsUdf(rg_.start);
}


float RangeProperty::gtAvgVal() const
{
    Interval<float> sanerg( rg_ );
    if ( mIsUdf(sanerg.start) )
	sanerg.start = sanerg.stop;
    else if ( mIsUdf(sanerg.stop) )
	sanerg.stop = sanerg.start;
    if ( ref().isThickness() )
    {
	if ( sanerg.start < 0.f ) sanerg.start = 0.f;
	if ( sanerg.stop < sanerg.start ) sanerg.stop = sanerg.start;
    }

    return 0.5f * (sanerg.start + sanerg.stop);
}


float RangeProperty::gtVal( Property::EvalOpts eo ) const
{
    if ( isUdf() )
	return mUdf(float);
    else if ( eo.isAvg() )
	return gtAvgVal();

    return rg_.start + eo.relpos_ * (rg_.stop - rg_.start);
}


void RangeProperty::doUnitChange( const UnitOfMeasure* olduom,
				  const UnitOfMeasure* newuom )
{
    Property::doUnitChange( olduom, newuom );
    convValue( rg_.start, olduom, newuom );
    convValue( rg_.stop, olduom, newuom );
}



//------- MathProperty ----------

static const PropertyRef& depthPropRef()
{
    mDefineStaticLocalObject( PtrMan<PropertyRef>, depthpropref,
			= new PropertyRef( Mnemonic::distance(), "Depth" ) );
    return *depthpropref;
}

static const ValueProperty& depthProp()
{
    mDefineStaticLocalObject( PtrMan<ValueProperty>, depthprop,
			 = new ValueProperty( depthPropRef(), 0.f) );
    return *depthprop;
}

static const PropertyRef& reldepthPropRef()
{
    mDefineStaticLocalObject( PtrMan<PropertyRef>, reldepthpropref,
			= new PropertyRef( Mnemonic::distance(), "RelDepth" ) );
    return *reldepthpropref;
}

static const ValueProperty& relDepthProp()
{
    mDefineStaticLocalObject( PtrMan<ValueProperty>, reldepthprop,
		     = new ValueProperty( reldepthPropRef(), 0.f ) );
    return *reldepthprop;
}

static const PropertyRef& xposPropRef()
{
    mDefineStaticLocalObject( PtrMan<PropertyRef>, xpospropref, = nullptr );
    if ( !xpospropref )
    {
	auto* ret = new PropertyRef( Mnemonic::volume(), "XPos" );
	if ( xpospropref.setIfNull(ret,true) )
	{
	    ret->disp_.range_ = Interval<float>( 0.f ,1.f );
	    ret->disp_.typicalrange_ = Interval<float>( 0.f ,1.f );
	}
    }
    return *xpospropref;
}


static const ValueProperty& xposProp()
{
    mDefineStaticLocalObject( PtrMan<ValueProperty>, xposprop,
			 = new ValueProperty( xposPropRef(), 0.f ) );
    return *xposprop;
}


static const StringView sKeyMathForm( "Formula: " );

const Math::SpecVarSet& MathProperty::getSpecVars()
{
    mDefineStaticLocalObject( Math::SpecVarSet, svs, );

    if ( svs.isEmpty() )
    {
	const Mnemonic* distmn = &Mnemonic::distance();
	svs.add( "Depth", "Depth", true, distmn );
	svs.add( "Z", "Depth", true, distmn );
	svs.add( "RelDepth", "Relative Depth", true, distmn );
	svs.add( "RelZ", "Relative Depth", true, distmn );
	svs.add( "XPos", "Relative horizontal position (0-1)" );
    }

    return svs;
}


HiddenParam<MathProperty,int> mathpropisfromrphymgr_(0);

MathProperty::MathProperty( const PropertyRef& pr, const char* df )
    : Property(pr)
    , form_(*new Math::Formula(false,getSpecVars()))
{
    mathpropisfromrphymgr_.setParam( this, 0 );
    form_.setOutputMnemonic( &mn() );
    inps_.allowNull( true );
    if ( df && *df )
	setDef( df );
}


MathProperty::MathProperty( const MathProperty& oth )
    : Property(oth.ref())
    , form_(*new Math::Formula(oth.form_))
    , inps_(oth.inps_)
{
    mathpropisfromrphymgr_.setParam( this,
			mathpropisfromrphymgr_.getParam( &oth ) );
}


MathProperty::~MathProperty()
{
    delete &form_;
    mathpropisfromrphymgr_.removeParam( this );
}


bool MathProperty::hasCyclicalDependency( BufferStringSet& parentnms ) const
{
    parentnms.add( name() );
    for ( int iinp=0; iinp<inps_.size(); iinp++ )
    {
	const Property* inpprop = inps_[iinp];
	if ( inpprop && parentnms.isPresent(inpprop->name()) )
	{
	    errmsg_ =
		tr( "Input '%1' is dependent on itself" ).arg(name());
	    return true;
	}

	mDynamicCastGet(const MathProperty*,mathprop,inpprop)
	if ( !mathprop ) continue;

	BufferStringSet newthrpnms( parentnms );
	if ( mathprop->hasCyclicalDependency(newthrpnms) )
	{
	    errmsg_ =
		tr( "Input '%1' is dependent on itself" ).arg(name());
	    return true;
	}
    }

    return false;
}


bool MathProperty::init( const PropertySet& ps ) const
{
    if ( !form_.isOK() )
    {
	errmsg_ = tr( "No valid definition for '%1'" ).arg( name() );
	return false;
    }

    const bool isfromrockphys_ = mathpropisfromrphymgr_.getParam( this ) == 1;
    const int nrinps = form_.nrInputs();
    inps_.erase();
    for ( int iinp=0; iinp<nrinps; iinp++ )
    {
	const Property* prop = nullptr;
	if ( form_.isSpec(iinp) )
	{
	    const int specidx = form_.specIdx(iinp);
	    if ( specidx > 4 )
		prop = &xposProp();
	    else
		prop = specidx < 2 ? &depthProp() : &relDepthProp();
	}
	else if ( !form_.isConst(iinp) )
	{
	    const char* inpnm = form_.inputDef( iinp );
	    const Mnemonic* inpmn = form_.inputMnemonic( iinp );
	    if ( inpmn )
		prop = ps.getByMnemonic( *inpmn );
	    if ( !prop )
		prop = ps.getByName( inpnm );

	    if ( !prop )
	    {
		errmsg_ =
		    tr( "Missing input '%1' for the calculation of '%2'" )
			.arg( inpnm ).arg( name() );
		return false;
	    }
	}

	inps_ += prop;
	if ( prop )
	{
	    form_.setInputValUnit( iinp, prop->unit() );
	    if ( !isfromrockphys_ )
		form_.setInputMnemonic( iinp, &prop->mn() );
	}
    }

    form_.setOutputValUnit( unit() );

    Property::init( ps );
    if ( !isfromrockphys_ && !form_.hasFixedUnits() )
    {
	if ( ROCKPHYSFORMS().getMatching(mSelf().form_) )
	{
	    mathpropisfromrphymgr_.setParam( &mSelf(), 1 );
	    return init( ps );
	}
    }

    return true;
}


bool MathProperty::dependsOn( const Property& p ) const
{
    if ( &p == this )
	return true;

    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const Property* inp = inps_[idx];
	if ( inp && inp->dependsOn(p) )
	    return true;
    }
    return false;
}


const char* MathProperty::def() const
{
    IOPar iop;
    form_.fillPar( iop );
    BufferString iopdef;
    iop.putTo( iopdef );
    fulldef_.set( sKeyMathForm ).add( iopdef );
    return fulldef_.buf();
}


void MathProperty::setDef( const char* defstr )
{
    inps_.erase();
    form_.clearInputDefs();

    if ( !StringView(defstr).startsWith(sKeyMathForm) )
	{ setPreV5Def( defstr ); return; }

    defstr += sKeyMathForm.size();
    IOPar iop; iop.getFrom( defstr );
    form_.usePar( iop );
    if ( !form_.hasFixedUnits() )
	mathpropisfromrphymgr_.setParam( this,
	    ROCKPHYSFORMS().getMatching( form_ ) ? 1 : 0 );
}


void MathProperty::setPreV5Def( const char* inpstr )
{
    FileMultiString fms( inpstr );
    const int fmssz = fms.size();
    BufferString defstr( fms[0] );
    if ( defstr.isEmpty() )
	return;

    // Variables were the property names, need to replace them with "v_i_xx"
    const PropertyRefSet& props = PROPS();
    PropertyRefSelection prs;
    for ( int idx=props.size()-1; idx>-2; idx-- )
    {
	const PropertyRef& pr = idx<0 ? PropertyRef::thickness()
				      : *props.get( idx );
	const OD::String& propnm = pr.name();
	if ( !defstr.contains(propnm.buf()) )
	    continue;

	prs.add( &pr );
	BufferString cleanpropnm( propnm ); cleanpropnm.clean();
	BufferString varnm( "v_", prs.size(), "_" );
	varnm.add( cleanpropnm );

	defstr.replace( propnm.buf(), varnm );
    }

    form_.setText( defstr );
    if ( !form_.isOK() )
	return;

    const int nrinps = form_.nrInputs();
    for ( int iinp=0; iinp<nrinps; iinp++ )
    {
	const BufferString varnm( form_.variableName(iinp) );
	if ( !varnm.startsWith("v_") )
	{
	    ErrMsg( BufferString("Could not fully use Pre-V5 math property."
			"\nPlease replace '", inpstr, "'" ) );
	    continue;
	}

	BufferString varidxstr( varnm.buf() + 2 );
	varidxstr.replace( '_', '\0' );
	const int propidx = toInt( varidxstr.buf() ) - 1;
	if ( !prs.validIdx(propidx) )
	    { pErrMsg("Huh"); continue; }

	const PropertyRef& pr = *prs.get(propidx);
	form_.setInputDef( iinp, pr.name() );
	form_.setInputMnemonic( iinp, &pr.mn() );

	const UnitOfMeasure* uom = nullptr;
	if ( fmssz > 2 + iinp )
	    uom = UoMR().get( fms[2+iinp] );

	form_.setInputFormUnit( iinp, uom );
    }

    if ( fmssz > 1 )
	form_.setOutputFormUnit( UoMR().get(fms[1]) );

    if ( !form_.hasFixedUnits() )
	mathpropisfromrphymgr_.setParam( this,
		ROCKPHYSFORMS().getMatching( form_ ) ? 1 : 0 );
}


bool MathProperty::isUdf() const
{
    return !form_.isOK();
}


float MathProperty::gtVal( Property::EvalOpts eo ) const
{
    if ( isUdf() )
	return mUdf(float);

    EvalOpts nonmatheo( eo );
    if ( nonmatheo.valopt_ == EvalOpts::New )
	nonmatheo.valopt_ = EvalOpts::Prev;
    const EvalOpts matheo( eo );

    TypeSet<double> inpvals;
    for ( int iinp=0; iinp<inps_.size(); iinp++ )
    {
	const Property* prop = inps_[iinp];
	double val = 0.;
	if ( !prop )
	    val = form_.getConstVal( iinp );
	else
	{
	    if ( prop == &xposProp() )
		val = eo.relpos_;
	    else if ( prop == &depthProp() || prop == &relDepthProp() )
	    {
		val = prop == &depthProp() ? eo.absz_ : eo.relz_;
		if ( SI().depthsInFeet() )
		    val *= mToFeetFactorF;
	    }
	    else
		val = prop->value( prop->isFormula() ? matheo : nonmatheo );
	}

	inpvals += val;
    }

    return sCast(float, form_.getValue( inpvals.arr() ) );
}


const Mnemonic* MathProperty::inputMnemonic( int iinp ) const
{
    if ( iinp < 0 || iinp >= nrInputs() )
	return nullptr;

    if ( inps_.validIdx(iinp) )
    {
	const Property* inp = inps_[iinp];
	if ( inp )
	    return &inp->mn();
    }

    PropertyRefSelection prs( true );
    prs.append( PropertyRefSelection( mn() ) );
    const char* propnm = form_.inputDef( iinp );
    const PropertyRef* pr = prs.getByName( propnm );
    return pr ? &pr->mn() : nullptr;
}


Mnemonic::StdType MathProperty::inputType( int iinp ) const
{
    const Mnemonic* mn = inputMnemonic( iinp );
    return mn ? mn->stdType() : Mnemonic::undef().stdType();
}


void MathProperty::setInput( int iinp, const Property* p )
{
    if ( !inps_.validIdx(iinp) )
	{ pErrMsg("idx out of range"); return; }

    if ( p && p->dependsOn(*this) )
    {
	BufferString msg( "Invalid cyclic dependency for ", name() );
	ErrMsg( msg );
	p = nullptr;
    }

    inps_.replace( iinp, p );
    form_.setInputValUnit( iinp, p->unit() );
}


const char* MathProperty::formText( bool usrdisp ) const
{
    return usrdisp ? form_.userDispText() : form_.text();
}


int MathProperty::nrInputs() const
{
    return form_.nrInputs();
}


const char* MathProperty::inputName( int iinp ) const
{
    return form_.inputDef( iinp );
}


const UnitOfMeasure* MathProperty::inputUnit( int iinp ) const
{
    return form_.inputFormUnit( iinp );
}


bool MathProperty::isConst( int iinp ) const
{
    return form_.isConst( iinp );
}


void MathProperty::setUnit( const UnitOfMeasure* )
{}


const UnitOfMeasure* MathProperty::unit() const
{
    return Property::unit();
}


void MathProperty::doUnitChange( const UnitOfMeasure*,
				 const UnitOfMeasure* )
{}


//------- PropertySet ----------

PropertySet::PropertySet()
    : ObjectSet<Property>()
{
}


PropertySet::PropertySet( const PropertySet& oth )
    : ObjectSet<Property>()
{
    *this = oth;
}


PropertySet::PropertySet( const PropertyRefSelection& prs )
    : ObjectSet<Property>()
{
    for ( const auto* pr : prs )
	add( new ValueProperty(*pr) );
}


PropertySet::~PropertySet()
{
    erase();
}


PropertySet& PropertySet::operator =( const PropertySet& ps )
{
    deepCopyClone( *this, ps );

    return *this;
}


Property* PropertySet::getByName( const char* nm, bool matchaliases )
{
    const Property* ret =
	const_cast<const PropertySet*>(this)->getByName( nm, matchaliases );
    return const_cast<Property*>( ret );
}


const Property* PropertySet::getByName( const char* nm,
					bool matchaliases ) const
{
    PropertyRefSelection prs( false );
    for ( const auto* prop : *this )
	prs.add( &prop->ref() );

    const PropertyRef* pr = PropertyRefSet::getByName( nm, prs, matchaliases );
    if ( !pr )
	return nullptr;

    for ( const auto* prop : *this )
    {
	if ( &prop->ref() == pr )
	    return prop;
    }

    return nullptr;
}


const Property* PropertySet::getByMnemonic( const Mnemonic& mn, int occ ) const
{
    for ( const auto* prop : *this )
    {
	if ( prop->ref().isCompatibleWith(mn) )
	{
	    occ--;
	    if ( occ < 0 )
		return prop;
	}
    }

    return nullptr;
}


PropertySet& PropertySet::doAdd( Property* prop )
{
    if ( !prop || getByName(prop->name(),false) )
    {
	delete prop;
	return *this;
    }

    ObjectSet<Property>::doAdd( prop );
    return *this;
}


Property* PropertySet::set( Property* prop )
{
    if ( !prop )
	return nullptr;

    Property* oldprop = getByName( prop->name(), false );
    if ( oldprop )
	delete replace( indexOf(oldprop), prop );
    else
	add( prop );

    return prop;
}


bool PropertySet::prepareUsage() const
{
    for ( const auto* prop : *this )
    {
	if ( !prop->init(*this) )
	    { errmsg_ = prop->errMsg(); return false; }
    }

    for ( const auto* prop : *this )
    {
	if ( !prop->isFormula() )
	    continue;

	mDynamicCastGet(const MathProperty*,mathprop,prop);
	BufferStringSet inputnms;
	if ( mathprop->hasCyclicalDependency(inputnms) )
	{
	    errmsg_ = prop->errMsg();
	    return false;
	}
    }

    return true;
}


void PropertySet::resetMemory()
{
    for ( auto* prop : *this )
	prop->reset();
}

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/


#include "mathproperty.h"
#include "propertyref.h"
#include "mathformula.h"
#include "mathspecvars.h"
#include "unitofmeasure.h"
#include "keystrs.h"
#include "iopar.h"
#include "separstr.h"
#include <typeinfo>


Property::Property( const PropertyRef& pr )
    : ref_(pr)
    , lastval_(mUdf(float))
    , mn_(MNC().find(pr.getMnemonic()))
{}


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


void Property::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Name(), name() );
    iop.set( sKey::Type(), type() );
    iop.set( sKey::Value(), def() );
}


void Property::usePar( const IOPar& iop )
{
    const char* res = iop.find( sKey::Value() );
    if ( res && *res )
	setDef( res );
}


Property* Property::get( const IOPar& iop )
{
    const char* nm = iop.find( sKey::Name() );
    if ( !nm || !*nm ) return 0;

    const PropertyRef* ref = PROPS().find( nm );
    if ( !ref && PropertyRef::thickness().name() == nm )
	ref = &PropertyRef::thickness();
    if ( !ref ) return 0;

    const char* typ = iop.find( sKey::Type() );
    if ( !typ || !*typ ) typ = ValueProperty::typeStr();
    Property* prop = factory().create( typ, *ref );
    if ( prop )
	prop->usePar( iop );

    return prop;
}


bool Property::init(const PropertySet&) const
{
    const_cast<Property*>(this)->reset();
    return true;
}


//------- ValueProperty ----------


const char* ValueProperty::def() const
{
    static FileMultiString fms;
    fms = ::toString( val_ );
    const BufferString unitlbl( ref_.getStorUnitLbl() );
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
    convValue( val_, inpuom, ref_.storUnit() );
}


bool ValueProperty::isUdf() const
{
    return mIsUdf(val_);
}


float ValueProperty::gtVal( Property::EvalOpts ) const
{
    return val_;
}


//------- RangeProperty ----------


const char* RangeProperty::def() const
{
    if ( isUdf() )
	return "1e30`0";

    static FileMultiString fms;
    fms = ::toString( rg_.start );
    fms += ::toString( rg_.stop );
    const BufferString unitlbl( ref_.getStorUnitLbl() );
    if ( !unitlbl.isEmpty() )
	fms += unitlbl;

    return BufferString( fms.buf() );
}


void RangeProperty::setDef( const char* defstr )
{
    const FileMultiString fms( defstr );
    const int sz = fms.size();
    rg_.start = sz > 0 ? fms.getFValue( 0 ) : 1e30f;
    rg_.stop = sz > 1 ? fms.getFValue( 1 ) : 0.f;

    const UnitOfMeasure* inpuom = sz > 2 ? UoMR().get( fms[2] ) : nullptr;
    const UnitOfMeasure* targetuom = ref_.storUnit();
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
	if ( sanerg.start < 0 ) sanerg.start = 0;
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


//------- MathProperty ----------

static const PropertyRef& depthPropRef()
{
    mDefineStaticLocalObject( PtrMan<PropertyRef>, depthpropref,
				= new PropertyRef("depth", PropertyRef::Dist) );
    return *depthpropref;
}

static const ValueProperty& depthProp()
{
    mDefineStaticLocalObject( PtrMan<ValueProperty>, depthprop,
				 = new ValueProperty(depthPropRef(), 0) );
    return *depthprop;
}

static const PropertyRef& relDepthPropRef()
{
    mDefineStaticLocalObject( PtrMan<PropertyRef>, reldepthpropref,
			    = new PropertyRef("RelDepth", PropertyRef::Dist) );
    return *reldepthpropref;
}

static const ValueProperty& relDepthProp()
{
    mDefineStaticLocalObject( PtrMan<ValueProperty>, reldepthprop,
				 = new ValueProperty(relDepthPropRef(), 0) );
    return *reldepthprop;
}

static const PropertyRef& xposPropRef()
{
    mDefineStaticLocalObject( PtrMan<PropertyRef>, xpospropref,
			    = new PropertyRef("XPos", PropertyRef::Volum) );
    return *xpospropref;
}

static const ValueProperty& xposProp()
{
    mDefineStaticLocalObject( PtrMan<ValueProperty>, xposprop,
				 = new ValueProperty(xposPropRef(), 0) );
    return *xposprop;
}
static const FixedString sKeyMathForm( "Formula: " );


const Math::SpecVarSet& MathProperty::getSpecVars()
{
    mDefineStaticLocalObject( Math::SpecVarSet, svs, );

    if ( svs.isEmpty() )
    {
	svs.add( "Depth", "Depth", true, PropertyRef::Dist );
	svs.add( "Z", "Depth", true, PropertyRef::Dist );
	svs.add( "RelDepth", "Relative Depth", true, PropertyRef::Dist );
	svs.add( "RelZ", "Relative Depth", true, PropertyRef::Dist );
	svs.add( "XPos", "Relative horizontal position (0-1)" );
    }

    return svs;
}


MathProperty::MathProperty( const PropertyRef& pr, const char* df )
    : Property(pr)
    , form_(*new Math::Formula(false,getSpecVars()))
{
    inps_.allowNull( true );
    if ( df && *df )
	setDef( df );
}


MathProperty::MathProperty( const MathProperty& oth )
    : Property(oth.ref())
    , form_(*new Math::Formula(oth.form_))
    , inps_(oth.inps_)
{
}


MathProperty::~MathProperty()
{
    delete &form_;
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

    const int nrinps = form_.nrInputs();
    inps_.erase();
    for ( int iinp=0; iinp<nrinps; iinp++ )
    {
	const Property* prop = 0;
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
	    prop = ps.find( inpnm );
	    if ( !prop )
	    {
		errmsg_ =
		    tr( "Missing input '%1' for '%2'" ).arg(name()).arg(inpnm);
		return false;
	    }
	}

	inps_ += prop;
    }

    const_cast<MathProperty*>(this)->reset();

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
    inps_.erase(); form_.clearInputDefs();

    if ( !FixedString(defstr).startsWith(sKeyMathForm) )
	{ setPreV5Def( defstr ); return; }

    defstr += sKeyMathForm.size();
    IOPar iop; iop.getFrom( defstr );
    form_.usePar( iop );
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
    BufferStringSet propnms;
    for ( int idx=props.size()-1; idx>-2; idx-- )
    {
	const char* propnm = idx<0 ? "Thickness" : props[idx]->name().buf();
	if ( !defstr.contains(propnm) )
	    continue;
	propnms.add( propnm );

	BufferString cleanpropnm( propnm ); cleanpropnm.clean();
	BufferString varnm( "v_", propnms.size(), "_" );
	varnm.add( cleanpropnm );

	defstr.replace( propnm, varnm );
    }
    form_.setText( defstr );
    if ( !form_.isOK() )
	return;

    const int nrinps = form_.nrInputs();
    for ( int iinp=0; iinp<nrinps; iinp++ )
    {
	BufferString varnm( form_.variableName(iinp) );
	if ( !varnm.startsWith("v_") )
	{
	    ErrMsg( BufferString("Could not fully use Pre-V5 math property."
			"\nPlease replace '", inpstr, "'" ) );
	    continue;
	}

	BufferString varidxstr( varnm.buf() + 2 );
	varidxstr.replace( '_', '\0' );
	const int propidx = toInt( varidxstr.buf() ) - 1;
	if ( !propnms.validIdx(propidx) )
	    { pErrMsg("Huh"); continue; }

	form_.setInputDef( iinp, propnms.get(propidx) );
    }

    if ( fmssz > 1 )
	form_.setOutputUnit( UoMR().get(fms[1]) );

    for ( int iinp=0; iinp<form_.nrInputs(); iinp++ )
    {
	const UnitOfMeasure* uom = 0;
	if ( fmssz > 2 + iinp )
	    uom = UoMR().get( fms[2+iinp] );
	form_.setInputUnit( iinp, uom );
    }
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

    TypeSet<float> inpvals;
    for ( int iinp=0; iinp<inps_.size(); iinp++ )
    {
	const Property* prop = inps_[iinp];
	float val;
	if ( !prop )
	    val = (float)form_.getConstVal( iinp );
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
	    {
		mDynamicCastGet(const MathProperty*,mp,prop)
		val = prop->value( mp ? matheo : nonmatheo );
		const UnitOfMeasure* uom = form_.inputUnit( iinp );
		if ( uom )
		    val = uom->getUserValueFromSI( val );
	    }
	}
	inpvals += val;
    }

    return form_.getValue( inpvals.arr() );
}


PropertyRef::StdType MathProperty::inputType( int iinp ) const
{
    if ( iinp < 0 || iinp >= nrInputs() )
	return PropertyRef::Other;

    if ( inps_.validIdx(iinp) )
    {
	const Property* inp = inps_[iinp];
	if ( inp )
	    return inp->ref().stdType();
    }

    PropertyRefSelection prs( PropertyRefSelection::getAll(true,&ref()) );
    const char* propnm = form_.inputDef( iinp );
    const PropertyRef* pr = prs.getByName( propnm );
    if ( pr )
	return pr->stdType();

    return PropertyRef::Other;
}


void MathProperty::setInput( int iinp, const Property* p )
{
    if ( !inps_.validIdx(iinp) )
	{ pErrMsg("idx out of range"); return; }

    if ( p && p->dependsOn(*this) )
    {
	BufferString msg( "Invalid cyclic dependency for ", ref().name() );
	ErrMsg( msg );
	p = 0;
    }

    inps_.replace( iinp, p );
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
    return form_.inputUnit( iinp );
}


bool MathProperty::isConst( int iinp ) const
{
    return form_.isConst( iinp );
}


void MathProperty::setUnit( const UnitOfMeasure* uom )
{
    form_.setOutputUnit( uom );
}


const UnitOfMeasure* MathProperty::unit() const
{
    return form_.outputUnit();
}


//------- PropertySet ----------

PropertySet::PropertySet( const PropertyRefSelection& prs )
{
    for ( int idx=0; idx<prs.size(); idx++ )
	props_ += new ValueProperty( *prs[idx] );
}


PropertySet& PropertySet::operator =( const PropertySet& ps )
{
    if ( this != &ps )
    {
	erase();
	for ( int idx=0; idx<ps.size(); idx++ )
	    props_ += ps.get(idx).clone();
    }
    return *this;
}


void PropertySet::replace( int idx, Property* p )
{
    if ( p )
	delete props_.replace( idx, p );
}


int PropertySet::indexOf( const char* nm, bool matchaliases ) const
{
    if ( !nm || !*nm ) return -1;

    for ( int idx=0; idx<props_.size(); idx++ )
    {
	const Property& p = *props_[idx];
	if ( p.ref().name() == nm )
	    return idx;
    }
    if ( matchaliases )
    {
	for ( int idx=0; idx<props_.size(); idx++ )
	{
	    const Property& p = *props_[idx];
	    if ( p.ref().isKnownAs(nm) )
		return idx;
	}
    }

    return -1;
}


Property* PropertySet::fnd( const char* nm, bool ma ) const
{
    const int idx = indexOf(nm,ma);
    return idx < 0 ? 0 : const_cast<Property*>( props_[idx] );
}


int PropertySet::indexOf( PropertyRef::StdType st, int occ ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Property& pr = *props_[idx];
	if ( pr.ref().hasType(st) )
	{
	    occ--;
	    if ( occ < 0 )
		return idx;
	}
    }

    return -1;
}


bool PropertySet::add( Property* p )
{
    if ( !p ) return false;
    if ( indexOf(p->name(),false) >= 0 )
	return false;
    props_ += p;
    return true;
}


int PropertySet::set( Property* p )
{
    if ( !p ) return -1;

    int idxof = indexOf( p->name(), false );
    if ( idxof >= 0 )
	delete props_.replace( idxof, p );
    else
    {
	idxof = props_.size();
	props_ += p;
    }
    return idxof;
}


void PropertySet::remove( int idx )
{
    delete props_.removeSingle( idx );
}


bool PropertySet::prepareUsage() const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( !props_[idx]->init(*this) )
	    { errmsg_ = props_[idx]->errMsg(); return false; }
    }

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(const MathProperty*,mathprop,props_[idx]);
	if ( !mathprop ) continue;
	BufferStringSet inputnms;
	if ( mathprop->hasCyclicalDependency(inputnms) )
	{
	    errmsg_ = props_[idx]->errMsg();
	    return false;
	}
    }
    return true;
}


void PropertySet::resetMemory()
{
    for ( int idx=0; idx<size(); idx++ )
	props_[idx]->reset();
}


void PropertySet::getPropertiesOfRefType( PropertyRef::StdType proptype,
					  ObjectSet<Property>& resultset ) const
{
    for ( int idx=0; idx<props_.size(); idx++ )
	if ( props_[idx] && props_[idx]->ref().hasType( proptype ) )
	    resultset += const_cast<PropertySet*>(this)->props_[idx];
}

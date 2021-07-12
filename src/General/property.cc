/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

#include "ascstream.h"
#include "iopar.h"
#include "ioman.h"
#include "keystrs.h"
#include "mathproperty.h"
#include "mathformula.h"
#include "mathspecvars.h"
#include "propertyref.h"
#include "separstr.h"
#include "safefileio.h"
#include "unitofmeasure.h"
#include <typeinfo>


static const char* filenamebase = "Properties";
static const char* sKeyMnemonic = "Mnemonic";
static const char* sKeyDefaultValue = "DefaultValue";
static const char* sKeyDefinition = "Definition";

mImplFactory2Param(Property,const char*,const Mnemonic&,Property::factory)

static bool thickness_propman_is_deleting_thickness = false;

class ThicknessProperty : public ValueProperty
{
public:

ThicknessProperty()
    : ValueProperty( sKey::Thickness(), Mnemonic::distance() )
{
    disp_.range_ = Interval<float>(0,10000);
    disp_.typicalrange_ = Interval<float>(0,100);
}

private:

~ThicknessProperty()
{
    if ( !thickness_propman_is_deleting_thickness )
	pErrMsg( "Fatal error, should not delete 'Thickness'." );
}

friend struct Prop_Thick_Man;

};


struct Prop_Thick_Man : public CallBacker
{

Prop_Thick_Man()
{
    ref_ = new ThicknessProperty();
    setZUnit();
    IOM().afterSurveyChange.notify( mCB(this,Prop_Thick_Man,setZUnit) );
}


~Prop_Thick_Man()
{
    thickness_propman_is_deleting_thickness = true;
    delete ref_;
}


void setDefaultVals( const Property* otherthref )
{
    ref_->defval_ = otherthref->defval_;
}


void setZUnit( CallBacker* cb=0 )
{
    ref_->disp_.unit_ =
	UnitOfMeasure::zUnitAnnot( false, true, false ).getFullString();
}

    ThicknessProperty*	     ref_;
};


Prop_Thick_Man* getProp_Thick_Man()
{
    mDefineStaticLocalObject( PtrMan<Prop_Thick_Man>, ptm,
			      = new Prop_Thick_Man );
    return ptm;
}


const Property& Property::thickness()
{
    return *getProp_Thick_Man()->ref_;
}


void Property::setThickness( const Property* thref )
{
    getProp_Thick_Man()->setDefaultVals( thref );;
}


Property& Property::operator =( const Property& pr )
{
    if ( this != &pr )
    {
	setName( pr.name() );
	mn_ = const_cast<Mnemonic&>( pr.mnem() );
	disp_ = pr.disp_;
    }

    return *this;
}


void Property::setMnemonic( const Mnemonic& mn )
{
    mn_ = mn;
    disp_ = mn.disp_;
}


float Property::commonValue() const
{
    if ( defval_ && defval_->isValue() )
	return defval_->value();

    const bool udf0 = mIsUdf(disp_.range_.start);
    const bool udf1 = mIsUdf(disp_.range_.stop);
    if ( udf0 && udf1 )
	return 0;

    if ( udf0 || udf1 )
	return udf0 ? disp_.range_.stop : disp_.range_.start;

    return disp_.range_.center();
}


bool Property::isKnownAs( const char* nm ) const
{
    if ( !nm || !*nm )
	return false;

    if ( caseInsensitiveEqual(nm,name().buf(),0) )
	return true;

    return false;
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
    iop.set( sKeyMnemonic, mn_.name() );
    iop.set( sKey::Color(), disp_.color_ );

    Interval<float> vintv( disp_.range_ );
    const UnitOfMeasure* uom = UoMR().get( disp_.unit_ );
    if ( uom )
    {
	if ( !mIsUdf(vintv.start) )
	    vintv.start = uom->getUserValueFromSI(vintv.start);
	if ( !mIsUdf(vintv.stop) )
	    vintv.stop = uom->getUserValueFromSI(vintv.stop);
    }

    FileMultiString fms;
    fms += ::toString( vintv.start );
    fms += ::toString( vintv.stop );
    if ( !disp_.unit_.isEmpty() )
	fms += disp_.unit_;
    iop.set( sKey::Range(), fms );

    iop.set( sKey::Type(), type() );
    iop.set( sKey::Value(), def() );
}


void Property::getDataUsingPar( const IOPar& iop )
{
    iop.get( sKey::Color(), disp_.color_ );
    FileMultiString fms( iop.find(sKey::Range()) );
    int sz = fms.size();
    if ( sz > 1 )
    {
	disp_.range_.start = fms.getFValue( 0 );
	disp_.range_.stop = fms.getFValue( 1 );
	if ( sz > 2 )
	{
	    disp_.unit_ = fms[2];
	    const UnitOfMeasure* uom = UoMR().get( disp_.unit_ );
	    if ( uom )
	    {
		if ( !mIsUdf(disp_.range_.start) )
		    disp_.range_.start = uom->getSIValue(disp_.range_.start);
		if ( !mIsUdf(disp_.range_.stop) )
		    disp_.range_.stop = uom->getSIValue(disp_.range_.stop);
	    }
	}
    }

    deleteAndZeroPtr( defval_ );
    deleteAndZeroPtr( mathdef_ );
    BufferString mathdefstr;
    fms = iop.find( sKeyDefaultValue );
    sz = fms.size();
    if ( sz > 1 )
    {
	const BufferString typ( fms[0] );
	Property* prop = factory().create( name(), typ, mn_ );
	mDynamicCastGet(MathProperty*,mp,prop)
	if ( !mp )
	    defval_ = new ValueProperty( name(), mn_, toFloat(fms[1]) );
	else
	{
	    mathdef_ = mp;
	    mathdef_->setDef( fms[1] );
	}
    }

    const FixedString def = iop.find( sKeyDefinition );
    if ( !def.isEmpty() )
	mathdef_ = new MathProperty( name(), mn_, def );

    if ( !defval_ )
	defval_ = new ValueProperty( name(), mn_, commonValue() );
}


void Property::usePar( const IOPar& iop )
{
    const char* res = iop.find( sKey::Value() );
    if ( res && *res )
	setDef( res );
}


Property* Property::get( const IOPar& iop )
{
    PropertyRef::StdType st = PropertyRef::Other;
    BufferString propnm, mn;
    if ( !iop.isPresent(sKey::Name()) )
    {
	propnm = iop.getKey( 0 );
	if ( propnm.isEmpty() )
	    return nullptr;

	const BufferString stdtypstr( iop.getValue(0) );
	PropertyRef::parseEnumStdType(stdtypstr, st );

    }
    else
    {
	iop.get(sKey::Name(), propnm);
	if ( propnm.isEmpty() )
	    return nullptr;
    }

    Mnemonic* mnm = nullptr;
    iop.get( sKeyMnemonic, mn );
    if ( mn.isEmpty() )
	mnm = const_cast<Mnemonic*>( MNC().getGuessed(st) );
    else
	mnm = const_cast<Mnemonic*>( MNC().find(mn) );

    const char* typ = iop.find( sKey::Type() );
    if ( !typ || !*typ )
	typ = ValueProperty::typeStr();

    Property* prop = factory().create( typ, propnm, *mnm );
    if ( prop )
    {
	prop->getDataUsingPar( iop );
	prop->usePar( iop );
    }

    return prop;
}


bool Property::init(const PropertySet&) const
{
    const_cast<Property*>(this)->reset();
    return true;
}


void Property::setFixedDef( const MathProperty* mp )
{
    delete mathdef_;
    mathdef_ = mp ? mp->clone() : nullptr;
}



//------- ValueProperty ----------


ValueProperty::ValueProperty( const char* nm, const Mnemonic& mnc )
    : Property(nm, mnc)
    , val_(mnc.disp_.typicalrange_.center())
{}


ValueProperty::ValueProperty( const char* nm, const Mnemonic& mnc, float v )
    : Property(nm, mnc)
{
    val_ = v;
}


const char* ValueProperty::def() const
{
    return ::toString( val_ );
}


void ValueProperty::setDef( const char* defstr )
{
    if ( defstr && *defstr )
	val_ = toFloat( defstr );
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
    fms = ::toString(rg_.start);
    fms += ::toString(rg_.stop);
    return fms.buf();
}


void RangeProperty::setDef( const char* defstr )
{
    if ( !defstr || !*defstr )
	rg_.start = mUdf(float);
    else
    {
	FileMultiString fms( defstr );
	rg_.start = fms.getFValue( 0 );
	rg_.stop = fms.getFValue( 1 );
    }
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
    if ( isThickness() )
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

static const ValueProperty& depthProp()
{
    mDefineStaticLocalObject( PtrMan<ValueProperty>, depthprop,
			 = new ValueProperty("Depth",Mnemonic::distance(), 0) );
    depthprop->disp_.range_ = Interval<float>(0,5000);
    depthprop->disp_.typicalrange_ = Interval<float>(0,5000);
    return *depthprop;
}


static const ValueProperty& relDepthProp()
{
    mDefineStaticLocalObject( PtrMan<ValueProperty>, reldepthprop,
		     = new ValueProperty("RelDepth",Mnemonic::distance(), 0) );
    reldepthprop->disp_.range_ = Interval<float>( Interval<float>::udf() );
    reldepthprop->disp_.typicalrange_ = Interval<float>(-1000,1000);
    return *reldepthprop;
}


static const ValueProperty& xposProp()
{
    mDefineStaticLocalObject( PtrMan<ValueProperty>, xposprop,
			 = new ValueProperty("XPos",Mnemonic::distance(), 0) );
    xposprop->disp_.range_ = Interval<float>(0,1);
    xposprop->disp_.typicalrange_ = Interval<float>(0,1);
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


MathProperty::MathProperty( const char* propnm,
			    const Mnemonic& mn, const char* df )
    : Property(propnm, mn)
    , form_(*new Math::Formula(false,getSpecVars()))
{
    inps_.allowNull( true );
    if ( df && *df )
	setDef( df );
}


MathProperty::MathProperty( const MathProperty& oth )
    : Property(oth.name(), oth.mnem())
    , form_(*new Math::Formula(oth.form_))
    , inps_(oth.inps_)
{}


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
    const PropertySet& props = PROPS();
    BufferStringSet propnms;
    for ( int idx=props.size()-1; idx>-2; idx-- )
    {
	const char* propnm = idx<0 ? "Thickness" : props.get(idx).name().buf();
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
	    return inp->mnem().stdType();
    }

    const Property* exclude = PROPS().find( name() );
    PropertySet prs(
	    PropertySet::getAll(true, exclude) );
    const char* propnm = form_.inputDef( iinp );
    const Property* pr = prs.getByName( propnm );
    if ( pr )
	return pr->mnem().stdType();

    return PropertyRef::Other;
}


void MathProperty::setInput( int iinp, const Property* p )
{
    if ( !inps_.validIdx(iinp) )
	{ pErrMsg("idx out of range"); return; }

    if ( p && p->dependsOn(*this) )
    {
	BufferString msg( "Invalid cyclic dependency for ", name() );
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


//------- PropertySetMgr ----------

class PropertySetMgr : public CallBacker
{
public:

PropertySetMgr()
    : prs_(0)
{
    IOM().surveyChanged.notify( mCB(this,PropertySetMgr,doNull) );
}


~PropertySetMgr()
{
    delete prs_;
}


void doNull( CallBacker* )
{
    delete prs_; prs_ = 0;
}

void createSet()
{
    Repos::FileProvider rfp( filenamebase, true );
    while ( rfp.next() )
    {
	const BufferString fnm( rfp.fileName() );
	SafeFileIO sfio( fnm );
	if ( !sfio.open(true) )
	    continue;

	ascistream astrm( sfio.istrm(), true );
	PropertySet* oldprs = prs_;
	prs_ = new PropertySet;
	prs_->readFrom( astrm );
	if ( prs_->isEmpty() )
	{
	    delete prs_;
	    prs_ = oldprs;
	    sfio.closeSuccess();
	}
	else
	{
	    delete oldprs;
	    sfio.closeSuccess();
	    break;
	}
    }

    if ( !prs_ )
	prs_ = new PropertySet;

    MNC();	//Creating a MnemonicSet at the same time as PropertyRefSet
}

    PropertySet* prs_;

};


//------- PropertySet ----------

const PropertySet& PROPS()
{
    mDefineStaticLocalObject( PropertySetMgr, rsm, );
    if ( !rsm.prs_ )
	rsm.createSet();

    return *rsm.prs_;
}


PropertySet::PropertySet( const MnemonicSelection& mncs )
{
    for ( int idx=0; idx<mncs.size(); idx++ )
	props_ += new ValueProperty( mncs[idx]->name(), *mncs[idx] );

    props_ += getProp_Thick_Man()->ref_;
}


PropertySet::~PropertySet()
{
    if ( !thickness_propman_is_deleting_thickness )
	props_ -= getProp_Thick_Man()->ref_;

    erase();
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


void PropertySet::swap( int idx1, int idx2 )
{
    props_.swap( idx1, idx2 );
}


int PropertySet::indexOf( const char* nm, bool matchaliases ) const
{
    if ( !nm || !*nm )
	return -1;

    for ( int idx=0; idx<props_.size(); idx++ )
    {
	const Property& p = *props_[idx];
	if ( p.name() == nm )
	    return idx;
    }

    if ( matchaliases )
    {
	for ( int idx=0; idx<props_.size(); idx++ )
	{
	    const Property& p = *props_[idx];
	    if ( p.mnem().isKnownAs(nm) )
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
	if ( pr.mnem().hasType(st) )
	{
	    occ--;
	    if ( occ < 0 )
		return idx;
	}
    }

    return -1;
}


PropertySet PropertySet::getAll( bool withth, const Property* exclude )
{
    PropertySet ret;
    if ( !withth )
    {
	const int idxth = ret.indexOf( *getProp_Thick_Man()->ref_ );
	ret.remove( idxth );
    }

    const PropertySet& props = PROPS();
    for ( int idx=0; idx<props.size(); idx++ )
    {
	const Property* pr = &props.get( idx );
	if ( pr != exclude )
	    ret.add( const_cast<Property*>(pr) );
    }

    return ret;
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
			      ObjectSet<const Property>& resultset ) const
{
    for ( int idx=0; idx<props_.size(); idx++ )
	if ( props_[idx] && props_[idx]->mnem().hasType(proptype) )
	    resultset += props_[idx];
}


bool PropertySet::save( Repos::Source src ) const
{
    Repos::FileProvider rfp( filenamebase );
    BufferString fnm = rfp.fileName( src );

    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(false) )
    {
	BufferString msg( "Cannot write to " ); msg += fnm;
	ErrMsg( sfio.errMsg() );
	return false;
    }

    ascostream astrm( sfio.ostrm() );
    if ( !writeTo(astrm) )
    {
	sfio.closeFail();
	return false;
    }

    return sfio.closeSuccess();
}


void PropertySet::readFrom( ascistream& astrm )
{
    props_ -= getProp_Thick_Man()->ref_;
    deepErase( props_ );

    while ( !atEndOfSection(astrm.next()) )
    {
	IOPar iop; iop.getFrom(astrm);
	props_ += Property::get( iop );
    }
}


bool PropertySet::writeTo( ascostream& astrm ) const
{
    astrm.putHeader( "Properties" );
    for ( int idx=0; idx<size(); idx++ )
    {
	const Property& pr = *props_[idx];
	IOPar iop;
	iop.set( sKey::Name(),pr.name() );
	pr.fillPar( iop );
	iop.putTo( astrm );
    }

    return astrm.isOK();
}


PropertySelection::PropertySelection()
{
    *this += getProp_Thick_Man()->ref_;
}


bool PropertySelection::operator ==( const PropertySelection& oth ) const
{
    if ( size() != oth.size() )
	return false;

    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx] != oth[idx] )
	    return false;

    return true;
}


int PropertySelection::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Property& pr = *((*this)[idx]);
	if ( pr.name() == nm )
	    return idx;
    }
    return -1;
}


int PropertySelection::find( const char* nm ) const
{
    const int idxof = indexOf( nm );
    if ( idxof >= 0 )
	return idxof;

    for ( int idx=0; idx<size(); idx++ )
    {
	const Property& pr = *((*this)[idx]);
	if ( pr.isKnownAs( nm ) )
	    return idx;
    }

    return -1;
    }


PropertySelection PropertySelection::subselect( const Mnemonic& mn ) const
{
    PropertySelection subsel;
    subsel.erase();
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx] && (*this)[idx]->mnem()==mn  )
	    subsel += (*this) [idx];

    return subsel;
}


PropertySelection PropertySelection::getAll( bool withth,
					const Property* exclude )
{
    PropertySelection ret;
    if ( !withth )
      ret -= getProp_Thick_Man()->ref_;

    const PropertySet& props = PROPS();
    for ( int idx=0; idx<props.size(); idx++ )
    {
	const Property* pr = &props.get( idx );
	if ( pr != exclude )
	    ret += pr;
    }
    return ret;
}


PropertySelection PropertySelection::getAll( const Mnemonic& mn )
{
    PropertySelection ret;
    if ( mn != Mnemonic::distance() )
	ret -= getProp_Thick_Man()->ref_;

    const PropertySet& props = PROPS();
    for ( int idx=0; idx<props.size(); idx++ )
    {
	const Property* pr = &props.get( idx );
	if ( pr->mnem() == mn )
	    ret += pr;
    }

    return ret;
}

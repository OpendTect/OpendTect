/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "propertyref.h"

#include "ascstream.h"
#include "globexpr.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "mathformula.h"
#include "mathproperty.h"
#include "odpair.h"
#include "safefileio.h"
#include "separstr.h"
#include "survinfo.h"
#include "unitofmeasure.h"

#include <typeinfo>


static const char* filenamebase = "Properties";
static const char* sKeyAliases = "Aliases";
static const char* sKeyDefaultValue = "DefaultValue";
static const char* sKeyDefinition = "Definition";

const char* PropertyRefSelection::sKeyModelProp()
{ return "Model Properties"; }

static bool thickness_proprefman_is_deleting_thickness = false;

class ThicknessPropertyRef : public PropertyRef
{
public:

ThicknessPropertyRef()
    : PropertyRef( Mnemonic::distance(), sKey::Thickness() )
{
    addAliases( BufferStringSet("thick") );
    disp_.color_ = OD::Color::Black();
    disp_.typicalrange_ = Interval<float>( 1.f, 99.f );
    disp_.defval_ = (Property*)(this);
}

private:

~ThicknessPropertyRef()
{
    if ( !thickness_proprefman_is_deleting_thickness )
    { pErrMsg( "Fatal error, should not delete 'Thickness'." ); }
}

friend struct PropRef_Thick_Man;

};


struct PropRef_Thick_Man : public CallBacker
{

PropRef_Thick_Man()
{
    ref_ = new ThicknessPropertyRef();
    setZUnit();
    IOM().afterSurveyChange.notify( mCB(this,PropRef_Thick_Man,setZUnit) );
}


~PropRef_Thick_Man()
{
    thickness_proprefman_is_deleting_thickness = true;
    ref_->disp_.defval_ = nullptr;
    delete ref_;
}


void setZUnit( CallBacker* cb=nullptr )
{
    ref_->setUnit( toString(UnitOfMeasure::zUnitAnnot(false,true,false) ) );
}

    ThicknessPropertyRef*	ref_;
};


PropRef_Thick_Man* getPropRef_ThickRef_Man()
{
    mDefineStaticLocalObject( PtrMan<PropRef_Thick_Man>, ptm,
			      = new PropRef_Thick_Man );
    return ptm;
}


//------- PropertyRef::DispDefs -------

PropertyRef::DispDefs::DispDefs()
    : Mnemonic::DispDefs()
{
}


PropertyRef::DispDefs::DispDefs( const Mnemonic::DispDefs& mndisp )
    : Mnemonic::DispDefs(mndisp)
{
}


PropertyRef::DispDefs::~DispDefs()
{
    delete defval_;
}


PropertyRef::DispDefs& PropertyRef::DispDefs::operator=( const DispDefs& oth )
{
    if ( this != &oth )
    {
	Mnemonic::DispDefs::operator=( oth );
	delete defval_;
	defval_ = oth.defval_ ? oth.defval_->clone() : nullptr;
    }
    return *this;
}


bool PropertyRef::DispDefs::operator ==( const DispDefs& oth ) const
{
    return Mnemonic::DispDefs::operator ==( oth ) &&
	   ((!defval_ && !oth.defval_) ||
	    (defval_ && oth.defval_ && defval_->isEqualTo(*oth.defval_)) );
}


bool PropertyRef::DispDefs::operator !=( const DispDefs& oth ) const
{
    return !(*this == oth);
}


void PropertyRef::DispDefs::copyFrom( const Mnemonic::DispDefs& mndisp )
{
    Mnemonic::DispDefs::operator=( mndisp );
}


bool PropertyRef::DispDefs::setUnit( const char* newunitlbl )
{
    return Mnemonic::DispDefs::setUnit( newunitlbl );
}


float PropertyRef::DispDefs::commonValue() const
{
    if ( defval_ && defval_->isValue() )
	return defval_->value();

    return Mnemonic::DispDefs::commonValue();
}


//------- PropertyRef -------

PropertyRef::PropertyRef( const Mnemonic& mn, const char* nm )
    : NamedCallBacker((nm && *nm) ? nm : mn.name().buf())
    , mn_(mn)
    , uom_(mn.unit())
    , unitChanged(this)
{
    disp_.copyFrom( mn.disp_ );
    setDefaults();
}


PropertyRef::PropertyRef( const PropertyRef& pr )
    : NamedCallBacker(pr.name())
    , mn_(pr.mn_)
    , unitChanged(this)
{
    *this = pr;
}


PropertyRef::~PropertyRef()
{
    delete mathdef_;
}


PropertyRef& PropertyRef::operator =( const PropertyRef& pr )
{
    if ( this != &pr )
    {
	if ( &pr.mn_ != &mn_ )
	    { pErrMsg( "PropertyRef should not switch Mnemonic" ); }

	NamedCallBacker::setName( pr.name() );
	disp_ = pr.disp_;
	uom_ = pr.uom_;
	propaliases_ = pr.propaliases_;
	delete mathdef_;
	mathdef_ = pr.mathdef_ ? pr.mathdef_->clone() : nullptr;
    }

    return *this;
}


bool PropertyRef::operator ==( const PropertyRef& oth ) const
{
    if ( this == &oth )
	return true;

    return &mn_ == &oth.mn_ && NamedCallBacker::operator ==( oth ) &&
	   uom_ == oth.uom_ &&
	   propaliases_ == oth.propaliases_ &&
	   ((!mathdef_ && !oth.mathdef_) ||
	    (mathdef_ && oth.mathdef_ && mathdef_->isEqualTo(*oth.mathdef_))) &&
	   disp_ == oth.disp_;
}


bool PropertyRef::operator !=( const PropertyRef& oth ) const
{
    return !(*this == oth);
}


bool PropertyRef::matches( const char* nm, bool matchaliases,
			   float* matchval ) const
{
    BufferStringSet nms( name().buf() );
    if ( matchaliases )
    {
	if ( !propaliases_.isEmpty() )
	    nms.append( propaliases_ );

	nms.add( mn_.name() ).add( mn_.description() );
	if ( !mn_.aliases().isEmpty() )
	    nms.append( mn_.aliases() );
    }

    const float val = Mnemonic::getMatchValue( nm, nms, !matchaliases, false );
    if ( matchval )
	*matchval = val;

    return val > 0.f;
}


bool PropertyRef::isElastic() const
{
    return stdType() == Mnemonic::Den || stdType() == Mnemonic::Vel ||
	   &mn_ == &Mnemonic::defFracDensity() ||
	   &mn_ == &Mnemonic::defFracOrientation();
}


void PropertyRef::setFixedDef( const MathProperty* mp )
{
    delete mathdef_;
    mathdef_ = mp ? mp->clone() : nullptr;
}


void PropertyRef::setUnit( const char* newunitlbl )
{
    if ( StringView(newunitlbl) == disp_.getUnitLbl() )
	return;

    const UnitOfMeasure* olduom = uom_;
    uom_ = UoMR().get( newunitlbl );
    if ( !uom_ )
	uom_ = UoMR().getInternalFor( stdType() );

    const BufferString unitlbl = UnitOfMeasure::getUnitLbl( uom_, newunitlbl );
    if ( !disp_.setUnit(unitlbl) )
	return;

    unitChanged.trigger( olduom );
}


void PropertyRef::addAliases( const BufferStringSet& aliases )
{
    propaliases_.add( aliases, false );
}


const BufferStringSet PropertyRef::aliases() const
{
    BufferStringSet ret( mn_.aliases() );
    ret.add( propaliases_, false );
    return ret;
}


PropertyRef* PropertyRef::get( const IOPar& iop, Repos::Source src )
{
    BufferString propnm;
    iop.get( sKey::Name(), propnm );
    if ( propnm.isEmpty() )
	return nullptr;

    Mnemonic::StdType st = Mnemonic::undef().stdType();
    const BufferString stdtypstr = iop.find( propnm );
    if ( !stdtypstr.isEmpty() )
	Mnemonic::parseEnumStdType( stdtypstr, st );

    BufferString mn;
    iop.get( Mnemonic::sKeyMnemonic(), mn );
    BufferStringSet hintnms( propnm.buf() );
    BufferString aliasnms;
    if ( iop.get(sKeyAliases,aliasnms) && !aliasnms.isEmpty() )
    {
	const FileMultiString fms( aliasnms.str() );
	for ( int idx=0; idx<fms.size(); idx++ )
	    hintnms.addIfNew( fms[idx] );
    }

    const Mnemonic* mnptr = mn.isEmpty() || mn == Mnemonic::undef().name()
			  ? MnemonicSelection::getGuessed(nullptr,st,&hintnms)
			  : MNC().getByName( mn );
    if ( Repos::isUserDefined(src) )
	mnptr = getFromLegacy( mnptr, propnm.buf() );

    if ( !mnptr || mnptr->isUdf() )
	return nullptr;

    auto* ret = new PropertyRef( *mnptr, propnm );
    ret->usePar( iop );
    ret->source_ = src;

    return ret;
}


void PropertyRef::setDefaults()
{
    const MnemonicSelection allsats = MnemonicSelection::getAllSaturations();
    if ( !mathdef_ && !disp_.defval_ && allsats.isPresent(&mn_) )
    {
	float defval = BufferString(
	      mn_.description()).contains( "Water Saturation" ) ? 1.f : 0.f;
	convValue( defval, nullptr, unit() );
	disp_.defval_ = new ValueProperty( *this, defval );
    }
}


void PropertyRef::usePar( const IOPar& iop )
{
    propaliases_.setEmpty();
    FileMultiString fms( iop.find(sKeyAliases) );
    int sz = fms.size();
    for ( int ifms=0; ifms<sz; ifms++ )
	propaliases_.addIfNew( fms[ifms] );

    iop.get( sKey::Color(), disp_.color_ );

    const UnitOfMeasure* intuom = UoMR().getInternalFor( stdType() );

    fms = iop.find( sKey::Range() );
    if ( fms.size() > 1 )
    {
	Interval<float> typicalrange( fms.getFValue(0), fms.getFValue(1) );
	const UnitOfMeasure* valsuom = fms.size() > 2
				     ? UoMR().get( fms[2].buf() ) : nullptr;
	if ( valsuom && intuom && valsuom->isCompatibleWith(*intuom) )
	{
	    if ( unit() == mn_.unit() && valsuom != unit() )
	    {
		NotifyStopper ns( unitChanged );
		setUnit( fms[2].buf() );
	    }
	    else
	    {
		convValue( typicalrange.start, valsuom, unit() );
		convValue( typicalrange.stop, valsuom, unit() );
	    }
	}
	else
	{
	    convValue( typicalrange.start, intuom, unit() );
	    convValue( typicalrange.stop, intuom, unit() );
	}

	disp_.typicalrange_ = typicalrange;
    }

    BufferString mathdefstr;
    fms = iop.find( sKeyDefaultValue );
    sz = fms.size();
    if ( sz == 1 && fms[0].isNumber() )
    {
	const float defval = getConvertedValue( fms[0].toFloat(),
						intuom, unit() );
	delete disp_.defval_;
	disp_.defval_ = new ValueProperty( *this, defval );
    }
    else if ( sz > 0 )
    {
	const StringView typ( fms[0] );
	if ( typ == ValueProperty::typeStr() && sz > 1 )
	{
	    float defval = fms[1].toFloat();
	    const UnitOfMeasure* valuom = sz > 2 ? UoMR().get(fms[2]) : nullptr;
	    if ( valuom && intuom && valuom->isCompatibleWith(*intuom) )
	    {
		if ( unit() == mn_.unit() && valuom != unit() )
		{
		    NotifyStopper ns( unitChanged );
		    setUnit( fms[2].buf() );
		}
		else
		    convValue( defval, valuom, unit() );
	    }
	    else
		convValue( defval, intuom, unit() );

	    if ( !mIsUdf(defval) )
	    {
		delete disp_.defval_;
		disp_.defval_ = new ValueProperty( *this, defval );
	    }
	}
	else if ( typ == RangeProperty::typeStr() && sz > 2 )
	{
	    Interval<float> defrange( fms[1].toFloat(), fms[2].toFloat() );
	    const UnitOfMeasure* valsuom = sz > 3 ? UoMR().get(fms[3])
						  : nullptr;
	    if ( valsuom && intuom && valsuom->isCompatibleWith(*intuom) )
	    {
		if ( unit() == mn_.unit() && valsuom != unit() )
		{
		    NotifyStopper ns( unitChanged );
		    setUnit( fms[2].buf() );
		}
		else
		{
		    convValue( defrange.start, valsuom, unit() );
		    convValue( defrange.stop, valsuom, unit() );
		}
	    }
	    else
	    {
		convValue( defrange.start, intuom, unit() );
		convValue( defrange.stop, intuom, unit() );
	    }

	    if ( !defrange.isUdf() )
	    {
		delete disp_.defval_;
		disp_.defval_ = new RangeProperty( *this, defrange );
	    }
	}
	else if ( (typ == MathProperty::typeStr() && sz > 1) ||
		  (sz == 1 && typ.startsWith("Formula")) )
	{
	    const StringView formdef = typ == MathProperty::typeStr() && sz > 1
				     ? fms[1] : typ;
	    PtrMan<MathProperty> mathdef =
				 new MathProperty( *this, formdef.buf() );
	    if ( mathdef && mathdef->getForm().isOK() )
	    {
		delete disp_.defval_;
		disp_.defval_ = mathdef.release();
	    }
	    else
		{ pErrMsg("Cannot parse default value formula"); }
	}
    }

    const BufferString def = iop.find( sKeyDefinition );
    if ( !def.isEmpty() )
    {
	PtrMan<MathProperty> mathdef = new MathProperty( *this, def );
	if ( mathdef && mathdef->getForm().isOK() )
	{
	    delete mathdef_;
	    mathdef_ = mathdef.release();
	}
	else
	    { pErrMsg("Cannot parse fixed definition formula"); }
    }

    if ( disp_.defval_ && mathdef_ )
	deleteAndNullPtr( disp_.defval_ ); //Keep only one possibility
}


void PropertyRef::fillPar( IOPar& iop ) const
{
    iop.set( Mnemonic::sKeyMnemonic(), mn_.name() );
    if ( propaliases_.isEmpty() )
	iop.removeWithKey( sKeyAliases );
    else
    {
	FileMultiString fms( propaliases_.get(0) );
	for ( int idx=1; idx<propaliases_.size(); idx++ )
	    fms += propaliases_.get(idx);
	iop.set( sKeyAliases, fms );
    }

    const Mnemonic::DispDefs& mndisp = mn_.disp_;
    if ( disp_.color_ == mndisp.color_ )
	iop.removeWithKey( sKey::Color() );
    else
	iop.set( sKey::Color(), disp_.color_ );

    FileMultiString fms;
    const Interval<float>& vintv = disp_.typicalrange_;
    const float inteps = vintv.width() == 0.f ? mDefEpsF
					  : Math::Abs( vintv.center() ) * 1e-4f;
    Interval<float> mnintv = mndisp.typicalrange_;
    const UnitOfMeasure* mnuom = mn_.unit();
    convValue( mnintv.start, mnuom, uom_ );
    convValue( mnintv.stop, mnuom, uom_ );
    if ( mnuom != uom_ || !vintv.isEqual(mnintv,inteps) )
    {
	fms += ::toString( vintv.start );
	fms += ::toString( vintv.stop );
	const char* unitlbl = disp_.getUnitLbl();
	if ( unitlbl && *unitlbl )
	    fms += unitlbl;

	iop.set( sKey::Range(), fms );
    }
    else
	iop.removeWithKey( sKey::Range() );

    if ( disp_.defval_ )
    {
	fms.set( disp_.defval_->type() );
	const FileMultiString deffms( disp_.defval_->def() );
	fms.add( deffms );
	iop.set( sKeyDefaultValue, fms );
    }
    else
	iop.removeWithKey( sKeyDefaultValue );

    if ( mathdef_ )
	iop.set( sKeyDefinition, mathdef_->def() );
    else
	iop.removeWithKey( sKeyDefinition );
}


const Mnemonic* PropertyRef::getFromLegacy( const Mnemonic* mn,
					    const char* propstr )
{
    const Mnemonic* exactmn = MNC().getByName( propstr, false );
    if ( exactmn )
	return exactmn;

    const StringView propnm( propstr );
    if ( propnm.startsWith("Vp_",OD::CaseInsensitive) )
	return &Mnemonic::defPVEL();
    if ( propnm.startsWith("Vs_",OD::CaseInsensitive) )
	return &Mnemonic::defSVEL();

    //First try to volumetrics
    const MnemonicSelection mns( Mnemonic::Volum );
    for ( const auto* volmn : mns )
    {
	if ( volmn->aliases().isPresent(propstr,OD::CaseInsensitive) )
	    return volmn;
    }

    //then try them all
    for ( const auto* anymn : MNC() )
    {
	if ( anymn->aliases().isPresent(propstr,OD::CaseInsensitive) )
	    return anymn;
    }

    return mn;
}


const PropertyRef& PropertyRef::thickness()
{
    return *getPropRef_ThickRef_Man()->ref_;
}


//------- PropertyRefSetMgr ----------

class PropertyRefSetMgr : public CallBacker
{
public:

PropertyRefSetMgr()
{
    IOM().surveyChanged.notify( mCB(this,PropertyRefSetMgr,doNull) );
}


~PropertyRefSetMgr()
{
    delete prs_;
}

void doNull( CallBacker* )
{
    delete prs_; prs_ = nullptr;
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
	PropertyRefSet* oldprs = prs_;
	prs_ = new PropertyRefSet;
	prs_->readFrom( astrm, rfp.source() );
	sfio.closeSuccess();
	if ( prs_->isEmpty() )
	{
	    delete prs_;
	    prs_ = oldprs;
	}
	else
	{
	    delete oldprs;
	    break;
	}
    }

    if ( !prs_ )
	prs_ = new PropertyRefSet;
}

    PropertyRefSet* prs_ = nullptr;

};


const PropertyRefSet& PROPS()
{
    mDefineStaticLocalObject( PropertyRefSetMgr, rsm, );
    if ( !rsm.prs_ )
	rsm.createSet();

    return *rsm.prs_;
}


//------- PropertyRefSet ----------

PropertyRefSet::PropertyRefSet()
    : ManagedObjectSet<PropertyRef>()
{
}


PropertyRef* PropertyRefSet::getByName( const char* nm, bool matchaliases )
{
    const PropertyRef* ret =
	const_cast<const PropertyRefSet*>(this)->getByName( nm, matchaliases );
    return const_cast<PropertyRef*>( ret );
}


const PropertyRef* PropertyRefSet::getByName( const char* nm,
					      bool matchaliases ) const
{
    PropertyRefSelection prs( false );
    for ( const auto* pr : *this )
	prs.add( pr );

    return getByName( nm, prs, matchaliases );
}


const PropertyRef* PropertyRefSet::getByName( const char* nm,
				    const ObjectSet<const PropertyRef>& prs,
				    bool matchaliases )
{
    const StringView nmstr( nm );
    const int sz = nmstr.size();
    if ( sz < 1 )
	return nullptr;

    float val;
    TypeSet<OD::Pair<float,const PropertyRef*> > prmatchvals;
    int maxidx = -1; float maxval = -1.f;
    for ( const auto* pr : prs )
    {
	if ( !pr || !pr->matches(nm,matchaliases,&val) )
	    continue;

	prmatchvals += OD::Pair<float,const PropertyRef*>( val, pr );
	if ( val > maxval )
	{
	    maxval = val;
	    maxidx = prmatchvals.size()-1;
	}
    }

    if ( prmatchvals.isEmpty() )
	return nullptr;

    const PropertyRef* ret = prmatchvals.validIdx( maxidx )
			   ? prmatchvals[maxidx].second() : nullptr;
    return ret;
}


PropertyRef* PropertyRefSet::getByType( PropertyRef::StdType st, int occ )
{
    const PropertyRef* ret =
		const_cast<const PropertyRefSet*>(this)->getByType( st, occ );
    return const_cast<PropertyRef*>( ret );
}


const PropertyRef* PropertyRefSet::getByType( PropertyRef::StdType st,
					      int occ ) const
{
    for ( const auto* pr : *this )
    {
	if ( pr->hasType(st) )
	{
	    occ--;
	    if ( occ < 0 )
		return pr;
	}
    }

    return nullptr;
}


const PropertyRef* PropertyRefSet::getByMnemonic( const Mnemonic& mn,
						  int occ ) const
{
    for ( const auto* pr : *this )
    {
	if ( pr->isCompatibleWith(mn) )
	{
	    occ--;
	    if ( occ < 0 )
		return pr;
	}
    }

    return nullptr;
}


PropertyRef* PropertyRefSet::ensurePresent( const Mnemonic& mnc,
			const char* nm1, const char* nm2, const char* nm3 )
{
    PropertyRefSelection prs( mnc );
    if ( !prs.isEmpty() )
    {
	const PropertyRef* pr = prs.getByName( nm1 );
	if ( !pr )
	    pr = prs.getByName( nm2 );
	if ( !pr )
	    pr = prs.getByName( nm3 );
	if ( !pr )
	    pr = prs.first();

	return (PropertyRef*)(pr);
    }

    auto* pr = new PropertyRef( mnc, nm1 );
    const BufferStringSet aliases( nm2, nm3 );
    pr->addAliases( aliases );
    pr->disp_.color_ = OD::Color::stdDrawColor( int(mnc.stdType()) );
    add( pr );

    return pr;
}


bool PropertyRefSet::ensureHasElasticProps( bool withswave, bool withai )
{
    if ( !ensurePresent(Mnemonic::defDEN(),PropertyRef::standardDenStr(),
			PropertyRef::standardDenAliasStr()) )
	return false;

    if ( !ensurePresent(Mnemonic::defPVEL(),PropertyRef::standardPVelStr(),
			PropertyRef::standardPVelAliasStr()) )
	return false;

    if ( withswave && !ensurePresent( Mnemonic::defSVEL(),
				      PropertyRef::standardSVelStr(),
				      PropertyRef::standardSVelAliasStr() ) )
	return false;

    if ( withai && !ensurePresent(Mnemonic::defAI(),nullptr) )
	return false;

    return true;
}


bool PropertyRefSet::subselect( PropertyRef::StdType proptype,
				ObjectSet<const PropertyRef>& prs ) const
{
    prs.erase();
    for ( const auto* pr : *this )
	if ( pr->hasType(proptype)  )
	    prs += pr;

    return !prs.isEmpty();
}


PropertyRefSet& PropertyRefSet::doAdd( PropertyRef* pr )
{
    if ( !pr || getByName(pr->name(),false) )
    {
	delete pr;
	return *this;
    }

    ObjectSet<PropertyRef>::doAdd( pr );
    return *this;
}


void PropertyRefSet::readFrom( ascistream& astrm, Repos::Source src )
{
    deepErase( *this );

    while ( !atEndOfSection(astrm.next()) )
    {
	const BufferString propnm( astrm.keyWord() );
	IOPar iop;
	iop.getFrom(astrm);
	BufferStringSet keys;
	iop.getKeys( keys );
	if ( !iop.hasKey(sKey::Name()) )
	    iop.set( sKey::Name(), propnm );

	if ( getByName(propnm,false) )
	    continue;

	PropertyRef* pr = PropertyRef::get( iop, src );
	add( pr );
    }
}


bool PropertyRefSet::writeTo( ascostream& astrm ) const
{
    astrm.putHeader( filenamebase );
    for ( const auto* pr : *this )
    {
	IOPar iop;
	iop.set( sKey::Name(), pr->name() );
	pr->fillPar( iop );
	iop.putTo( astrm );
    }

    return astrm.isOK();
}


bool PropertyRefSet::save( Repos::Source src ) const
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


//------- PropertyRefSelection ----------

PropertyRefSelection::PropertyRefSelection( bool with_thickness )
    : ObjectSet<const PropertyRef>()
{
    if ( with_thickness )
	*this += getPropRef_ThickRef_Man()->ref_;
}


PropertyRefSelection::PropertyRefSelection( bool with_thickness,
					    const PropertyRef* exclude )
    : ObjectSet<const PropertyRef>()
{
    if ( with_thickness )
	*this += getPropRef_ThickRef_Man()->ref_;

    const PropertyRefSet& props = PROPS();
    for ( const auto* pr : props )
	if ( pr != exclude )
	    add( pr );
}


PropertyRefSelection::PropertyRefSelection( const Mnemonic& mnc )
    : ObjectSet<const PropertyRef>()
{
    const PropertyRefSet& props = PROPS();
    for ( const auto* pr : props )
	if ( pr->isCompatibleWith(mnc) )
	    add( pr );
}


PropertyRefSelection::PropertyRefSelection( PropertyRef::StdType type )
    : ObjectSet<const PropertyRef>()
{
    const PropertyRefSet& props = PROPS();
    for ( const auto* pr : props )
	if ( pr->stdType() == type )
	    add( pr );
}


void PropertyRefSelection::fillPar( IOPar& par ) const
{
    BufferStringSet names;
    for ( const auto* pr : *this )
	names.add( pr->name().buf() );

    IOPar modelpar;
    modelpar.set( sKey::Names(), names );
    par.mergeComp( modelpar, sKeyModelProp() );
}


bool PropertyRefSelection::usePar( const IOPar& par )
{
    PtrMan<IOPar> modelpar = par.subselect( sKeyModelProp() );
    if ( !modelpar )
	return false;

    PropertyRefSelection newprs( false );
    BufferStringSet names;
    if ( !modelpar->get(sKey::Names(),names) )
	return false;

    for ( const auto* nm : names )
    {
	const PropertyRef* pr = PROPS().getByName( nm->buf(), false );
	if ( !pr )
	    return false;
	newprs.add( pr );
    }

    for ( const auto* pr : newprs )
	addIfNew( pr );

    return true;
}


const PropertyRef* PropertyRefSelection::getByName( const char* nm,
						    bool matchaliases ) const
{
    return PropertyRefSet::getByName( nm, *this, matchaliases );
}


const PropertyRef* PropertyRefSelection::getByType( PropertyRef::StdType st,
						    int occ ) const
{
    for ( const auto* pr : *this )
    {
	if ( pr->hasType(st) )
	{
	    occ--;
	    if ( occ < 0 )
		return pr;
	}
    }

    return nullptr;
}


const PropertyRef* PropertyRefSelection::getByMnemonic( const Mnemonic& mn,
							int occ ) const
{
    for ( const auto* pr : *this )
    {
	if ( pr->isCompatibleWith(mn) )
	{
	    occ--;
	    if ( occ < 0 )
		return pr;
	}
    }

    return nullptr;
}


PropertyRefSelection PropertyRefSelection::subselect(
					PropertyRef::StdType type ) const
{
    PropertyRefSelection subsel( false );
    for ( const auto* pr : *this )
	if ( pr->hasType(type) )
	    subsel += pr;

    return subsel;
}

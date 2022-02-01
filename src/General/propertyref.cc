/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/


#include "propertyref.h"

#include "ascstream.h"
#include "globexpr.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "mathproperty.h"
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
    , unitChanged(this)
{
    disp_.copyFrom( mn.disp_ );
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


bool PropertyRef::matches( const char* nm, bool matchaliases ) const
{
    return matchaliases ? isKnownAs( nm ) : name() == nm;
}


bool PropertyRef::isKnownAs( const char* nm ) const
{
    const FixedString nmstr( nm );
    if ( nmstr.isEmpty() )
	return false;

    if ( name().matches(nm,CaseInsensitive) )
	return true;

    BufferStringSet nms( name() );
    nms.add( aliases(), false );
    for ( const auto* findnm : nms )
    {
	if ( findnm->isEmpty() )
	    continue;

	BufferString gexpr( findnm->buf() );
	gexpr.trimBlanks().replace( " ", "*" ).add( "*" );
	const GlobExpr ge( gexpr, false );
	if ( ge.matches(nmstr) )
	    return true;
    }

    return false;
}


bool PropertyRef::isElastic() const
{
    return stdType() == Mnemonic::Den || stdType() == Mnemonic::Vel;
}


void PropertyRef::setFixedDef( const MathProperty* mp )
{
    delete mathdef_;
    mathdef_ = mp ? mp->clone() : nullptr;
}


void PropertyRef::setUnit( const char* newunitlbl )
{
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


PropertyRef* PropertyRef::get( const IOPar& iop )
{
    Mnemonic::StdType st = Mnemonic::undef().stdType();
    BufferString propnm;
    if ( iop.isPresent(sKey::Name()) )
	iop.get( sKey::Name(), propnm );
    else
    { //Old format
	propnm = iop.getKey( 0 );
	const BufferString stdtypstr( iop.getValue(0) );
	Mnemonic::parseEnumStdType( stdtypstr, st );
    }

    if ( propnm.isEmpty() )
	return nullptr;

    BufferString mn;
    iop.get( Mnemonic::sKeyMnemonic(), mn );
    BufferStringSet hintnms( propnm );
    BufferString aliasnms;
    if ( iop.get(sKeyAliases,aliasnms) && !aliasnms.isEmpty() )
    {
	const FileMultiString fms( aliasnms.str() );
	for ( int idx=0; idx<fms.size(); idx++ )
	    hintnms.addIfNew( fms[idx] );
    }

    const Mnemonic* mnptr = mn.isEmpty() ? &MNC().getGuessed( st, &hintnms )
					 : MNC().getByName( mn );
    if ( !mnptr || mnptr->isUdf() )
	return nullptr;

    auto* ret = new PropertyRef( *mnptr, propnm );
    ret->usePar( iop );

    return ret;
}


void PropertyRef::usePar( const IOPar& iop )
{
    propaliases_.setEmpty();
    FileMultiString fms( iop.find(sKeyAliases) );
    int sz = fms.size();
    for ( int ifms=0; ifms<sz; ifms++ )
	propaliases_.addIfNew( fms[ifms] );

    iop.get( sKey::Color(), disp_.color_ );
    fms = iop.find( sKey::Range() );
    sz = fms.size();
    Interval<float> typicalrange = disp_.typicalrange_;
    if ( sz > 1 )
    {
	typicalrange.start = fms.getFValue( 0 );
	typicalrange.stop = fms.getFValue( 1 );
    }

    NotifyStopper ns( unitChanged );
    setUnit( sz > 2 ? fms[2] : disp_.getUnitLbl() );
    disp_.typicalrange_ = typicalrange;

    deleteAndZeroPtr( disp_.defval_ );
    deleteAndZeroPtr( mathdef_ );
    BufferString mathdefstr;
    fms = iop.find( sKeyDefaultValue );
    sz = fms.size();
    if ( sz == 1 )
    {
	const BufferString retstr( fms.buf() );
	if ( retstr.isNumber() )
	    disp_.defval_ = new ValueProperty( *this, retstr.toFloat() );
    }
    else if ( sz > 1 )
    {
	const BufferString typ( fms[0] );
	if ( typ == ValueProperty::typeStr() )
	    disp_.defval_ = new ValueProperty( *this, toFloat(fms[1]) );
	else if ( typ == RangeProperty::typeStr() && sz > 2 )
	    disp_.defval_ = new RangeProperty( *this,
		    Interval<float>(toFloat(fms[1]), toFloat(fms[2])) );
	else if ( typ == MathProperty::typeStr() )
	    disp_.defval_ = new MathProperty( *this, fms[1] );
    }

    const FixedString def = iop.find( sKeyDefinition );
    if ( !def.isEmpty() )
	mathdef_ = new MathProperty( *this, def );
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

    iop.set( sKey::Color(), disp_.color_ );

    const Interval<float> vintv( disp_.typicalrange_ );

    FileMultiString fms;
    fms += ::toString( vintv.start );
    fms += ::toString( vintv.stop );
    const char* unitlbl = disp_.getUnitLbl();
    if ( unitlbl && *unitlbl )
	fms += unitlbl;

    iop.set( sKey::Range(), fms );
    if ( disp_.defval_ )
    {
	fms.set( disp_.defval_->type() ).add( disp_.defval_->def() );
	iop.set( sKeyDefaultValue, fms );
    }
    else
	iop.removeWithKey( sKeyDefaultValue );

    if ( mathdef_ )
	iop.set( sKeyDefinition, mathdef_->def() );
    else
	iop.removeWithKey( sKeyDefinition );
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
	prs_->readFrom( astrm );
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
    if ( nm && *nm )
    {
	for ( const auto* pr : *this )
	    if ( pr->matches(nm,matchaliases) )
		return pr;
    }

    return nullptr;
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


void PropertyRefSet::readFrom( ascistream& astrm )
{
    deepErase( *this );

    while ( !atEndOfSection(astrm.next()) )
    {
	IOPar iop; iop.getFrom(astrm);
	const BufferString propnm( iop.getKey(0) );
	if ( getByName(propnm,false) )
	    continue;

	PropertyRef* pr = PropertyRef::get( iop );
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
    if ( nm && *nm )
    {
	for ( const auto* pr : *this )
	    if ( pr->matches(nm,matchaliases) )
		return pr;
    }

    return nullptr;
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

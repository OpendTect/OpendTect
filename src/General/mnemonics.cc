/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mnemonics.h"

#include "ascstream.h"
#include "ioman.h"
#include "globexpr.h"
#include "odpair.h"
#include "safefileio.h"
#include "separstr.h"
#include "unitofmeasure.h"

#include <regex>
#include <QHash>
#include <QString>


static const char* filenamebase = "Mnemonics";
const char* Mnemonic::sKeyMnemonic()	{ return "Mnemonic"; }

//------- Mnemonic::DispDefs ----------

Mnemonic::DispDefs::DispDefs()
{
}


Mnemonic::DispDefs::~DispDefs()
{
}


bool Mnemonic::DispDefs::operator ==( const DispDefs& oth ) const
{
    if ( this == &oth )
	return true;

    return color_ == oth.color_ &&
	   scale_ == oth.scale_ &&
	   unitlbl_ == oth.unitlbl_ &&
	   range_ == oth.range_ &&
	   typicalrange_ == oth.typicalrange_;
}


bool Mnemonic::DispDefs::operator !=( const DispDefs& oth ) const
{
    return !(*this == oth );
}


bool Mnemonic::DispDefs::setUnit( const char* newunitlbl )
{
    const UnitOfMeasure* olduom = UoMR().get( unitlbl_ );
    const UnitOfMeasure* newuom = UoMR().get( newunitlbl );
    if ( newuom == olduom )
	return false;

    unitlbl_.set( newunitlbl );
    convValue( range_.start, olduom, newuom );
    convValue( range_.stop, olduom, newuom );
    convValue( typicalrange_.start, olduom, newuom );
    convValue( typicalrange_.stop, olduom, newuom );

    return true;
}


float Mnemonic::DispDefs::commonValue() const
{
    const Interval<float>& range = defRange();
    const bool udf0 = mIsUdf(range.start);
    const bool udf1 = mIsUdf(range.stop);
    if ( udf0 && udf1 )
	return 0.f;

    if ( udf0 || udf1 )
	return udf0 ? range.stop : range.start;

    return range.center();
}


//------- Mnemonic ----------

mDefineEnumUtils(Mnemonic,StdType,"Standard Property")
{
	"Angle",
	"Anisotropy",
	"Area",
	"Classification",
	"Compressibility",
	"Density",
	"Distance/Depth",
	"Elastic Ratio",
	"Electrical Potential",
	"Fluid", // Only for some radioactive measurements
	"Gamma Ray",
	"Impedance",
	"Permeability",
	"Pressure",
	"Pressure Gradient",
	"Pressure-Weight",
	"Resistivity",
	"Sonic travel time",
	"Temperature",
	"Time",
	"Velocity",
	"Volumetrics", // ratios: relative
	"Volume", // absolute
	"Other",
	nullptr
};


Mnemonic::StdType Mnemonic::surveyZType( const SurveyInfo* si )
{
    return (si ? si->zIsTime() : SI().zIsTime()) ? Time : Dist;
}


mDefineEnumUtils(Mnemonic,Scale,"Plot Scale")
{
    "Linear",
    "Logarithmic",
    nullptr
};


Mnemonic::Mnemonic( const char* nm, StdType stdtype )
    : NamedObject(nm)
    , stdtype_(stdtype)
{}


Mnemonic::Mnemonic( const Mnemonic& mnc )
    : NamedObject(mnc.name())
{
    *this = mnc;
}


Mnemonic::~Mnemonic()
{
}


Mnemonic& Mnemonic::operator =( const Mnemonic& oth )
{
    if ( this != &oth )
    {
	NamedObject::setName( oth.name() );
	stdtype_ = oth.stdtype_;
	logtypename_ = oth.logtypename_;
	aliases_ = oth.aliases_;
	disp_ = oth.disp_;
	uom_ = oth.uom_;
    }
    return *this;
}


bool Mnemonic::operator ==( const Mnemonic& oth ) const
{
    if ( this == &oth )
	return true;

    return stdtype_ == oth.stdtype_ && uom_ == oth.uom_ &&
	   name() == oth.name() &&
	   logtypename_ == oth.logtypename_ &&
	   aliases_ == oth.aliases_ &&
	   disp_ == oth.disp_;
}


bool Mnemonic::operator !=( const Mnemonic& oth ) const
{
    return !(*this == oth);
}


bool Mnemonic::matches( const char* nm, bool matchaliases,
			float* matchval ) const
{
    BufferStringSet nms( name().buf(), logtypename_.buf() );
    if ( matchaliases && !aliases_.isEmpty() )
	nms.append( aliases_ );

    const float val = getMatchValue( nm, nms, !matchaliases );
    if ( matchval )
	*matchval = val;

    return val > 0.f;
}


float Mnemonic::getMatchValue( const char* nm, const BufferStringSet& nms,
			       bool exactmatch, bool hasaltnm )
{
    const StringView nmstr( nm );
    const int sz = nmstr.size();
    if ( sz < 1 || nms.isEmpty() )
	return 0.f;

    const OD::String& mainnm = *nms.first();
    if ( mainnm == nm )
	return 3000.f;
    else if ( !exactmatch && mainnm.matches(nm,OD::CaseInsensitive) )
	return 2000.f;

    if ( hasaltnm && nms.size() > 1 )
    {
	const OD::String& altnm = nms.get( 1 );
	if ( altnm == nm )
	    return 1000.f;
	else if ( !exactmatch && altnm.matches(nm,OD::CaseInsensitive) )
	    return 500.f;
    }

    if ( exactmatch )
	return 0.f;

    const std::string tomatch = nm;
    std::smatch m;
    for ( const auto* findnm : nms )
    {
	if ( findnm->isEmpty() )
	    continue;

	if ( findnm->isEqual(nm,OD::CaseSensitive) )
	    return findnm->size();
	else if ( findnm->isEqual(nm,OD::CaseInsensitive) )
	    return (float)findnm->size() - 1e-4f;

	BufferString gexpr( findnm->buf() );
	gexpr.trimBlanks().replace( " ", "[^a-z]{0,2}" )
			  .replace( "_", "[^a-z]{0,2}" );
	const std::string expr = gexpr.str();
	const std::regex stdge = std::regex( expr, std::regex::icase );
	if ( std::regex_search(tomatch,m,stdge) )
	    return findnm->size();
    }

    return 0.f;
}


bool Mnemonic::isCompatibleWith( const Mnemonic* oth ) const
{
    if ( !oth )
	return false;

    static MnemonicSelection allvols = MnemonicSelection::getAllVolumetrics();
    static MnemonicSelection allpors = MnemonicSelection::getAllPorosity();
    static MnemonicSelection allsats = MnemonicSelection::getAllSaturations();
    return this == oth ||
	   ( allpors.isPresent(this) && allpors.isPresent(oth) ) ||
	   ( allvols.isPresent(this) && allvols.isPresent(oth) ) ||
	   ( allsats.isPresent(this) && allsats.isPresent(oth) );
}


const char* Mnemonic::description() const
{
    return logtypename_.buf();
}


void Mnemonic::usePar( const IOPar& iop )
{
    aliases_.erase();
    fromString( iop.find(name()) );
}


void Mnemonic::fromString( const char* str )
{
    aliases_.erase();
    FileMultiString fms( str );
    const int sz = fms.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( idx == 0 )
	    logtypename_ = fms[idx];
	else if ( idx == 1 )
	    stdtype_ = StdTypeDef().parse( fms[idx] );
	else if ( idx == 2 )
	    disp_.scale_ = Mnemonic::ScaleDef().parse( fms[2] );
	else if ( idx == 3 )
	    disp_.range_.start = fms.getFValue( 3 );
	else if ( idx == 4 )
	    disp_.range_.stop = fms.getFValue( 4 );
	else if ( idx == 5 )
	    disp_.typicalrange_.start = fms.getFValue( 5 );
	else if ( idx == 6 )
	    disp_.typicalrange_.stop = fms.getFValue( 6 );
	else if ( idx == 7 )
	    setUnit( fms[7] );
	else if ( idx >= 8 && idx <= 10 )
	    disp_.color_.set( fms.getFValue(8), fms.getFValue(9),
						fms.getFValue(10) );
	else
	{
	    const StringView fmsstr = fms[idx];
	    if ( !fmsstr.isEmpty() )
		aliases_.add( fmsstr );
	}
    }
}


void Mnemonic::fillPar( IOPar& iop ) const
{
    FileMultiString fms( logtypename_ );
    fms += StdTypeDef().toString( stdtype_ );
    fms += Mnemonic::ScaleDef().toString( disp_.scale_ );
    const Interval<float> vrange( disp_.range_ );
    const Interval<float> vtypicalrange( disp_.typicalrange_ );
    fms.add( vrange.start );
    fms.add( vrange.stop );
    fms.add( vtypicalrange.start );
    fms.add( vtypicalrange.stop );
    const char* unitlbl = disp_.getUnitLbl();
    fms += unitlbl && *unitlbl ? unitlbl : "";
    fms += disp_.color_.r();
    fms += disp_.color_.g();
    fms += disp_.color_.b();
    for ( int idx=0; idx<aliases_.size(); idx++ )
	fms += aliases_.get(idx);

    iop.set( name(), fms );
}


void Mnemonic::setUnit( const char* newunitlbl )
{
    uom_ = UoMR().get( newunitlbl );
    if ( !uom_ )
	uom_ = UoMR().getInternalFor( stdType() );

    disp_.unitlbl_ = UnitOfMeasure::getUnitLbl( uom_, newunitlbl );
}


const Mnemonic& Mnemonic::undef()
{
    mDefineStaticLocalObject( PtrMan<Mnemonic>, udf, = nullptr );
    if ( !udf )
    {
	auto* newudf = new Mnemonic( "OTH", Other );
	if ( udf.setIfNull(newudf,true) )
	{
	    newudf->logtypename_ = "Other";
	    BufferStringSet& aliases = newudf->aliases();
	    aliases.add( "udf" ).add( "unknown" );
	    newudf->disp_.color_ = OD::Color::LightGrey();
	}
    }
    return *udf;
}


const Mnemonic& Mnemonic::distance()
{
    mDefineStaticLocalObject( PtrMan<Mnemonic>, dist, = nullptr );
    if ( !dist )
    {
	auto* ret = new Mnemonic( "DIST", Dist );
	if ( dist.setIfNull(ret,true) )
	{
	    ret->disp_.range_ = Interval<float>( 0.f, mUdf(float) );
	    ret->disp_.typicalrange_ = Interval<float>( 0.f, mUdf(float) );
	    /* NO default unit, as lateral (XY) and vertical(Z) distances
	       may have different units */
	}
    }
    return *dist;
}


const Mnemonic& Mnemonic::volume()
{
    mDefineStaticLocalObject( PtrMan<Mnemonic>, volume, = nullptr );
    if ( !volume )
    {
	auto* ret = new Mnemonic( "VOL", Volum );
	if ( volume.setIfNull(ret,true) )
	{
	    ret->disp_.range_ = Interval<float>( 0.f, mUdf(float) );
	    ret->disp_.typicalrange_ = Interval<float>( 0.f, mUdf(float) );
	}
    }
    return *volume;
}


const Mnemonic& Mnemonic::defDEN() { return *MNC().getByName("RHOB",false); }
const Mnemonic& Mnemonic::defPVEL() { return *MNC().getByName("PVEL",false); }
const Mnemonic& Mnemonic::defSVEL() { return *MNC().getByName("SVEL",false); }
const Mnemonic& Mnemonic::defDT() { return *MNC().getByName("DT",false); }
const Mnemonic& Mnemonic::defDTS() { return *MNC().getByName("DTS",false); }
const Mnemonic& Mnemonic::defPHI() { return *MNC().getByName("PHI",false); }
const Mnemonic& Mnemonic::defSW() { return *MNC().getByName("SW",false); }
const Mnemonic& Mnemonic::defAI() { return *MNC().getByName("AI",false); }
const Mnemonic& Mnemonic::defSI() { return *MNC().getByName("SI",false); }
const Mnemonic& Mnemonic::defVEL() { return *MNC().getByName("VEL",false); }
const Mnemonic& Mnemonic::defTime() { return *MNC().getByName("TWT",false); }
const Mnemonic& Mnemonic::defFracDensity()
{ return *MNC().getByName( "FRACRHO", false ); }
const Mnemonic& Mnemonic::defFracOrientation()
{ return *MNC().getByName( "FRACSTRIKE", false ); }


//------- MnemonicSetMgr ----------

class MnemonicSetMgr : public CallBacker
{
public:

MnemonicSetMgr()
{
    mAttachCB( IOM().surveyChanged, MnemonicSetMgr::doNull );
}


~MnemonicSetMgr()
{
    detachAllNotifiers();
    delete mns_;
}


void doNull( CallBacker* )
{
    delete mns_;
    mns_ = nullptr;
}


void createSet()
{
    Repos::FileProvider rfp( filenamebase, true );
    rfp.setSource(Repos::Source::ApplSetup);
    while ( rfp.next() )
    {
	const BufferString fnm( rfp.fileName() );
	SafeFileIO sfio( fnm );
	if ( !sfio.open(true) )
	    continue;

	ascistream astrm( sfio.istrm(), true );
	MnemonicSet* oldmns_ = mns_;
	mns_ = new MnemonicSet;
	mns_->readFrom( astrm );
	if ( mns_->isEmpty() )
	{
	    delete mns_;
	    mns_ = oldmns_;
	}
	else
	{
	    delete oldmns_;
	    sfio.closeSuccess();
	    break;
	}
    }

    if ( !mns_ )
	mns_ = new MnemonicSet;
}

    MnemonicSet* mns_ = nullptr;

};


const MnemonicSet& MNC()
{
    mDefineStaticLocalObject( MnemonicSetMgr, msm, );
    if ( !msm.mns_)
	msm.createSet();

    return *msm.mns_;
}


//------- MnemonicSet ----------

MnemonicSet::MnemonicSet()
    : ManagedObjectSet<Mnemonic>()
{
}


Mnemonic* MnemonicSet::getByName( const char* nm, bool matchaliases )
{
    const Mnemonic* ret =
	const_cast<const MnemonicSet*>(this)->getByName( nm, matchaliases );
    return const_cast<Mnemonic*>( ret );
}


using MnemonicsCache = QHash<QString,const Mnemonic*>;

MnemonicsCache& getMnemonicLookupCache( bool matchaliases )
{
    if ( matchaliases )
    {
	mDefineStaticLocalObject( MnemonicsCache, macache, );
	return macache;
    }

    mDefineStaticLocalObject( MnemonicsCache, cache, );
    return cache;
}


const Mnemonic* MnemonicSet::getByName( const char* nm,
					bool matchaliases ) const
{
    MnemonicsCache& cache = getMnemonicLookupCache( matchaliases );
    const QString qstr( nm );
    if ( cache.contains(qstr) )
	return cache[qstr];

    MnemonicSelection mnsel;
    for ( const auto* mnc : *this )
	mnsel.add( mnc );

    const Mnemonic* ret = getByName( nm, mnsel, matchaliases );
    if ( ret || !cache.empty() )
	cache[qstr] = ret;

    return ret;
}


const Mnemonic* MnemonicSet::getByName( const char* nm,
					const ObjectSet<const Mnemonic>& mnsel,
					bool matchaliases )
{
    const StringView nmstr( nm );
    const int sz = nmstr.size();
    if ( sz < 1 )
	return nullptr;

    float val;
    TypeSet<OD::Pair<float,const Mnemonic*> > mnemmatchvals;
    int maxidx = -1; float maxval = -1.f;
    for ( const auto* mnc : mnsel )
    {
	if ( !mnc || !mnc->matches(nm,matchaliases,&val) )
	    continue;

	mnemmatchvals += OD::Pair<float,const Mnemonic*>( val, mnc );
	if ( val > maxval )
	{
	    maxval = val;
	    maxidx = mnemmatchvals.size()-1;
	}
    }

    if ( mnemmatchvals.isEmpty() )
	return nullptr;

    const Mnemonic* ret = mnemmatchvals.validIdx( maxidx )
			? mnemmatchvals[maxidx].second() : nullptr;
    return ret;
}


void MnemonicSet::getNames( BufferStringSet& names ) const
{
    for ( const auto* mnc : *this )
	names.add( mnc->name() );
}


MnemonicSet& MnemonicSet::doAdd( Mnemonic* mn )
{
    if ( !mn || getByName(mn->name(),false) )
    {
	delete mn;
	return *this;
    }

    ObjectSet<Mnemonic>::doAdd( mn );
    return *this;
}


void MnemonicSet::readFrom( ascistream& astrm )
{
    while ( !atEndOfSection(astrm.next()) )
    {
	while ( !astrm.atEOS() )
	{
	    const BufferString mnemonicnm( astrm.keyWord() );
	    if ( !getByName(mnemonicnm,false) )
	    {
		auto* mnc = new Mnemonic( mnemonicnm, Mnemonic::Other );
		mnc->fromString( astrm.value() );
		add( mnc );
	    }

	    astrm.next();
	}
    }
}


//------- MnemonicSelection ----------

MnemonicSelection::MnemonicSelection()
    : ObjectSet<const Mnemonic>()
{
}


MnemonicSelection::MnemonicSelection( const Mnemonic* exclude )
    : ObjectSet<const Mnemonic>()
{
    const MnemonicSet& mns = MNC();
    for ( const auto* mnc : mns )
    {
	if ( mnc != exclude )
	    add( mnc );
    }

    if ( !exclude )
	add( &Mnemonic::undef() );
}


MnemonicSelection::MnemonicSelection( const Mnemonic::StdType stdtyp )
    : ObjectSet<const Mnemonic>()
{
    const MnemonicSet& mns = MNC();
    for ( const auto* mnc : mns )
    {
	if ( mnc->stdType() == stdtyp )
	    add( mnc );
    }

    if ( stdtyp == Mnemonic::Other )
	add( &Mnemonic::undef() );
}


MnemonicSelection::MnemonicSelection( const UnitOfMeasure& uom )
    : ObjectSet<const Mnemonic>()
{
    const MnemonicSet& mns = MNC();
    TypeSet<Mnemonic::StdType> uomtypes;
    for ( int idx=0; idx<uom.nrTypes(); idx++ )
	uomtypes += uom.propType( idx );

    for ( const auto* mnc : mns )
    {
	if ( uomtypes.isPresent(mnc->stdType()) )
	    add( mnc );
    }

    if ( isEmpty() )
	add( &Mnemonic::undef() );
}


void MnemonicSelection::getNames( BufferStringSet& names ) const
{
    for ( const auto* mnc : *this )
	names.addIfNew( mnc->name() );
}


const Mnemonic* MnemonicSelection::getByName( const char* nm,
					      bool matchaliases ) const
{
    return MnemonicSet::getByName( nm, *this, matchaliases );
}


void MnemonicSelection::fillPar( IOPar& iop ) const
{
    BufferStringSet mnnms;
    for ( const auto* mnc : *this )
	mnnms.addIfNew( mnc->name() );

    iop.set( sKey::Mnemonics(), mnnms );
}


bool MnemonicSelection::usePar( const IOPar& iop )
{
    MnemonicSelection newmns;
    BufferStringSet mnnms;
    if ( !iop.get( sKey::Mnemonics(),mnnms ) )
	return false;

    bool ret = !mnnms.isEmpty();
    for ( const auto* nm : mnnms )
    {
	const Mnemonic* mn = MNC().getByName( *nm, false );
	if ( mn )
	    addIfNew(mn);
	else
	    ret = false;
    }

    return ret;
}


MnemonicSelection MnemonicSelection::getAllVolumetrics()
{
    MnemonicSelection mnsel( Mnemonic::Volum );
    for ( int idx=mnsel.size()-1; idx>=0; idx-- )
    {
	const Mnemonic& mn = *mnsel.get( idx );
	const StringView desc( mn.description() );
	if ( !desc.startsWith("Volume of ") &&
	     !desc.startsWith("Volume Fraction") )
	    mnsel.removeSingle( idx );
    }

    return mnsel;
}


MnemonicSelection MnemonicSelection::getGroupFor( const Mnemonic& mn )
{
    MnemonicSelection mnsel( mn.stdType() );
    if ( mn.stdType() != Mnemonic::Volum )
	return mnsel;

    const MnemonicSelection allsats = getAllSaturations();
    const MnemonicSelection allpors = getAllPorosity();
    if ( allsats.isPresent(&mn) )
	mnsel = allsats;
    else if ( allpors.isPresent(&mn) )
	mnsel = allpors;
    else
    {
	for ( const auto* thismn : allsats )
	    mnsel -= thismn;
	for ( const auto* thismn : allpors )
	    mnsel -= thismn;
    }

    return mnsel;
}


MnemonicSelection MnemonicSelection::getAllSaturations()
{
    MnemonicSelection mnsel( Mnemonic::Volum );
    for ( int idx=mnsel.size()-1; idx>=0; idx-- )
    {
	const Mnemonic& mn = *mnsel.get( idx );
	const StringView desc( mn.description() );
	if ( !desc.contains("Water Saturation") &&
	     desc != "Oil Saturation" && desc != "Gas Saturation" )
	    mnsel.removeSingle( idx );
    }

    return mnsel;
}


MnemonicSelection MnemonicSelection::getAllPorosity()
{
    MnemonicSelection mnsel( Mnemonic::Volum );
    for ( int idx=mnsel.size()-1; idx>=0; idx-- )
    {
	const Mnemonic& mn = *mnsel.get( idx );
	const StringView desc( mn.description() );
	if ( !desc.contains("Porosity") )
	    mnsel.removeSingle( idx );
    }

    return mnsel;
}


const Mnemonic* MnemonicSelection::getGuessed( const char* nm,
		const Mnemonic::StdType typ, const BufferStringSet* hintnms )
{
    const MnemonicSelection mnsel( typ );
    return mnsel.getGuessed( nm, hintnms );
}


const Mnemonic* MnemonicSelection::getGuessed( const char* nm,
		const UnitOfMeasure* uom, const BufferStringSet* hintnms )
{
    MnemonicSelection mnsel;
    if ( uom )
	mnsel = MnemonicSelection( *uom );
    else
	mnsel = MnemonicSelection( nullptr );

    return mnsel.getGuessed( nm, hintnms );
}


const Mnemonic* MnemonicSelection::getGuessed( const char* nm,
					const BufferStringSet* hintnms ) const
{
    const Mnemonic* ret = getByName( nm, false );
    if ( !ret )
	ret = getByName( nm );

    if ( !ret && hintnms )
    {
	for ( const auto* hintnm : *hintnms )
	{
	    ret = getByName( hintnm->buf() );
	    if ( ret )
		break;
	}
    }

    return ret ? ret : ( isEmpty() ? nullptr : first() );
}

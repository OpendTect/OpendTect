/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mnemonics.h"

#include "ascstream.h"
#include "draw.h"
#include "filepath.h"
#include "genc.h"
#include "keystrs.h"
#include "ioman.h"
#include "oddirs.h"
#include "odpair.h"
#include "safefileio.h"
#include "separstr.h"
#include "settings.h"
#include "unitofmeasure.h"

#include <QHash>
#include <QRegularExpression>
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
    convValue( range_.start_, olduom, newuom );
    convValue( range_.stop_, olduom, newuom );
    convValue( typicalrange_.start_, olduom, newuom );
    convValue( typicalrange_.stop_, olduom, newuom );

    return true;
}


float Mnemonic::DispDefs::commonValue() const
{
    const Interval<float>& range = defRange();
    const bool udf0 = mIsUdf(range.start_);
    const bool udf1 = mIsUdf(range.stop_);
    if ( udf0 && udf1 )
	return 0.f;

    if ( udf0 || udf1 )
	return udf0 ? range.stop_ : range.start_;

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
	"Force",
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
{}


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
	source_ = oth.source_;
	origin_ = oth.origin_;
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


Mnemonic* Mnemonic::getFromTemplate( const Mnemonic& oth,const char* customname,
				     Repos::Source src )
{
    if ( !oth.isTemplate() )
	return nullptr;

    auto* ret = new Mnemonic( oth );
    ret->setName( customname );
    ret->source_ = src;
    ret->origin_ = &oth;
    return ret;
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

    const QString qtomatch = nm;
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

	const QString expr = gexpr.str();
	const QRegularExpression qge( expr,
				QRegularExpression::CaseInsensitiveOption );
	const QRegularExpressionMatch match = qge.match( qtomatch );
	if ( match.hasMatch() )
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


BufferString Mnemonic::getUserMnemonicsFileName()
{
    BufferString mnemfile;
    FilePath dd_fp( GetDataDir(), BufferString(sKey::Mnemonics(), "_",
					       GetInterpreterName()) );
    if ( dd_fp.exists() )
	mnemfile = dd_fp.fullPath();
    else
    {
	FilePath bdd_fp( dd_fp );
	bdd_fp.setPath( GetBaseDataDir() );
	if ( bdd_fp.exists() )
	    mnemfile = bdd_fp.fullPath();
	else
	    mnemfile = dd_fp.fullPath();
    }

    return mnemfile;
}


IOPar Mnemonic::getUserMnemonics()
{
    IOPar iop;
    iop.read( getUserMnemonicsFileName(), sKey::Mnemonics(), true );
    return iop;
}


void Mnemonic::setUserMnemonics( const IOPar& mnemsetts )
{
    mnemsetts.write( getUserMnemonicsFileName(), sKey::Mnemonics() );
}


const UnitOfMeasure* Mnemonic::getDisplayInfo( Mnemonic::Scale& scale,
					       Interval<float>& range,
					       BufferString& unitlbl,
					       OD::LineStyle& lstyle ) const
{
    return getDisplayInfo( getUserMnemonics(), scale, range, unitlbl, lstyle );
}


const UnitOfMeasure* Mnemonic::getDisplayInfo( const IOPar& mnemsetts,
					       Mnemonic::Scale& scale,
					       Interval<float>& range,
					       BufferString& unitlbl,
					       OD::LineStyle& lstyle ) const
{
    const FileMultiString fms( mnemsetts.find(name()) );

    const UnitOfMeasure* uom = nullptr;
    int idx = 0;
    if ( fms.size() == 9 )
    {
	parseEnum( fms[idx++], scale );
	range.start_ = fms.getFValue( idx++ );
	range.stop_ = fms.getFValue( idx++ );
	unitlbl = fms[ idx++];
	uom = UoMR().get( unitlbl );
	OD::LineStyle::parseEnum( fms[idx++], lstyle.type_ );
	lstyle.width_ = fms.getIValue( idx++ );
	lstyle.color_ = OD::Color( fms.getIValue(idx), fms.getIValue(idx+1),
				   fms.getIValue(idx+2) );
    }
    else
    {
	scale = disp_.scale_;
	lstyle.color_ = disp_.color_;
	range = disp_.typicalrange_;
	unitlbl = disp_.getUnitLbl();
	uom = unit();
    }

    return uom;
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
	    disp_.range_.start_ = fms.getFValue( 3 );
	else if ( idx == 4 )
	    disp_.range_.stop_ = fms.getFValue( 4 );
	else if ( idx == 5 )
	    disp_.typicalrange_.start_ = fms.getFValue( 5 );
	else if ( idx == 6 )
	    disp_.typicalrange_.stop_ = fms.getFValue( 6 );
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
    fms.add( vrange.start_ );
    fms.add( vrange.stop_ );
    fms.add( vtypicalrange.start_ );
    fms.add( vtypicalrange.stop_ );
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
	    newudf->source_ = Repos::ApplSetup;
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
	    ret->source_ = Repos::ApplSetup;
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
	    ret->source_ = Repos::ApplSetup;
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
const Mnemonic& Mnemonic::defMD() { return *MNC().getByName("MD",false); }
const Mnemonic& Mnemonic::defTVD() { return *MNC().getByName("TVD",false); }
const Mnemonic& Mnemonic::defTVDSS() { return *MNC().getByName("TVDSS",false); }
const Mnemonic& Mnemonic::defTVDSD() { return *MNC().getByName("TVDSD",false); }
const Mnemonic& Mnemonic::defTVDGL() { return *MNC().getByName("TVDGL",false); }

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

//------- MnemonicSetMgr ----------

class MnemonicSetMgr : public CallBacker
{
public:

MnemonicSetMgr()
{
    if ( !NeedDataBase() )
	return;

    if ( IOMan::isOK() )
	iomReadyCB( nullptr );
    else
	mAttachCB( IOMan::iomReady(), MnemonicSetMgr::iomReadyCB );
}


~MnemonicSetMgr()
{
    detachAllNotifiers();
    delete mns_;
}


void iomReadyCB( CallBacker* )
{
    mAttachCB( IOM().surveyChanged, MnemonicSetMgr::removeSurveyMnemonics );
    mAttachCB( IOM().afterSurveyChange, MnemonicSetMgr::addSurveyMnemonics );
}


void createSet()
{
    const Repos::Source src = Repos::ApplSetup;
    Repos::FileProvider rfp( filenamebase, true );
    rfp.setSource( src );
    rfp.next();

    const BufferString fnm( rfp.fileName() );
    SafeFileIO sfio( fnm );
    if ( sfio.open(true) )
    {
	ascistream astrm( sfio.istrm(), true );
	MnemonicSet* oldmns_ = mns_;
	mns_ = new MnemonicSet;
	mns_->readFrom( astrm );
	mns_->setSource( src );
	sfio.closeSuccess();
	if ( mns_->isEmpty() )
	{
	    delete mns_;
	    mns_ = oldmns_;
	}
	else
	{
	    delete oldmns_;
	    addSurveyMnemonics( nullptr );
	}
    }

    if ( !mns_ )
	mns_ = new MnemonicSet;
}

void addSurveyMnemonics( CallBacker* )
{
    if ( !IOMan::isOK() || !mns_ )
	return;

    PtrMan<IOPar> iop = SI().getPars().subselect( sKey::Mnemonics() );
    if ( !iop )
	return;

    int idx = 0;
    while ( true )
    {
	FileMultiString fms;
	if ( !iop->get(toString(idx++),fms) )
	    return;
	if ( fms.size() < 2 )
	    continue;

	const Mnemonic* origin = mns_->getByName( fms[1], false );
	if ( !origin )
	    continue;

	Mnemonic* mn = Mnemonic::getFromTemplate( *origin, fms[0],
						  Repos::Survey );
	if ( fms.size() > 2 )
	{
	    const OD::Color fcolor( fms.getFValue(2),
				    fms.getFValue(3), fms.getFValue(4) );
	    if ( fcolor != origin->disp_.color_ )
		mn->disp_.color_ = fcolor;
	}

	mns_->add( mn );
    }
}

void removeSurveyMnemonics( CallBacker* )
{
    if ( !mns_ )
	return;

    for ( int idx=mns_->size()-1; idx>=0; idx-- )
	if ( !mns_->get(idx)->isTemplate() )
	    mns_->removeSingle( idx );
}

    MnemonicSet* mns_ = nullptr;

};


const MnemonicSet& MNC()
{
    mDefineStaticLocalObject( MnemonicSetMgr, msm, );
    if ( !msm.mns_ )
	msm.createSet();

    return *msm.mns_;
}


//------- MnemonicSet ----------

MnemonicSet::MnemonicSet()
    : ManagedObjectSet<Mnemonic>()
    , customMnemonicRemoved(this)
{
}


void MnemonicSet::erase()
{
    getMnemonicLookupCache( true ).clear();
    getMnemonicLookupCache( false ).clear();
    return ManagedObjectSet<Mnemonic>::erase();
}


Mnemonic* MnemonicSet::pop()
{
    if ( !isEmpty() )
	removeCache( first() );

    return ManagedObjectSet<Mnemonic>::pop();
}


Mnemonic* MnemonicSet::removeSingle( int idx, bool keep_order )
{
    const Mnemonic* mn = get( idx );
    if ( !mn )
	return nullptr;

    if ( !mn->isTemplate() )
	customMnemonicRemoved.trigger( *mn );

    removeCache( mn );
    return ManagedObjectSet<Mnemonic>::removeSingle( idx, keep_order );
}


void MnemonicSet::removeRange( int from, int to )
{
    for ( int idx=from; idx<=to; idx++ )
    {
	const Mnemonic* mn = get( idx );
	if ( !mn )
	    continue;

	if ( !mn->isTemplate() )
	    customMnemonicRemoved.trigger( *mn );

	removeCache( get(idx) );
    }

    return ManagedObjectSet<Mnemonic>::removeRange( from, to );
}


Mnemonic* MnemonicSet::replace( int idx, Mnemonic* oth )
{
    const Mnemonic* mn = get( idx );
    if ( !mn )
	return nullptr;

    if ( !mn->isTemplate() )
	customMnemonicRemoved.trigger( *mn );

    removeCache( get(idx) );
    return ManagedObjectSet<Mnemonic>::replace( idx, oth );
}


Mnemonic* MnemonicSet::removeAndTake( int idx, bool keep_order )
{
    const Mnemonic* mn = get( idx );
    if ( !mn )
	return nullptr;

    if ( !mn->isTemplate() )
	customMnemonicRemoved.trigger( *mn );

    removeCache( get(idx) );
    return ManagedObjectSet<Mnemonic>::removeAndTake( idx, keep_order );
}


ManagedObjectSetBase<Mnemonic>& MnemonicSet::operator -=( Mnemonic* mn )
{
    if ( !mn )
	return ManagedObjectSet<Mnemonic>::operator-=( nullptr );

    if ( !mn->isTemplate() )
	customMnemonicRemoved.trigger( *mn );

    removeCache( mn );
    return ManagedObjectSet<Mnemonic>::operator -=( mn );
}


void MnemonicSet::removeCache( const Mnemonic* mn )
{
    if ( mn )
    {
	const QString qstr( mn->name() );
	getMnemonicLookupCache( true ).remove( qstr );
	getMnemonicLookupCache( false ).remove( qstr );
    }
}


Mnemonic* MnemonicSet::getByName( const char* nm, bool matchaliases )
{
    const Mnemonic* ret =
	const_cast<const MnemonicSet*>(this)->getByName( nm, matchaliases );
    return const_cast<Mnemonic*>( ret );
}


const Mnemonic* MnemonicSet::getByName( const char* nm,
					bool matchaliases ) const
{
    MnemonicsCache& cache = getMnemonicLookupCache( matchaliases );
    const QString qstr( nm );
    if ( cache.contains(qstr) && cache[qstr] )
	return cache[qstr];

    MnemonicSelection mnsel;
    for ( const auto* mnc : *this )
	mnsel.add( mnc );

    const Mnemonic* ret = getByName( nm, mnsel, matchaliases );
    if ( ret )
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


void MnemonicSet::setSource( Repos::Source src )
{
    for ( auto* mn : *this )
	mn->source_ = src;
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
    const Mnemonic* ret = nullptr;
    if ( nm && *nm )
    {
	ret = getByName( nm, false );
	if ( !ret )
	    ret = getByName( nm );
    }

    if ( !ret && hintnms )
    {
	for ( const auto* hintnm : *hintnms )
	{
	    ret = getByName( hintnm->buf() );
	    if ( ret )
		break;
	}
    }

    return ret;
}


void MnemonicSelection::sort()
{
    BufferStringSet mnemnms;
    getNames( mnemnms );
    ConstArrPtrMan<int> sortindexes = mnemnms.getSortIndexes();
    if ( sortindexes )
	useIndexes( sortindexes.ptr() );
}

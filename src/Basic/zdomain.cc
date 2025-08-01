/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "zdomain.h"

#include "compoundkey.h"
#include "keystrs.h"
#include "odruncontext.h"
#include "perthreadrepos.h"
#include "settings.h"
#include "survinfo.h"
#include "uistrings.h"


const char* ZDomain::sKey()		{ return "ZDomain"; }
const char* ZDomain::sKeyNoAll()	{ return "ZDomainNoAll"; }
const char* ZDomain::sKeyTime()		{ return "TWT"; }
const char* ZDomain::sKeyDepth()	{ return "Depth"; }
const char* ZDomain::sKeyUnit()		{ return "ZDomain.Unit"; }
const char* ZDomain::sKeyDec()		{ return "Nr Decimals"; }

//Must match data/UnitsOfMeasure:
static const char* sKeySeconds = "Seconds";
static const char* sKeySecondsSymbol = "s";
static const char* sKeyMeter = "Meter";
static const char* sKeyMeterSymbol = "m";
static const char* sKeyFeet = "Feet";
static const char* sKeyFeetSymbol = "ft";

ObjectSet<const ZDomain::Def>& DEFS()
{
    static ObjectSet<const ZDomain::Def> defs;
    if ( defs.isEmpty() )
    {
	defs.add( &ZDomain::Time() );
	defs.add( &ZDomain::Depth() );
    }

    return defs;
}


ObjectSet<const ZDomain::Info>& ZDOMAINS()
{
    static ManagedObjectSet<const ZDomain::Info> zinfos;
    return zinfos;
}


const ZDomain::Def& ZDomain::SI()
{
    return ::SI().zIsTime() ? Time() : Depth();
}


const ZDomain::Def& ZDomain::Time()
{
    static Def def( sKeyTime(), uiStrings::sTime(), "ms", 1000 );
    return def;
}


const ZDomain::Def& ZDomain::Depth()
{
    static Def def( sKeyDepth(), uiStrings::sDepth(), "", 1 );
    return def;
}

namespace ZDomain
{

static bool zIsTime( const IOPar& iop, bool* isfound =nullptr )
{
    BufferString domstr;
    if ( iop.get(sKey(),domstr) &&
	 (domstr == Time().key() || domstr == Depth().key() ||
	  domstr == sKey::Time()) )
    {
	if ( isfound ) *isfound = true;
	return domstr == Time().key() || domstr == sKey::Time();
    }

    bool zistime;
    if ( iop.getYN("Z Is Time",zistime) )
    {
	if ( isfound ) *isfound = true;
	return zistime;
    }
    else if ( iop.getYN("Z Is time",zistime) )
    {
	if ( isfound ) *isfound = true;
	return zistime;
    }

    BufferString unitstr;
    if ( iop.get(sKey::ZUnit(),unitstr) && !unitstr.isEmpty() )
    {
	if ( unitstr.isEqual(sKeySeconds,OD::CaseInsensitive) ||
	     unitstr.isEqual(sKeySecondsSymbol,OD::CaseInsensitive) )
	{
	    if ( isfound ) *isfound = true;
	    return true;
	}
	else if ( unitstr.isEqual(sKeyMeter,OD::CaseInsensitive) ||
		  unitstr.isEqual(sKeyMeterSymbol,OD::CaseInsensitive) ||
		  unitstr.isEqual(sKeyFeet,OD::CaseInsensitive) ||
		  unitstr.isEqual(sKeyFeetSymbol,OD::CaseInsensitive) )
	{
	    if ( isfound ) *isfound = true;
	    return false;
	}
    }

    if ( isfound ) *isfound = false;
    return ::SI().zIsTime();
}


static const ZDomain::Info* get( const IOPar& iop )
{
    bool isfound = false;
    const bool zit = zIsTime( iop, &isfound );
    if ( !isfound )
    {
	const ObjectSet<const Info>& otherzdoms = ZDOMAINS();
	const BufferString keystr( iop.find(sKey()) );
	if ( keystr.isEmpty() )
	    return nullptr;

	for ( const auto* zinfo : otherzdoms )
	    if ( keystr == zinfo->key() )
		return zinfo;

	return nullptr;
    }

    // Below: time or depth only
    if ( zit )
	return &TWT();

    BufferString unitstr;
    if ( !iop.get(sKeyUnit(),unitstr) || unitstr.isEmpty() )
	if ( !iop.get(sKey::ZUnit(),unitstr) || unitstr.isEmpty() )
	    return &DefaultDepth(false);

    if ( unitstr.isEqual(sKeyMeter,OD::CaseInsensitive) ||
	 unitstr.isEqual(sKeyMeterSymbol,OD::CaseInsensitive) )
	return &DepthMeter();
    else if ( unitstr.isEqual(sKeyFeet,OD::CaseInsensitive) ||
	      unitstr.isEqual(sKeyFeetSymbol,OD::CaseInsensitive) )
	return &DepthFeet();

    return &DefaultDepth(false);
}

} // namespace ZDomain


const ZDomain::Info& ZDomain::Info::getFrom( const ZDomain::Info& oth )
{
    const ObjectSet<const Info>& zinfos = sAll();
    for ( const auto* zinfo : zinfos )
	if ( zinfo->isCompatibleWith(oth) )
	    return *zinfo;

    return ::SI().zDomainInfo();
}


const ZDomain::Info& ZDomain::Info::getFrom( const char* zdomkey,
					     const char* zunitstr )
{
    if ( !zdomkey || !*zdomkey )
	return ::SI().zDomainInfo();

    IOPar iop;
    iop.set( sKey(), zdomkey );
    if ( zunitstr && *zunitstr )
	iop.set( sKeyUnit(), zunitstr );

    const Info* ret = getFrom( iop );
    return ret ? *ret : ::SI().zDomainInfo();
}


const ZDomain::Info* ZDomain::Info::getFrom( const IOPar& iop )
{
    const BufferString keystr( iop.find(sKey()) );
    if ( keystr.isEmpty() )
	return nullptr;

    const ObjectSet<const Info>& zinfos = sAll();
    for ( const auto* zinfo : zinfos )
	if ( zinfo->isCompatibleWith(iop) )
	    return zinfo;

    return nullptr;
}


bool ZDomain::isSI( const IOPar& iop )
{
    const ZDomain::Info* zinfo = get( iop );
    if ( !zinfo )
	return true;

    return (zinfo->isTime() && ::SI().zIsTime()) ||
	   (zinfo->isDepthMeter() && ::SI().zInMeter()) ||
	   (zinfo->isDepthFeet() && ::SI().zInFeet());
}


bool ZDomain::isDepth( const IOPar& iop )
{
    return !zIsTime( iop );
}


bool ZDomain::isTime( const IOPar& iop )
{
    return zIsTime( iop );
}


void ZDomain::setSI( IOPar& iop )
{
    iop.set( sKey(), SI().key() );
}


void ZDomain::setDepth( IOPar& iop )
{
    iop.set( sKey(), Depth().key() );
}


void ZDomain::setTime( IOPar& iop )
{
    iop.set( sKey(), Time().key() );
}


// ZDomain::Def

ZDomain::Def::Def( const char* ky, const uiString& usrnm, const char* defun,
		   int usrfac )
    : key_(ky)
    , usrnm_(usrnm)
    , defunit_(defun)
    , usrfac_(usrfac)
{
}


ZDomain::Def::~Def()
{
}


bool ZDomain::Def::isSI() const
{
    return *this == ::SI().zDomain();
}


bool ZDomain::Def::isTime() const
{
    return key_ == sKeyTime();
}


bool ZDomain::Def::isDepth() const
{
    return key_ == sKeyDepth();
}


void ZDomain::Def::set( IOPar& iop ) const
{
    iop.set( sKey(), key_ );
}


uiString ZDomain::Def::getLabel() const
{
    const uiString usrnm = userName();
    const uiString unitstr = uiUnitStr( true );
    if ( unitstr.isEmpty() )
	return usrnm;
    else
	return uiStrings::phrJoinStrings( usrnm, unitstr );
}


uiString ZDomain::Def::getRange() const
{
    return uiStrings::phrJoinStrings( userName(),
				      uiStrings::sRange().toLower() );
}


const char* ZDomain::Def::unitStr( bool withparens ) const
{
    mDeclStaticString( ret );
    if ( isDepth() )
	ret = getDistUnitString( ::SI().depthsInFeet(), withparens );
    else
    {
	if ( defunit_.isEmpty() )
	    ret.setEmpty();
	else
	{
	    ret.set( defunit_ );
	    if ( withparens )
		ret.embed('(',')');
	}
    }

    return ret.buf();
}


uiString ZDomain::Def::uiUnitStr( bool withparens ) const
{
    uiString ret;
    if ( isDepth() )
    {
	const bool abbreviated = true;
	return uiStrings::sDistUnitString( ::SI().depthsInFeet(), abbreviated,
					   withparens );
    }

    if ( defunit_.isEmpty() )
	return uiString::empty();

    ret = ::toUiString( defunit_ );
    if ( withparens )
	ret.parenthesize();
    return ret;
}


int ZDomain::Def::nrZDecimals( float zstepfp ) const
{
    const double zstep = sCast( double, zstepfp * userFactor() );
    int nrdec = 0;
    double decval = zstep;
    while ( decval > Math::Floor(decval) &&
	    !mIsZero(decval,1e-4) && !mIsEqual(decval,1.,1e-4) )
    {
	nrdec++;
	decval = decval*10 - Math::Floor(decval*10);
    }

    return nrdec;
}


const ZDomain::Def& ZDomain::Def::get( const char* ky )
{
    if ( !ky || !*ky )
	return ZDomain::SI();

    if ( *ky == '`' )
	ky++; // cope with "`TWT"

    const ObjectSet<const ZDomain::Def>& defs = DEFS();
    for ( const auto* def : defs )
	if ( def->key_ == ky )
	    return *def;

    return ZDomain::SI();
}


const ZDomain::Def& ZDomain::Def::get( const IOPar& iop )
{
    return get( iop.find(sKey()) );
}


bool ZDomain::Def::add( ZDomain::Def* newdef )
{
    if ( !newdef )
	return false;

    ObjectSet<const ZDomain::Def>& defs = DEFS();
    for ( const auto* def : defs )
	if ( def->key_ == newdef->key_ )
	    { delete newdef; return false; }

    ObjectSet<const ZDomain::Info>& allinfos = getNonConst( sAll() );
    defs.add( newdef );
    ZDOMAINS().add( new ZDomain::Info(*newdef) );
    allinfos.add( ZDOMAINS().last() );
    return true;
}


// ZDomain::Info

ObjectSet<const ZDomain::Info>& INFOS()
{
    static ObjectSet<const ZDomain::Info> infos;
    if ( infos.isEmpty() )
    {
	infos.add( &ZDomain::TWT() );
	infos.add( &ZDomain::DepthMeter() );
	infos.add( &ZDomain::DepthFeet() );
    }

    return infos;
}


const ZDomain::Info& ZDomain::TWT()
{
    static Info zinfo( Time(), sKeySeconds );
    return zinfo;
}


const ZDomain::Info& ZDomain::DepthMeter()
{
    static Info zinfo( Depth(), sKeyMeter );
    return zinfo;
}


const ZDomain::Info& ZDomain::DepthFeet()
{
    static Info zinfo( Depth(), sKeyFeet );
    return zinfo;
}


const ZDomain::Info& ZDomain::DefaultDepth( bool display,
					    const SurveyInfo* extsi )
{
    const SurveyInfo& si = extsi ? *extsi : ::SI();
    const bool zistime = si.zIsTime();
    const bool depthinft = si.depthsInFeet();
    const bool zinft = zistime ? depthinft : si.zInFeet();
    return display ? (depthinft ? DepthFeet() : DepthMeter())
		   : (zinft ? DepthFeet() : DepthMeter());
}


const ObjectSet<const ZDomain::Info>& ZDomain::sAll()
{
    static ObjectSet<const ZDomain::Info> zinfos;
    if ( zinfos.isEmpty() )
    {
	zinfos.append( INFOS() );
	zinfos.append( ZDOMAINS() );
    }

    return zinfos;
}


ZDomain::Info::Info( const Def& def, const char* unitstr )
    : def_(def)
    , pars_(*new IOPar)
{
    if ( unitstr && *unitstr )
	pars_.set( sKeyUnit(), unitstr );
    else
	setDefaultUnit();

    createNrDecStr();
}


ZDomain::Info::Info( const ZDomain::Info& oth )
    : def_(oth.def_)
    , pars_(*new IOPar(oth.pars_))
    , nrdecstr_(oth.nrdecstr_)
    , nrdec_(oth.nrdec_)
{
}


ZDomain::Info::Info( const IOPar& iop )
    : def_(ZDomain::Def::get(iop))
    , pars_(*new IOPar(iop))
{
    pars_.removeWithKey( sKey() );
    if ( !pars_.isPresent(sKeyUnit()) )
	setDefaultUnit();

    createNrDecStr();
}


ZDomain::Info::~Info()
{
    delete &pars_;
}


bool ZDomain::Info::operator ==( const Info& oth ) const
{
    if ( &oth == this )
	return true;

    return def_ == oth.def_ && pars_ == oth.pars_;
}


bool ZDomain::Info::operator !=( const Info& oth ) const
{
    return !(oth == *this);
}


void ZDomain::Info::setDefaultUnit()
{
    if ( isTime() )
	pars_.set( sKeyUnit(), sKeySeconds );
    else if ( isDepth() )
	setDepthUnit( ::SI().depthType() );
}


void ZDomain::Info::createNrDecStr()
{
    CompoundKey keystr( sKey() );
    keystr += key();
    const StringView unitstr( unitStr() );
    if ( !unitstr.isEmpty() )
	keystr += unitStr();

    keystr += sKeyDec();
    nrdecstr_ = keystr.buf();
}


bool ZDomain::Info::hasID() const
{
    return !getID().isUdf();
}


const char* ZDomain::Info::unitStr( bool wp ) const
{
    mDeclStaticString( ret );
    if ( isDepthMeter() )
    {
	ret.set( getDistUnitString( false, wp ) );
	return ret.buf();
    }
    if ( isDepthFeet() )
    {
	ret.set( getDistUnitString( true, wp ) );
	return ret.buf();
    }

    return def_.unitStr( wp );
}


uiString ZDomain::Info::uiUnitStr( bool wp ) const
{
    const bool abbrevated = true;
    if ( isDepthMeter() )
	return uiStrings::sDistUnitString( false, abbrevated, wp );
    if ( isDepthFeet() )
	return uiStrings::sDistUnitString( true, abbrevated, wp );

    return def_.uiUnitStr( wp );
}


uiString ZDomain::Info::getLabel() const
{
    const uiString usrnm = userName();
    const uiString unitstr = uiUnitStr(true);
    if ( unitstr.isEmpty() )
	return usrnm;
    else
	return uiStrings::phrJoinStrings( usrnm, unitstr );
}


uiString ZDomain::Info::getRange( bool withunit ) const
{
    uiString ret = def_.getRange();
    if ( withunit )
    {
	const uiString unitstr = uiUnitStr(true);
	if ( !unitstr.isEmpty() )
	    ret.appendPhrase( uiUnitStr(true), uiString::Space,
			      uiString::OnSameLine );
    }

    return ret;
}


ZDomain::TimeType ZDomain::Info::timeType() const
{
    return TimeType::Seconds;
}


ZDomain::DepthType ZDomain::Info::depthType() const
{
    if ( isDepthFeet() )
	return DepthType::Feet;

    return DepthType::Meter;
}


bool ZDomain::Info::isDepthMeter() const
{
    if ( !isDepth() || !pars_.isPresent(sKeyUnit()) )
	return false;

    const BufferString unitstr = pars_.find( sKeyUnit() );
    return unitstr.isEqual( sKeyMeter, OD::CaseInsensitive ) ||
	   unitstr.isEqual( sKeyMeterSymbol, OD::CaseInsensitive );
}


bool ZDomain::Info::isDepthFeet() const
{
    if ( !isDepth() || !pars_.isPresent(sKeyUnit()) )
	return false;

    const BufferString unitstr = pars_.find( sKeyUnit() );
    return unitstr.isEqual( sKeyFeet, OD::CaseInsensitive ) ||
	   unitstr.isEqual( sKeyFeetSymbol, OD::CaseInsensitive );
}


void ZDomain::Info::setDepthUnit( DepthType typ )
{
    if ( !isDepth() )
	return;

    pars_.set( sKeyUnit(), typ == DepthType::Feet ? sKeyFeet : sKeyMeter );
}


const MultiID ZDomain::Info::getID() const
{
    MultiID mid;
    pars_.get( sKey::ID(), mid );
    if ( mid.isUdf() )
	pars_.get( IOPar::compKey(sKey(),sKey::ID()), mid );

    if ( mid.isUdf() )
	pars_.get( "ZDomain ID", mid );

    return mid;
}


void ZDomain::Info::setID( const char* id )
{
    pars_.set( sKey::ID(), id );
}


void ZDomain::Info::setID( const MultiID& key )
{
    pars_.set( sKey::ID(), key );
}


bool ZDomain::Info::fillPar( IOPar& par ) const
{
    const Def& inpdef = Def::get( par );
    if ( inpdef == def_ )
    {
	IOParIterator iopiter( pars_ );
	BufferString key, val;
	bool allequal = true;
	while( iopiter.next(key,val) )
	{
	    if ( !par.isPresent(key) )
	    {
		allequal = false;
		break;
	    }

	    const BufferString othval = par.find( key );
	    if ( othval != val )
	    {
		allequal = false;
		break;
	    }
	}

	if ( allequal && par.isPresent(sKey()) )
	    return false;
    }

    def_.set( par );
    par.merge( pars_ );
    return true;
}


bool ZDomain::Info::isCompatibleWith( const Info& inf ) const
{
    if ( &inf.def_ != &def_ )
	return false;

    if ( def_.isDepth() )
    {
	if ( isDepthMeter() != inf.isDepthMeter() ||
	     isDepthFeet() != inf.isDepthFeet() )
	    return false;
    }

    const MultiID myid( getID() );
    const MultiID iopid = inf.getID();
    if ( myid.isUdf() || iopid.isUdf() )
	return true;

    return myid == iopid;
}


bool ZDomain::Info::isCompatibleWith( const IOPar& iop ) const
{
    const Info* info = get( iop );
    return info ? isCompatibleWith( *info ) : false;
}


const char* ZDomain::Info::sKeyNrDec() const
{
    return nrdecstr_.str();
}


Interval<float> ZDomain::Info::getReasonableZRange( bool foruser ) const
{
    Interval<float> validrg;
    if ( isCompatibleWith(::SI().zDomainInfo()) )
	validrg = ::SI().zRange();
    else if ( isDepthFeet() )
    {
	validrg.start_ = -50000.f;
	validrg.stop_ = 50000.f;
    }
    else if ( isDepthMeter() )
    {
	validrg.start_ = -15000.f;
	validrg.stop_ = 15000.f;
    }
    else if ( isTime() )
    {
	validrg.start_ = -10.f; // s
	validrg.stop_ = 30.f;
    }
    else
    {
	//TODO support plugin-based defs
	validrg.setUdf();
    }

    if ( foruser )
	validrg.scale( userFactor() );

    return validrg;
}


ZSampling ZDomain::Info::getReasonableZSampling( bool work, bool foruser ) const
{
    ZSampling zrg = getReasonableZRange( false );
    if ( def_.isSI() )
	zrg = ::SI().zRange( work );
    else if ( isDepthFeet() )
	zrg.step_ = 10;
    else if ( isDepthMeter() )
	zrg.step_ = 4;
    else if ( isTime() )
	zrg.step_ = 0.004;
    else
	zrg.setUdf();

    if ( foruser )
	zrg.scale( userFactor() );

    return zrg;
}


int ZDomain::Info::nrDecimals( float zstepfp, bool usepref ) const
{
    const bool hasstep = !mIsUdf(zstepfp);
    if ( !hasstep )
    {
	const ZSampling zrg = getReasonableZSampling( true );
	zstepfp = mIsUdf(zrg.step_) ? 1.f : zrg.step_;
    }

    if ( !mIsUdf(nrdec_) && usepref )
	return nrdec_;

    if ( usepref && (OD::InNormalRunContext() || OD::InStandAloneRunContext()) )
    {
	int nrdecsett = mUdf(int);
	if ( Settings::common().get(sKeyNrDec(),nrdecsett) &&
	     !mIsUdf(nrdecsett) )
	{
	    getNonConst(*this).nrdec_ = nrdecsett;
	    return nrdec_;
	}
    }

    const double zstep = sCast( double, zstepfp * userFactor() );
    int nrdec = Math::NrSignificantDecimals( zstep );
    if ( !hasstep )
	nrdec++;

    return nrdec;
}


void ZDomain::Info::setPreferredNrDec( int nrdec )
{
    if ( !mIsUdf(nrdec) )
	nrdec_ = nrdec;
}

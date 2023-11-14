/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "zdomain.h"

#include "iopar.h"
#include "keystrs.h"
#include "perthreadrepos.h"
#include "survinfo.h"
#include "uistrings.h"


const char* ZDomain::sKey()		{ return "ZDomain"; }
const char* ZDomain::sKeyTime()		{ return "TWT"; }
const char* ZDomain::sKeyDepth()	{ return "Depth"; }
const char* ZDomain::sKeyUnit()		{ return "ZDomain.Unit"; }

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
	 (domstr == Time().key() || domstr == Depth().key()) )
    {
	if ( isfound ) *isfound = true;
	return domstr == Time().key();
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

} // namespace ZDomain


const ZDomain::Info* ZDomain::get( const IOPar& iop )
{
    bool isfound = false;
    const bool zit = zIsTime( iop, &isfound );
    if ( !isfound )
	return nullptr;

    if ( zit )
	return &TWT();

    BufferString unitstr;
    if ( !iop.get(sKeyUnit(),unitstr) || unitstr.isEmpty() )
	if ( !iop.get(sKey::ZUnit(),unitstr) || unitstr.isEmpty() )
	    return ::SI().depthsInFeet() ? &DepthFeet() : &DepthMeter();

    const Info zinfo( Depth(), unitstr.buf() );
    if ( zinfo.isDepthMeter() )
	return &DepthMeter();
    if ( zinfo.isDepthFeet() )
	return &DepthFeet();

    return ::SI().depthsInFeet() ? &DepthFeet() : &DepthMeter();
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
{ return uiStrings::phrJoinStrings( userName(), uiUnitStr(true) ); }


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
	ret.set( defunit_ );
	if ( withparens )
	    ret.embed('(',')');
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

    defs.add( newdef );
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


ZDomain::Info::Info( const Def& def )
    : Info(def,nullptr)
{
}


ZDomain::Info::Info( const Def& def, const char* unitstr )
    : def_(def)
    , pars_(*new IOPar)
{
    if ( unitstr && *unitstr )
	pars_.set( sKeyUnit(), unitstr );
    else
	setDefaultUnit();
}


ZDomain::Info::Info( const ZDomain::Info& oth )
    : def_(oth.def_)
    , pars_(*new IOPar(oth.pars_))
{
}


ZDomain::Info::Info( const IOPar& iop )
    : def_(ZDomain::Def::get(iop))
    , pars_(*new IOPar(iop))
{
    pars_.removeWithKey( sKey() );
    if ( !pars_.isPresent(sKeyUnit()) )
	setDefaultUnit();
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


bool ZDomain::Info::hasID() const
{
    return !getID().isUdf();
}


const char* ZDomain::Info::unitStr_( bool wp ) const
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


uiString ZDomain::Info::uiUnitStr_( bool wp ) const
{
    const bool abbrevated = true;
    if ( isDepthMeter() )
	return uiStrings::sDistUnitString( false, abbrevated, wp );
    if ( isDepthFeet() )
	return uiStrings::sDistUnitString( true, abbrevated, wp );

    return def_.uiUnitStr( wp );
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
    const ZDomain::Info inf( iop );
    return isCompatibleWith( inf );
}

Interval<float> ZDomain::Info::getReasonableZRange( bool foruser ) const
{
    Interval<float> validrg;
    if ( isDepthFeet() )
    {
	validrg.start = -50000;
	validrg.stop = 50000;
    }
    else if ( isDepthMeter() )
    {
	validrg.start = -15000;
	validrg.stop = 15000;
    }
    else if ( isTime() )
    {
	validrg.start = -10; // s
	validrg.stop = 30;
    }
    else
	validrg.setUdf();

    if ( foruser )
	validrg.scale( userFactor() );

    return validrg;
}

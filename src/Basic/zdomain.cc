/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          18-4-1996
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "zdomain.h"
#include "survinfo.h"
#include "keystrs.h"
#include "iopar.h"
#include "staticstring.h"


const char* ZDomain::sKey()		{ return "ZDomain"; }
const char* ZDomain::sKeyTime()		{ return "TWT"; }
const char* ZDomain::sKeyDepth()	{ return "Depth"; }

static ObjectSet<ZDomain::Def>& DEFS()
{
    static ObjectSet<ZDomain::Def>* defs = 0;
    if ( !defs )
    {
	defs = new ObjectSet<ZDomain::Def>;
	*defs += new ZDomain::Def( ZDomain::sKeyTime(), "Time", "ms", 1000 );
	*defs += new ZDomain::Def( ZDomain::sKeyDepth(), "Depth", "", 1 );
    }

    return *defs;
}


const ZDomain::Def& ZDomain::SI()
{
    return ::SI().zIsTime() ? Time() : Depth();
}


const ZDomain::Def& ZDomain::Time()
{
    return *DEFS()[0];
}


const ZDomain::Def& ZDomain::Depth()
{
    return *DEFS()[1];
}


bool ZDomain::isSI( const IOPar& iop )
{
    const char* domstr = iop.find( sKey() );
    if ( !domstr || !*domstr ) return true;

    const Def& def = Def::get( domstr );
    return def.isSI();
}


bool ZDomain::isDepth( const IOPar& iop )
{
    const char* domstr = iop.find( sKey() );
    return domstr && *domstr ? !strcmp(domstr,sKeyDepth()) : !::SI().zIsTime();
}


bool ZDomain::isTime( const IOPar& iop )
{
    const char* domstr = iop.find( sKey() );
    return !domstr || !*domstr ? ::SI().zIsTime() : !strcmp(domstr,sKeyTime());
}


void ZDomain::setSI( IOPar& iop )
{
    iop.removeWithKey( sKey() );
}


void ZDomain::setDepth( IOPar& iop )
{
    if ( ::SI().zIsTime() )
	iop.set( sKey(), sKeyDepth() );
    else
	setSI( iop );
}


void ZDomain::setTime( IOPar& iop )
{
    if ( !::SI().zIsTime() )
	iop.set( sKey(), sKeyTime() );
    else
	setSI( iop );
}


bool ZDomain::Def::isSI() const
{
    return ::SI().zIsTime() ? isTime() : isDepth();
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


const char* ZDomain::Def::unitStr( bool withparens ) const
{
    if ( withparens )
    {
	static StaticStringManager stm;
	BufferString& ret = stm.getString();
	ret = "";
	BufferString unitstr = unitStr( false );
	if ( !unitstr.isEmpty() )
	    ret.add( "(" ).add( unitstr ).add( ")" );
	return ret.buf();
    }

    if ( !isDepth() )
	return defunit_;

    return getDistUnitString( ::SI().zInFeet(), false );
}


const ZDomain::Def& ZDomain::Def::get( const char* ky )
{
    if ( !ky || !*ky )
	return ZDomain::SI();

    if ( *ky == '`' )
	ky++; // cope with "`TWT"

    const ObjectSet<ZDomain::Def>& defs = DEFS();
    for ( int idx=0; idx<defs.size(); idx++ )
	if ( defs[idx]->key_ == ky )
	    return *defs[idx];

    return ZDomain::SI();
}


const ZDomain::Def& ZDomain::Def::get( const IOPar& iop )
{
    return get( iop.find(sKey()) );
}


bool ZDomain::Def::add( ZDomain::Def* def )
{
    if ( !def ) return false;
    ObjectSet<ZDomain::Def>& defs = DEFS();
    for ( int idx=0; idx<defs.size(); idx++ )
	if ( defs[idx]->key_ == def->key_ )
	    { delete def; return false; }
    defs += def;
    return true;
}


ZDomain::Info::Info( const Def& def )
    : def_(def)
    , pars_(*new IOPar)
{
}


ZDomain::Info::Info( const ZDomain::Info& i )
    : def_(i.def_)
    , pars_(*new IOPar(i.pars_))
{
}


ZDomain::Info::Info( const IOPar& iop )
    : def_(ZDomain::Def::get(iop))
    , pars_(*new IOPar(iop))
{
    const BufferString torem( ZDomain::sKey(), "*" );
    pars_.removeWithKey( torem.buf() );
}


ZDomain::Info::~Info()
{
    delete &pars_;
}


bool ZDomain::Info::hasID() const
{
    const char* res = getID();
    return res && *res;
}


const char* ZDomain::Info::getID() const
{
    const char* res = pars_.find( sKey::ID() );
    if ( !res || !*res )
	res = pars_.find( IOPar::compKey(sKey(),sKey::ID()) );
    if ( !res || !*res )
	res = pars_.find( "ZDomain ID" );
    return res;
}


void ZDomain::Info::setID( const char* id )
{
    pars_.set( sKey::ID(), id );
}


bool ZDomain::Info::isCompatibleWith( const IOPar& iop ) const
{
    ZDomain::Info inf( iop );
    if ( &inf.def_ != &def_ )
	return false;

    BufferString myid( getID() );
    const char* iopid = inf.getID();
    if ( myid.isEmpty() || !iopid )
	return true;

    return myid == iopid;
}

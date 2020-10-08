/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          18-4-1996
________________________________________________________________________

-*/

#include "zdomain.h"

#include "survinfo.h"
#include "keystrs.h"
#include "iopar.h"
#include "staticstring.h"
#include "uistrings.h"


const char* ZDomain::sKey()		{ return "ZDomain"; }
const char* ZDomain::sKeyTime()		{ return sKey::TWT(); }
const char* ZDomain::sKeyDepth()	{ return sKey::Depth(); }

ObjectSet<ZDomain::Def>& DEFS()
{
    mDefineStaticLocalObject( PtrMan<ManagedObjectSet<ZDomain::Def> >,
                              defs, (nullptr) );

    if ( !defs )
    {
        ManagedObjectSet<ZDomain::Def>* newdefs =
				new ManagedObjectSet<ZDomain::Def>;
        *newdefs += new ZDomain::Def( ZDomain::sKeyTime(),
				      uiStrings::sTime(), "ms", 1000 );
	*newdefs += new ZDomain::Def( ZDomain::sKeyDepth(),
				      uiStrings::sDepth(), "", 1 );

        defs.setIfNull( newdefs, true );
    }

    return *defs;
}


const ZDomain::Def& ZDomain::SI()
{
    return ::SI().zDomain().isTime() ? Time() : Depth();
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
    FixedString domstr = iop.find( sKey() );
    return !domstr.isEmpty() ? domstr==sKeyDepth() : !::SI().zDomain().isTime();
}


bool ZDomain::isTime( const IOPar& iop )
{
    FixedString domstr = iop.find( sKey() );
    return domstr.isEmpty() ? ::SI().zDomain().isTime() : domstr==sKeyTime();
}


void ZDomain::setSI( IOPar& iop )
{
    iop.removeWithKey( sKey() );
}


void ZDomain::setDepth( IOPar& iop )
{
    if ( ::SI().zDomain().isTime() )
	iop.set( sKey(), sKeyDepth() );
    else
	setSI( iop );
}


void ZDomain::setTime( IOPar& iop )
{
    if ( !::SI().zDomain().isTime() )
	iop.set( sKey(), sKeyTime() );
    else
	setSI( iop );
}


ZDomain::Def::GenID ZDomain::Def::genID() const
{
    GenID id = 0;
    const char* ptr = key_;
    while ( *ptr )
	id += *ptr++;
    return id;
}


bool ZDomain::Def::isSI() const
{
    return ::SI().zDomain().isTime() ? isTime() : isDepth();
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
{ return userName().withUnit( unitStr() ); }


uiString ZDomain::Def::getRange() const
{ return tr("%1 Range").arg( userName() ); }


const char* ZDomain::Def::fileUnitStr( bool withparens ) const
{
    if ( withparens )
    {
	mDeclStaticString( ret );
	ret.setEmpty();
	BufferString unitstr = fileUnitStr( false );
	if ( !unitstr.isEmpty() )
	    ret.add( "(" ).add( unitstr ).add( ")" );
	return ret.buf();
    }

    if ( !isDepth() )
	return defunit_;

    return getDistUnitString( ::SI().depthsInFeet(), false );
}


uiString ZDomain::Def::unitStr() const
{
    if ( !isDepth() )
	return toUiString(defunit_);

    return uiStrings::sDistUnitString( ::SI().depthsInFeet(), false );
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


const ZDomain::Def& ZDomain::Def::get( GenID genid )
{
    const ObjectSet<ZDomain::Def>& defs = DEFS();
    for ( int idx=0; idx<defs.size(); idx++ )
	if ( defs[idx]->genID() == genid )
	    return *defs[idx];
    return ZDomain::SI();
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
    pars_.removeWithKeyPattern( torem.buf() );
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


bool ZDomain::Info::isCompatibleWith( const ZDomain::Info& oth ) const
{
    if ( &def_ != &oth.def_ )
	return false;

    BufferString myid( getID() );
    const char* iopid = oth.getID();
    if ( myid.isEmpty() || !iopid )
	return true;

    return myid == iopid;
}


bool ZDomain::Info::isCompatibleWith( const IOPar& iop ) const
{
    ZDomain::Info othinf( iop );
    return isCompatibleWith( othinf );
}

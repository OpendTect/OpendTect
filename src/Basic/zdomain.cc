/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "zdomain.h"

#include "survinfo.h"
#include "keystrs.h"
#include "iopar.h"
#include "perthreadrepos.h"
#include "uistrings.h"


const char* ZDomain::sKey()		{ return "ZDomain"; }
const char* ZDomain::sKeyTime()		{ return "TWT"; }
const char* ZDomain::sKeyDepth()	{ return "Depth"; }

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
    const BufferString domstr = iop.find( sKey() );
    if ( domstr.isEmpty() )
	return true;

    const Def& def = Def::get( domstr );
    return def.isSI();
}


bool ZDomain::isDepth( const IOPar& iop )
{
    const BufferString domstr = iop.find( sKey() );
    return !domstr.isEmpty() ? domstr.isEqual( sKeyDepth() ) :
							    !::SI().zIsTime();
}


bool ZDomain::isTime( const IOPar& iop )
{
    const BufferString domstr = iop.find( sKey() );
    return domstr.isEmpty() ? ::SI().zIsTime() : domstr.isEqual( sKeyTime() );
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


uiString ZDomain::Def::getLabel() const
{ return uiStrings::phrJoinStrings( userName(), uiUnitStr(true) ); }


uiString ZDomain::Def::getRange() const
{
    return uiStrings::phrJoinStrings( userName(),
				uiStrings::sRange().toLower() );
}


const char* ZDomain::Def::unitStr( bool withparens ) const
{
    if ( withparens )
    {
	mDeclStaticString( ret );
	ret.setEmpty();
	BufferString unitstr = unitStr( false );
	if ( !unitstr.isEmpty() )
	    ret.add( "(" ).add( unitstr ).add( ")" );
	return ret.buf();
    }

    if ( !isDepth() )
	return defunit_;

    return getDistUnitString( ::SI().depthsInFeet(), false );
}


uiString ZDomain::Def::uiUnitStr( bool withparens ) const
{
    if ( withparens )
    {
	mDeclStaticString( ret );
	ret.setEmpty();
	BufferString unitstr = unitStr( false );
	if ( !unitstr.isEmpty() )
	    ret.add( "(" ).add( unitstr ).add( ")" );
	return toUiString(ret);
    }

    if ( !isDepth() )
	return toUiString(defunit_);

    return uiStrings::sDistUnitString( ::SI().depthsInFeet(), false, false );
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
    pars_.removeSubSelection( ZDomain::sKey() );
}


ZDomain::Info::~Info()
{
    delete &pars_;
}


bool ZDomain::Info::hasID() const
{
    return !getID().isUdf();
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


bool ZDomain::Info::isCompatibleWith( const IOPar& iop ) const
{
    ZDomain::Info inf( iop );
    if ( &inf.def_ != &def_ )
	return false;

    MultiID myid( getID() );
    const MultiID iopid = inf.getID();
    if ( myid.isUdf() || iopid.isUdf() )
	return true;

    return myid == iopid;
}

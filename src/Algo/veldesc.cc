/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "veldesc.h"

#include "iopar.h"
#include "separstr.h"
#include "stringview.h"
#include "staticsdesc.h"
#include "survinfo.h"
#include "uistrings.h"

const char* sKeyIsVelocity = "Is Velocity";
const char* VelocityDesc::sKeyIsVelocity()	{ return ::sKeyIsVelocity; }
const char* VelocityDesc::sKeyVelocityVolume()	{ return "Velocity volume"; }
const char* VelocityDesc::sKeyVelocityUnit()	{ return "Velocity Unit"; }
const char* VelocityDesc::sKeyVelocityType()
{ return OD::VelocityTypeDef().name().buf(); }


mDefineEnumUtils(VelocityDesc,Type,"Velocity Types")
{ "Unknown", "Vint", "Vrms", "Vavg", "Delta", "Epsilon", "Eta", nullptr };

VelocityDesc::VelocityDesc()
    : type_(Unknown)
{
}


VelocityDesc::VelocityDesc( Type typ )
    : type_(typ)
{
}


VelocityDesc::VelocityDesc( OD::VelocityType typ, const char* unitstr )
    : VelocityDesc( get(typ) )
{
    if ( unitstr )
	velunit_.set( unitstr );
}


bool VelocityDesc::operator==( const VelocityDesc& oth ) const
{
    return type_==oth.type_ && velunit_ == oth.velunit_ &&
	   statics_==oth.statics_;
}


bool VelocityDesc::operator!=( const VelocityDesc& oth ) const
{
    return !(*this==oth);
}


OD::VelocityType VelocityDesc::getType() const
{
    return get( type_ );
}


void VelocityDesc::setType( OD::VelocityType type )
{
    type_ = get( type );
}


bool VelocityDesc::isUdf( Type type )
{
    return type == Unknown;
}


bool VelocityDesc::isUdf( OD::VelocityType type )
{
    return type == OD::VelocityType::Unknown;
}


bool VelocityDesc::isUdf() const
{ return isUdf(type_); }


bool VelocityDesc::isVelocity( Type type )
{ return type==Interval || type==RMS || type==Avg; }


bool VelocityDesc::isVelocity( OD::VelocityType type )
{ return type==OD::VelocityType::Interval || type==OD::VelocityType::RMS ||
	 type==OD::VelocityType::Avg; }


bool VelocityDesc::isVelocity() const
{ return isVelocity(type_); }


bool VelocityDesc::isInterval() const
{
    return type_ == Interval;
}


bool VelocityDesc::isRMS() const
{
    return type_ == RMS;
}


bool VelocityDesc::isAvg() const
{
    return type_ == Avg;
}


bool VelocityDesc::isThomsen( Type type )
{ return type==Delta || type==Epsilon || type==Eta; }


bool VelocityDesc::isThomsen( OD::VelocityType type )
{ return type==OD::VelocityType::Delta || type==OD::VelocityType::Epsilon ||
	 type==OD::VelocityType::Eta; }


bool VelocityDesc::isThomsen() const
{ return isThomsen(type_); }


void VelocityDesc::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), sKey::Velocity() );
    par.set( sKeyVelocityType(), toString(type_) );
    if ( isVelocity() && !velunit_.isEmpty() )
	par.set( sKeyVelocityUnit(), velunit_ );

    if ( type_ == RMS )
	statics_.fillPar( par );
    else
	StaticsDesc::removePars( par );

    // Legacy, keep it for backward readability of the database:
    par.setYN( ::sKeyIsVelocity, true );
}


bool VelocityDesc::usePar( const IOPar& par )
{
    BufferString typstr;
    if ( !par.get(sKey::Type(),typstr) && typstr != sKey::Velocity() )
    {
	bool isvel;
	if ( !par.getYN(::sKeyIsVelocity,isvel) || !isvel )
	    return false;
    }

    Type type = type_;
    if ( parseEnum(par,sKeyVelocityType(),type) )
    {
	type_ = type;
	if ( type_ == RMS && !statics_.usePar(par) )
	    return false;

	par.get( sKeyVelocityUnit(), velunit_ );
    }
    else
    {  // legacy format
	BufferString arr;
	if ( !par.get("Velocity Desc",arr) )
	    return false;

	const FileMultiString fms( arr );
	if ( fms.isEmpty() )
	    return false;

	if ( !parseEnum(par,fms[0],type) )
	    return false;

	type_ = type;
	par.get( sKeyVelocityUnit(), velunit_ );
	statics_.velattrib_.setEmpty();
	statics_.vel_ = mUdf(double);
	return true;
    }

    return true;
}


void VelocityDesc::removePars( IOPar& par )
{
    par.removeWithKey( sKey::Type() );
    par.removeWithKey( sKeyVelocityType() );
    par.removeWithKey( sKeyVelocityUnit() );
    StaticsDesc::removePars( par );
    par.removeWithKey( ::sKeyIsVelocity );
}


uiString VelocityDesc::getVelUnit( bool wp )
{
    return ::toUiString( wp ? "(%1/%2)" : "%1/%2")
	.arg( uiStrings::sDistUnitString( SI().depthsInFeet(), true, false ))
	.arg( uiStrings::sTimeUnitString(true));
}


uiString VelocityDesc::getVelVolumeLabel()
{
    return tr( "Velocity model");
}


bool VelocityDesc::isUsable( Type type, const ZDomain::Def& zddef,
			     uiRetVal& uirv )
{
    if ( type == Interval || type == Avg )
	return uirv.isOK();

    if ( zddef.isTime() )
    {
	if ( type != RMS )
	    uirv.add( tr("Only RMS, Avg and Interval allowed for time based "
			 "models") );
    }
    else if ( zddef.isDepth() )
	uirv.add( tr("Only Avg and Interval allowed for depth based models") );
    else
	uirv.add( tr("Velocity not usable in the '%1' domain")
			.arg(zddef.userName()) );

    return uirv.isOK();
 }


 bool VelocityDesc::isUsable( OD::VelocityType typ, const ZDomain::Def& zddef,
			      uiRetVal& uirv )
 {
     return isUsable( get(typ), zddef, uirv );
 }


VelocityDesc::Type VelocityDesc::get( OD::VelocityType typ )
{
    if ( typ == OD::VelocityType::Interval )
	return Interval;
    if ( typ == OD::VelocityType::RMS )
	return RMS;
    if ( typ == OD::VelocityType::Avg )
	return Avg;
    if ( typ == OD::VelocityType::Delta )
	return Delta;
    if ( typ == OD::VelocityType::Epsilon )
	return Epsilon;
    if ( typ == OD::VelocityType::Eta )
	return Eta;

    return Unknown;
}


OD::VelocityType VelocityDesc::get( Type typ )
{
    if ( typ == Interval )
	return OD::VelocityType::Interval;
    if ( typ == RMS )
	return OD::VelocityType::RMS;
    if ( typ == Avg )
	return OD::VelocityType::Avg;
    if ( typ == Delta )
	return OD::VelocityType::Delta;
    if ( typ == Epsilon )
	return OD::VelocityType::Epsilon;
    if ( typ == Eta )
	return OD::VelocityType::Eta;

    return OD::VelocityType::Unknown;
}

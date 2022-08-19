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

const char* VelocityDesc::sKeyVelocityType()	{ return "Velocity Type"; }
const char* VelocityDesc::sKeyIsVelocity()	{ return "Is Velocity"; }
const char* VelocityDesc::sKeyVelocityVolume()	{ return "Velocity volume"; }
const char* VelocityDesc::sKeyVelocityUnit()	{ return "Velocity Unit"; }

mDefineEnumUtils(VelocityDesc,Type,"Velocity Types")
{ "Unknown", "Vint", "Vrms", "Vavg", "Delta", "Epsilon", "Eta", 0 };

VelocityDesc::VelocityDesc()
    : type_( Unknown )
{}


VelocityDesc::VelocityDesc( Type t )
    : type_( t )
{}


bool VelocityDesc::operator==( const VelocityDesc& b ) const
{
    return type_==b.type_ && statics_==b.statics_;
}


bool VelocityDesc::operator!=( const VelocityDesc& b ) const
{
    return !(*this==b);
}


bool VelocityDesc::isVelocity( VelocityDesc::Type type )
{ return type==Interval || type==RMS || type==Avg; }



bool VelocityDesc::isVelocity() const
{ return isVelocity(type_); }



bool VelocityDesc::isThomsen( VelocityDesc::Type type )
{ return type==Delta || type==Epsilon || type==Eta; }



bool VelocityDesc::isThomsen() const
{ return isThomsen(type_); }



void VelocityDesc::fillPar( IOPar& par ) const
{
    par.set( sKeyVelocityType(), TypeNames()[(int)type_] );
    if ( type_==RMS )
	statics_.fillPar( par );
    else
	StaticsDesc::removePars( par );

    par.setYN( sKeyIsVelocity(), true );
}


void VelocityDesc::removePars( IOPar& par )
{
    par.removeWithKey( sKeyVelocityType() );
    par.removeWithKey( sKeyIsVelocity() );
    StaticsDesc::removePars( par );
}


bool VelocityDesc::usePar( const IOPar& par )
{
    bool isvel;
    if ( !par.getYN( sKeyIsVelocity(), isvel ) || !isvel )
	return false;

    const StringView typestr = par.find( sKeyVelocityType() );
    if ( typestr.isEmpty() )
    {
	BufferString arr;
	if ( !par.get( "Velocity Desc", arr ) )
	    return false;

	SeparString sepstr( arr, '`' );
	if ( sepstr.size()<1 )
	    return false;

	if ( !parseEnumType( sepstr[0], type_ ) )
	    return false;

	statics_.velattrib_.setEmpty();
	statics_.vel_ = mUdf(float);

	return true;
    }

    par.get( sKeyVelocityUnit(), velunit_ );
    if ( !parseEnumType( typestr, type_ ) )
	return false;

    if ( type_==RMS && !statics_.usePar( par ) )
	return false;

    return true;
}


uiString VelocityDesc::getVelUnit( bool wp )
{
    return ::toUiString( wp ? "(%1/%2)" : "%1/%2")
	.arg( uiStrings::sDistUnitString( SI().depthsInFeet(), true, false ))
	.arg( uiStrings::sTimeUnitString(true));
}


uiString VelocityDesc::getVelVolumeLabel()
{
    return tr( "Velocity model %1").arg( getVelUnit(true) );
}

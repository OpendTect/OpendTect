/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Sep 2007
________________________________________________________________________

-*/


#include "veldesc.h"

#include "iopar.h"
#include "separstr.h"
#include "fixedstring.h"
#include "staticsdesc.h"
#include "survinfo.h"
#include "uistrings.h"

const char* VelocityDesc::sKeyVelocityType()	{ return "Velocity Type"; }
const char* VelocityDesc::sKeyIsVelocity()	{ return "Is Velocity"; }
const char* VelocityDesc::sKeyVelocityVolume()	{ return "Velocity volume"; }

mDefineEnumUtils(VelocityDesc,Type,"Velocity Types")
{ "Unknown", "Vint", "Vrms", "Vavg", "Delta", "Epsilon", "Eta", 0 };

template<>
void EnumDefImpl<VelocityDesc::Type>::init()
{
    uistrings_ += uiStrings::sUnknown();
    uistrings_ += mEnumTr("Interval Velocity",0);
    uistrings_ += mEnumTr("RMS Velocity",0);
    uistrings_ += mEnumTr("Average Velocity",0);
    uistrings_ += mEnumTr("Delta","Velocity Type");
    uistrings_ += mEnumTr("Epsilon","Velocity Type");
    uistrings_ += mEnumTr("Eta","Velocity Type");
}

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
    par.set( sKeyVelocityType(), TypeDef().getKey(type_) );
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

    const FixedString typestr = par.find( sKeyVelocityType() );
    if ( typestr.isEmpty() )
    {
	BufferString arr;
	if ( !par.get( "Velocity Desc", arr ) )
	    return false;

	SeparString sepstr( arr, '`' );
	if ( sepstr.size()<1 )
	    return false;

	if ( !TypeDef().parse( sepstr[0], type_ ) )
	    return false;

	statics_.velattrib_.setEmpty();
	statics_.vel_ = mUdf(float);

	return true;
    }

    if ( !TypeDef().parse( typestr, type_ ) )
	return false;

    if ( type_==RMS && !statics_.usePar( par ) )
	return false;

    return true;
}


uiString VelocityDesc::getVelUnit( bool wp )
{
    return ::toUiString( wp ? "(%1/%2)" : "%1/%2")
	.arg( uiStrings::sDistUnitString( SI().depthsInFeet() ) )
	.arg( uiStrings::sTimeUnitString() );
}


uiString VelocityDesc::getVelVolumeLabel()
{
    return tr("Velocity model %1").arg( getVelUnit(true) );
}

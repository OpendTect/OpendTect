/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Sep 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


#include "veldesc.h"

#include "iopar.h"
#include "separstr.h"
#include "fixedstring.h"
#include "staticsdesc.h"
#include "survinfo.h"

const char* VelocityDesc::sKeyVelocityType()	{ return "Velocity Type"; }
const char* VelocityDesc::sKeyIsVelocity()	{ return "Is Velocity"; }

DefineEnumNames(VelocityDesc,Type,0,"Velocity Types")
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
    par.remove( sKeyVelocityType() );
    par.remove( sKeyIsVelocity() );
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

	if ( !parseEnumType( sepstr[0], type_ ) )
	    return false;

	statics_.velattrib_.setEmpty();
	statics_.vel_ = mUdf(float);

	return true;
    }

    if ( !parseEnumType( typestr, type_ ) )
	return false;

    if ( type_==RMS && !statics_.usePar( par ) )
	return false;

    return true;
}


const char* VelocityDesc::getVelUnit( bool wp )
{
    if ( SI().depthsInFeetByDefault() )
	return wp ? "(ft/s)" : "ft/s";

    return wp ? "(m/s)" : "m/s";
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Sep 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: veldesc.cc,v 1.13 2009-07-22 16:01:31 cvsbert Exp $";


#include "veldesc.h"

#include "iopar.h"
#include "separstr.h"
#include "fixedstring.h"
#include "staticsdesc.h"

const char* VelocityDesc::sKeyVelocityType()	{ return "Velocity Type"; }
const char* VelocityDesc::sKeyIsVelocity()	{ return "Is Velocity"; }

DefineEnumNames(VelocityDesc,Type,0,"Velocity Types")
{ "Unknown", "Vint", "Vrms", "Vavg", 0 };

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

	const int idx = TypeDef().convert( sepstr[0] );
	if ( idx<0 )
	    return false;

	type_ = (Type) idx;

	statics_.velattrib_.setEmpty();
	statics_.vel_ = mUdf(float);

	return true;
    }

    const int type = TypeDef().convert( typestr );
    if ( type==-1 ) return false;

    type_ = (Type) type;

    if ( type_==RMS && !statics_.usePar( par ) )
	return false;

    return true;
}

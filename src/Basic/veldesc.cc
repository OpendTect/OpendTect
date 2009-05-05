/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Sep 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: veldesc.cc,v 1.10 2009-05-05 18:31:24 cvskris Exp $";


#include "veldesc.h"

#include "iopar.h"
#include "separstr.h"
#include "fixedstring.h"

const char* VelocityDesc::sKeyVelocityType()	{ return "Velocity Type"; }
const char* VelocityDesc::sKeyStaticsHorizon()	{ return "Statics Horizon"; }
const char* VelocityDesc::sKeyStaticsVelocity()	{ return "Statics Velocity"; }
const char* VelocityDesc::sKeyStaticsVelocityAttrib()	
{ return "Statics Velocity Attrib"; }


const char* VelocityDesc::sKeyIsVelocity()	{ return "Is Velocity"; }

DefineEnumNames(VelocityDesc,Type,0,"Velocity Types")
{ "Unknown", "Vint", "Vrms", "Vavg", 0 };

VelocityDesc::VelocityDesc()
    : type_( Unknown )
    , staticsvel_( mUdf(float) )
{}


VelocityDesc::VelocityDesc( Type t )
    : type_( t )
    , staticsvel_( mUdf(float) )
{}


void VelocityDesc::fillPar( IOPar& par ) const
{
    par.set( sKeyVelocityType(), TypeNames()[(int)type_] );
    if ( type_==RMS )
    {
	par.set( sKeyStaticsHorizon(), staticshorizon_ );
	par.set( sKeyStaticsVelocity(), staticsvel_ );
	par.set( sKeyStaticsVelocityAttrib(), staticsvelattrib_ );
    }

    par.setYN( sKeyIsVelocity(), true );
}


void VelocityDesc::removePars( IOPar& par )
{
    par.remove( sKeyVelocityType() );
    par.remove( sKeyStaticsHorizon() );
    par.remove( sKeyStaticsVelocity() );
    par.remove( sKeyStaticsVelocityAttrib() );
    par.remove( sKeyIsVelocity() );
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

	staticsvelattrib_.setEmpty();
	staticsvel_ = mUdf(float);

	return true;
    }

    const int type = TypeDef().convert( typestr );
    if ( type==-1 ) return false;

    type_ = (Type) type;

    if ( type_==RMS )
    {
	if  ( !par.get( sKeyStaticsHorizon(), staticshorizon_ ) )
	    return false;

	par.get( sKeyStaticsVelocityAttrib(), staticsvelattrib_ );
	par.get( sKeyStaticsVelocity(), staticsvel_ );
    }

    return true;
}

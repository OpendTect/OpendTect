/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Sep 2007
 RCS:           $Id: veldesc.cc,v 1.2 2007-11-15 18:43:59 cvskris Exp $
________________________________________________________________________

-*/


#include "veldesc.h"

#include "iopar.h"
#include "separstr.h"

const char* VelocityDesc::sKeyVelocityDesc()	{ return "Velocity Desc"; }

DefineEnumNames(VelocityDesc,Type,0,"Velocity Types")
{ "Unknown", "Vrms", "Vint", "Vavg", 0 };

DefineEnumNames(VelocityDesc,SampleRange,0,"Sample Ranges")
{ "Centered", "Above", "Below", 0 };

VelocityDesc::VelocityDesc()
    : type_( Unknown )
    , samplerange_( Centered )
{}


VelocityDesc::VelocityDesc( Type t, SampleRange sr )
    : type_( t )
    , samplerange_( sr )
{}



BufferString VelocityDesc::toString() const
{
    SeparString res ( 0, '`' );
    res.add( TypeNames[(int)type_] );
    res.add( SampleRangeNames[(int)samplerange_] );

    return BufferString( res );
}


bool VelocityDesc::fromString( const char* str )
{
    SeparString sepstr( str, '`' );
    if ( sepstr.size()!=2 )
	return false;

    const int type = TypeDefinition.convert( sepstr[0] );
    if ( type==-1 ) return false;

    const int samplerange = SampleRangeDefinition.convert( sepstr[1] );
    if ( samplerange==-1 ) return false;

    type_ = (Type) type;
    samplerange_ = (SampleRange) samplerange;

    return true;
}


void VelocityDesc::fillPar( IOPar& par ) const
{
    BufferString str = toString();
    par.set( sKeyVelocityDesc(), str.buf() );
}


bool VelocityDesc::usePar( const IOPar& par )
{
    BufferString str;
    return par.get( sKeyVelocityDesc(), str ) && fromString( str.buf() );
}

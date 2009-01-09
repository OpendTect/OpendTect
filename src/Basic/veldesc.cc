/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Sep 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: veldesc.cc,v 1.7 2009-01-09 04:35:56 cvsnanne Exp $";


#include "veldesc.h"

#include "iopar.h"
#include "separstr.h"

const char* VelocityDesc::sKeyVelocityDesc()	{ return "Velocity Desc"; }
const char* VelocityDesc::sKeyIsVelocity()	{ return "Is Velocity"; }

DefineEnumNames(VelocityDesc,Type,0,"Velocity Types")
{ "Unknown", "Vint", "Vrms", "Vavg", 0 };

DefineEnumNames(VelocityDesc,SampleSpan,0,"Sample Spans")
{ "Centered", "Above", "Below", 0 };

VelocityDesc::VelocityDesc()
    : type_( Unknown )
    , samplespan_( Centered )
{}


VelocityDesc::VelocityDesc( Type t, SampleSpan sr )
    : type_( t )
    , samplespan_( sr )
{}



BufferString VelocityDesc::toString() const
{
    SeparString res ( 0, '`' );
    res.add( TypeNames()[(int)type_] );
    res.add( SampleSpanNames()[(int)samplespan_] );

    return BufferString( res );
}


bool VelocityDesc::fromString( const char* str )
{
    SeparString sepstr( str, '`' );
    if ( sepstr.size()!=2 )
	return false;

    const int type = TypeDef().convert( sepstr[0] );
    if ( type==-1 ) return false;

    const int samplespan = SampleSpanDef().convert( sepstr[1] );
    if ( samplespan==-1 ) return false;

    type_ = (Type) type;
    samplespan_ = (SampleSpan) samplespan;

    return true;
}


void VelocityDesc::fillPar( IOPar& par ) const
{
    BufferString str = toString();
    par.set( sKeyVelocityDesc(), str.buf() );
    par.setYN( sKeyIsVelocity(), true );
}


bool VelocityDesc::usePar( const IOPar& par )
{
    bool isvel;
    if ( !par.getYN( sKeyIsVelocity(), isvel ) || !isvel )
	return false;

    BufferString str;
    return par.get( sKeyVelocityDesc(), str ) && fromString( str.buf() );
}

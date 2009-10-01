/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 18-4-1996
-*/

static const char* rcsID = "$Id: draw.cc,v 1.74 2009-10-01 07:33:12 cvsjaap Exp $";

/*! \brief Several implementations for UI-related things.

The main chunk is color table related.
*/

#include "draw.h"
#include "separstr.h"
#include "iopar.h"


DefineEnumNames(Alignment,HPos,1,"Alignment")
{ "Left", "Right", "Center", 0 };
DefineEnumNames(Alignment,VPos,1,"Alignment")
{ "Top", "Bottom", "Center", 0 };
DefineEnumNames(MarkerStyle2D,Type,2,"Marker type")
{ "None", "Square", "Circle", "Cross", 0 };
DefineEnumNames(MarkerStyle3D,Type,0,"Marker type")
{ "None", "Cube", "Cone", "Cylinder", "Sphere", "Arrow", "Cross", "Point", 0 };
DefineEnumNames(LineStyle,Type,0,"Line style")
{ "None", "Solid", "Dashed", "Dotted", "Dash-Dotted", "Dash-Dot-Dotted",0 };


Alignment::Pos Alignment::pos( bool hor ) const
{
    if ( hor )
	return hor_ == Left ? Start : (hor_ == Right ? Stop : Center);
    return ver_ == Top ? Start : (ver_ == Bottom ? Stop : Center);
}


void Alignment::set( Alignment::Pos h, Alignment::Pos v )
{
    hor_ = h == Start ? Left : (h == Stop ? Right : HCenter);
    ver_ = v == Start ? Top : (v == Stop ? Bottom : VCenter);
}


int Alignment::uiValue() const
{
    int ret = hor_ == Left ? 0x0001 : (hor_ == Right ? 0x0002 : 0x0004);
    ret |= ver_ == Top ? 0x0020 : (ver_ == Bottom ? 0x0040 : 0x0080);
    return ret;
}


void Alignment::setUiValue( int v )
{
    hor_ = v&0x0001 ? Left : (v&0x0002 ? Right : HCenter);
    ver_ = v&0x0020 ? Top : (v&0x0040 ? Bottom : VCenter);
}


#define mToStringImpl( clss, par ) \
void clss::toString( BufferString& bs ) const \
{ \
    FileMultiString fms; \
    fms = eString(Type,type_); \
    fms += par; \
    color_.fill( bs.buf() ); \
    fms += FileMultiString(bs); \
    bs = fms; \
}


#define mFromStringImpl( clss, par ) \
void clss::fromString( const char* s ) \
{ \
    FileMultiString fms( s ); \
    type_ = eEnum(Type,fms[0]); \
    par = atoi(fms[1]); \
    FileMultiString colfms( fms.from(2) ); \
    color_.use( colfms ); \
}


mToStringImpl( MarkerStyle2D, size_ )
mToStringImpl( MarkerStyle3D, size_ )
mToStringImpl( LineStyle, width_ )

mFromStringImpl( MarkerStyle2D, size_ )
mFromStringImpl( MarkerStyle3D, size_ )
mFromStringImpl( LineStyle, width_ )

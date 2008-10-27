/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-4-1996
-*/

static const char* rcsID = "$Id: draw.cc,v 1.70 2008-10-27 11:07:28 cvssatyaki Exp $";

/*! \brief Several implementations for UI-related things.

The main chunk is color table related.
*/

#include "draw.h"
#include "separstr.h"
#include "iopar.h"

// First some implementations for a couple of header files ...

DefineEnumNames(MarkerStyle2D,Type,2,"Marker type")
    { "None", "Square", "Circle", "Cross", 0 };
DefineEnumNames(MarkerStyle3D,Type,0,"Marker type")
    { "None", "Cube", "Cone", "Cylinder", "Sphere", "Arrow", "Cross", 0 };
DefineEnumNames(LineStyle,Type,0,"Line style")
    { "None", "Solid", "Dashed", "Dotted", "Dash-Dotted", "Dash-Dot-Dotted",0 };


// Then some draw.h stuff

#define mToStringImpl( clss, par ) \
void clss::toString( BufferString& bs ) const \
{ \
    FileMultiString fms; \
    fms = eString(Type,type_); \
    fms += par; \
    color_.fill( bs.buf() ); \
    fms += bs; \
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

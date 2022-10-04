/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigeom.h"


// uiSize
uiSize::uiSize( const Geom::Size2D<int>& a )
    :  Geom::Size2D<int>( a )
{}


uiSize::uiSize( int wdt, int hgt )
    : Geom::Size2D<int>(wdt,hgt)
{}


uiSize::~uiSize()
{}



// uiRect
uiRect::~uiRect()
{}



// uiBorder
uiBorder::uiBorder( int i )
    : lt_(i,i)
    , rb_(i,i)
{}


uiBorder::uiBorder( int l, int t, int r, int b )
    : lt_(l,t)
    , rb_(r,b)
{}


uiBorder::~uiBorder()
{}

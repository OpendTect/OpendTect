#ifndef draw_h
#define draw_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          26/07/2000
 RCS:           $Id: draw.h,v 1.2 2000-07-27 09:54:03 bert Exp $
________________________________________________________________________

-*/

#include "enums.h"
#include "color.h"
#include "fontdata.h"

class Draw
{
public:

    enum MarkerType	{ NoMarker, Square, Circle, Cross };
			DeclareEnumUtils(MarkerType)
    enum LineStyle	{ NoLine, Solid, Dash, Dot, DashDot, DashDotDot };
			DeclareEnumUtils(LineStyle)
};


class MarkerAppearance
{
public:

			MarkerAppearance( Draw::MarkerType t=Draw::Square,
					  Color c=Color(0,0,0,0) )
			: type(t), color(c)	{}

    Draw::MarkerType	type;
    Color		color;
    FontData		fontdata;

};


class LineAppearance
{
public:

			LineAppearance( Draw::LineStyle s=Draw::Solid,
					int w=1, Color c=Color(0,0,0,0) )
			: style(s), width(w), color(c)	{}

    Draw::LineStyle	style;
    int			width;
    Color		color;

};


#endif

#ifndef draw_h
#define draw_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          26/07/2000
 RCS:           $Id: draw.h,v 1.1 2000-07-26 15:35:40 bert Exp $
________________________________________________________________________

-*/

#include "enums.h"

class Draw
{
public:

    enum MarkerType	{ NoMarker, Circle, Square, Cross };
			DeclareEnumUtils(MarkerType)
    enum LineStyle	{ NoLine, Solid, Dash, Dot, DashDot, DashDotDot };
			DeclareEnumUtils(LineStyle)
};


#endif

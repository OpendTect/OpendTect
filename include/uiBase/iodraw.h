#ifndef iodraw_H
#define iodraw_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: iodraw.h,v 1.6 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include <uigeom.h>

class ioDrawTool;

/*! \brief anything that can be drawn upon....

    can give you a drawtool to do your job.

*/
class ioDrawArea
{
public:
			ioDrawArea()			{}
    virtual		~ioDrawArea()			{}

    inline ioDrawTool*	drawTool( uiPoint origin ) 
			    { return drawTool_( origin.x(), origin.y() ); }

    inline ioDrawTool*	drawTool( int x0=0, int y0=0 )	
			    { return drawTool_( x0, y0 ); }

protected:

    virtual ioDrawTool*	drawTool_( int x0, int y0 )		=0;

};

#endif

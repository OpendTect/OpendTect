#ifndef iodraw_H
#define iodraw_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: iodraw.h,v 1.5 2001-08-23 14:59:17 windev Exp $
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

#ifndef iodraw_h
#define iodraw_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: iodraw.h,v 1.10 2007-03-28 12:20:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigeom.h"

class ioDrawTool;

/*! \brief anything that can be drawn upon....

    can give you a drawtool to do your job.

*/
class ioDrawArea
{
public:
			ioDrawArea()			{}
    virtual		~ioDrawArea()			{}

    virtual void	update()			{}
    virtual ioDrawTool&	drawTool()			= 0;

    inline const ioDrawTool& drawTool() const
			{ return const_cast<ioDrawArea*>(this)->drawTool(); }

};

#endif

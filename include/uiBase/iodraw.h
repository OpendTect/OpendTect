#ifndef iodraw_h
#define iodraw_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: iodraw.h,v 1.13 2012-08-03 13:00:50 cvskris Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigeom.h"

class ioDrawTool;

/*! \brief anything that can be drawn upon....

    can give you a drawtool to do your job.

*/
mClass(uiBase) ioDrawArea
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


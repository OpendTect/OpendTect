#ifndef iodraw_h
#define iodraw_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uigeom.h"

class ioDrawTool;

/*! \brief anything that can be drawn upon....

    can give you a drawtool to do your job.

*/
mClass ioDrawArea
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

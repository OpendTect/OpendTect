#ifndef iodrawimpl_H
#define iodrawimpl_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          04/07/2001
 RCS:           $Id: iodrawimpl.h,v 1.2 2003-05-15 14:17:23 nanne Exp $
________________________________________________________________________

-*/

#include "iodraw.h"

class ioDrawTool;
class QPaintDevice;

class ioDrawAreaImpl : public ioDrawArea
{
public:
			ioDrawAreaImpl() : mDrawTool(0)	{}
    virtual		~ioDrawAreaImpl();

    virtual ioDrawTool*	drawTool_( int x0, int y0 );

protected:

    virtual QPaintDevice* mQPaintDevice()		=0;

    void		releaseDrawTool(); 

private:

    ioDrawTool* 	mDrawTool;

};

#endif

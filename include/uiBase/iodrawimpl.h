#ifndef iodrawimpl_H
#define iodrawimpl_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          04/07/2001
 RCS:           $Id: iodrawimpl.h,v 1.3 2003-11-07 12:21:54 bert Exp $
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

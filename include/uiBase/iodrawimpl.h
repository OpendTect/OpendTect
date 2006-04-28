#ifndef iodrawimpl_h
#define iodrawimpl_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          04/07/2001
 RCS:           $Id: iodrawimpl.h,v 1.5 2006-04-28 15:23:20 cvsnanne Exp $
________________________________________________________________________

-*/

#include "iodraw.h"

class ioDrawTool;
class QPaintDevice;

class ioDrawAreaImpl : public ioDrawArea
{
public:
			ioDrawAreaImpl() : drawtool(0)	{}
    virtual		~ioDrawAreaImpl();

    virtual ioDrawTool*	drawTool_( int x0, int y0 );

protected:

    virtual QPaintDevice* mQPaintDevice()		=0;

    void		releaseDrawTool(); 

private:

    ioDrawTool* 	drawtool;

};

#endif

#ifndef iodrawimpl_H
#define iodrawimpl_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          04/07/2001
 RCS:           $Id: iodrawimpl.h,v 1.4 2005-11-17 13:16:23 cvsarend Exp $
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

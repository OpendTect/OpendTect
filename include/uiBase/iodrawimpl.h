#ifndef iodrawimpl_h
#define iodrawimpl_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          04/07/2001
 RCS:           $Id: iodrawimpl.h,v 1.6 2007-03-28 12:20:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "iodraw.h"

class ioDrawTool;
class QPaintDevice;

class ioDrawAreaImpl : public ioDrawArea
{
public:
			ioDrawAreaImpl() : dt_(0)	{}
    virtual		~ioDrawAreaImpl()		{ releaseDrawTool(); }

    virtual ioDrawTool&	drawTool();

protected:

    virtual QPaintDevice* qPaintDevice()		= 0;
    void		releaseDrawTool(); 

private:

    ioDrawTool* 	dt_;

};

#endif

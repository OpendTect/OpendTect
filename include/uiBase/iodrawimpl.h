#ifndef iodrawimpl_h
#define iodrawimpl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          04/07/2001
 RCS:           $Id: iodrawimpl.h,v 1.11 2012-08-23 11:13:27 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "iodraw.h"

class ioDrawTool;
mFDQtclass(QPaintDevice)

mClass(uiBase) ioDrawAreaImpl : public ioDrawArea
{
public:
				ioDrawAreaImpl()
				    : dt_(0)	{}
    virtual			~ioDrawAreaImpl()
    				{ releaseDrawTool(); }

    virtual ioDrawTool&		drawTool();

protected:

    virtual mQtclass(QPaintDevice*)	qPaintDevice()		= 0;
    void			releaseDrawTool(); 

private:

    ioDrawTool*			dt_;

};

#endif


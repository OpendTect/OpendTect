#ifndef iodrawimpl_H
#define iodrawimpl_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          04/07/2001
 RCS:           $Id: iodrawimpl.h,v 1.1 2001-08-23 15:02:41 windev Exp $
________________________________________________________________________

-*/

#include <iodraw.h>
#include <iodrawtool.h>

class ioDrawAreaImpl : public ioDrawArea
{
public:
			ioDrawAreaImpl() : mDrawTool(0)	{}
    virtual		~ioDrawAreaImpl()		{ delete mDrawTool; }

    virtual ioDrawTool*	drawTool_( int x0, int y0 )
			    {
				 if ( mDrawTool ) 
				     mDrawTool->setOrigin( x0, y0 );
				 else
				     mDrawTool = 
					new ioDrawTool(mQPaintDevice(),x0,y0);
				 return mDrawTool; 
			    };

protected:

    virtual QPaintDevice* mQPaintDevice()		=0;

    void		releaseDrawTool() 
			    { if( mDrawTool ) {delete mDrawTool; mDrawTool=0;} }

private:

    ioDrawTool* 	mDrawTool;

};

#endif

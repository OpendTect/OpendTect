#ifndef uidrawable_H
#define uidrawable_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: uidrawable.h,v 1.6 2001-10-10 15:26:43 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
#include <iodraw.h>


class uiDrawableObj : public uiObject, public ioDrawArea
{

    mTTFriend(C,T,uiDrawableObjBody);
public:
			uiDrawableObj( uiParent* parnt, const char* nm, 
				       uiObjectBody& );

    virtual		~uiDrawableObj()	{}


    Notifier<uiDrawableObj> preDraw;
    Notifier<uiDrawableObj> postDraw;
    Notifier<uiDrawableObj> reSized;

mProtected:

/*! \brief handler for additional redrawing stuff

reDrawHandler() is called from uiDrawableObjBody::handlePaintEvent, which

1) triggers preDraw on associated uiDrawableObj (i.e. this)

2) calls Qt's paintEvent handler on associated widget

3) calls reDrawHandler() on associated uiDrawableObj (i.e. this)

4) triggers postDraw on associated uiDrawableObj (i.e. this)

Subclasses can override this method to do some additional drawing.

\sa uiDrawableObjBody::handlePaintEvent( uiRect r, QPaintEvent* QPEv=0 )

*/
    virtual void        reDrawHandler( uiRect updateArea )      {}
    virtual void        reSizeHandler( uiSize, uiSize )         {}

    virtual ioDrawTool* drawTool_( int x0, int y0 );

};


#endif

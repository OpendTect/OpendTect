#ifndef uidrawable_H
#define uidrawable_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: uidrawable.h,v 1.2 2001-05-02 13:50:11 windev Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
#include <iodraw.h>


class uiDrawableObj : public uiObject , public ioDrawArea
{
    mTFriend		(T,i_drwblQObj);
mTProtected:
			uiDrawableObj( uiObject* parnt, const char* nm )
			: uiObject( parnt, nm )
			, preDraw(this)
			, postDraw(this)
			, reSized(this)		{}

    virtual		~uiDrawableObj();
    virtual QPaintDevice* mQPaintDevice();

    Notifier<uiDrawableObj> preDraw;
    Notifier<uiDrawableObj> postDraw;
    Notifier<uiDrawableObj> reSized;

protected:

    virtual void	reDrawHandler( uiRect updateArea )	{}
    virtual void	reSizeHandler( uiSize, uiSize )		{}

};


#endif

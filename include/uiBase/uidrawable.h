#ifndef uidrawable_H
#define uidrawable_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: uidrawable.h,v 1.4 2001-05-16 14:58:38 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
#include <iodraw.h>


class uiDrawableObj : public uiObject , public ioDrawArea
{
    mTFriend		(T,i_drwblQObj);
public:
			uiDrawableObj( uiParent* parnt, const char* nm )
			: uiObject( parnt, nm )
			, preDraw(this)
			, postDraw(this)
			, reSized(this)		{}

    virtual		~uiDrawableObj();
    virtual QPaintDevice* mQPaintDevice();

    Notifier<uiDrawableObj> preDraw;
    Notifier<uiDrawableObj> postDraw;
    Notifier<uiDrawableObj> reSized;

mProtected:

    virtual void	reDrawHandler( uiRect updateArea )	{}
    virtual void	reSizeHandler( uiSize, uiSize )		{}

};


#endif

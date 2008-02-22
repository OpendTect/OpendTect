#ifndef uigraphicsitem_h
#define uigraphicsitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2007
 RCS:		$Id: uigraphicsitem.h,v 1.1 2008-02-22 12:26:24 cvsnanne Exp $
________________________________________________________________________

-*/

#include "callback.h"

class QGraphicsItem;


class uiGraphicsItem : public CallBacker
{
public:
    			uiGraphicsItem( QGraphicsItem* itm )
			    : qgraphicsitem_(itm)		{}
			~uiGraphicsItem()			{}

    QGraphicsItem*	qGraphicsItem()		{ return qgraphicsitem_; }

    void		show();
    void		hide();
    bool		isVisible() const;

    void		setPos(float x,float y);
    void		moveBy(float x,float y);
    void		rotate(float angle);
    void		scale(float x,float y);


protected:

    QGraphicsItem*	qgraphicsitem_;
};

#endif

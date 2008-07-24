#ifndef uigraphicsitem_h
#define uigraphicsitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2007
 RCS:		$Id: uigraphicsitem.h,v 1.3 2008-07-24 03:57:24 cvsnanne Exp $
________________________________________________________________________

-*/

#include "callback.h"

class Color;
class LineStyle;

class QGraphicsItem;
class QGraphicsItemGroup;


class uiGraphicsItem : public CallBacker
{
public:
			~uiGraphicsItem()			{}

    QGraphicsItem*	qGraphicsItem()		{ return qgraphicsitem_; }

    void		show();
    void		hide();
    bool		isVisible() const;

    void		setPos(float x,float y);
    void		moveBy(float x,float y);
    void		rotate(float angle);
    void		scale(float x,float y);

    void		setPenStyle(const LineStyle&);
    void		setFillColor(const Color&);

protected:
    			uiGraphicsItem( QGraphicsItem* itm )
			    : qgraphicsitem_(itm)		{}

    QGraphicsItem*	qgraphicsitem_;

    virtual QGraphicsItem*	mkQtObj()			= 0;
};


class uiGraphicsItemGroup : public uiGraphicsItem
{
public:
    			uiGraphicsItemGroup();
			~uiGraphicsItemGroup();

    void		add(uiGraphicsItem*);
    void		remove(uiGraphicsItem*);

protected:

    QGraphicsItem*	mkQtObj();

    QGraphicsItemGroup*	qgraphicsitemgrp_;
    ObjectSet<uiGraphicsItem>	items_;
};

#endif

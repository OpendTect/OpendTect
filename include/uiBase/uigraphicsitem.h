#ifndef uigraphicsitem_h
#define uigraphicsitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2007
 RCS:		$Id: uigraphicsitem.h,v 1.4 2008-09-01 07:41:24 cvssatyaki Exp $
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
    void		setZValue(int);

    virtual void	setPenStyle(const LineStyle&);
    virtual void	setPenColor(const Color&);
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
			uiGraphicsItemGroup(QGraphicsItemGroup*);
			~uiGraphicsItemGroup();

    void		add(uiGraphicsItem*);
    void		remove(uiGraphicsItem*,bool);
    void		removeAll(bool);

    QGraphicsItemGroup*	qGraphicsItemGroup()	{ return qgraphicsitemgrp_; }

protected:

    QGraphicsItem*	mkQtObj();

    QGraphicsItemGroup*	qgraphicsitemgrp_;
    ObjectSet<uiGraphicsItem>	items_;
};

#endif

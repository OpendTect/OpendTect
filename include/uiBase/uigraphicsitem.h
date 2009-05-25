#ifndef uigraphicsitem_h
#define uigraphicsitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2007
 RCS:		$Id: uigraphicsitem.h,v 1.14 2009-05-25 15:29:19 cvsbert Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "uigeom.h"

class Color;
class LineStyle;
class MouseCursor;
class uiGraphicsScene;

class QGraphicsItem;
class QGraphicsItemGroup;


mClass uiGraphicsItem : public CallBacker
{
public:
			~uiGraphicsItem()			{}

    QGraphicsItem*	qGraphicsItem()		{ return qgraphicsitem_; }

    void		show();
    void		hide();
    virtual bool	isVisible() const;
    virtual void	setVisible(bool);
    void		setSelectable(bool);
    void		setSelected(bool);
    bool		isSelectable();
    bool		isSelected() const		{ return selected_; }

    uiPoint		getPos() const;
    virtual void	setPos(int x,int y);
    virtual void	setPos(const uiPoint&); 
    void		moveBy(float x,float y);
    void		rotate(float angle);
    void		scale(float sx,float sy);
    void		scaleAroundXY(float sx,float sy,int x,int y);
    void		setZValue(int);
    uiPoint		transformToScenePos(const uiPoint& itmpos) const;
    virtual uiRect	boundingRect() const;

    virtual void	setPenStyle(const LineStyle&);
    virtual void	setPenColor(const Color&);
    void		setFillColor(const Color&);

    void		setCursor(const MouseCursor&);

    void		setParent(uiGraphicsItem*);
    uiGraphicsItem*	addToScene(uiGraphicsScene*);

protected:
    			uiGraphicsItem( QGraphicsItem* itm )
			    : qgraphicsitem_(itm)
			    , selected_(false)			{}

    QGraphicsItem*	qgraphicsitem_;

    virtual QGraphicsItem*	mkQtObj()			= 0;
    bool			selected_; // Remove when things in Qt works
};



mClass uiGraphicsItemGroup : public uiGraphicsItem
{
public:
    			uiGraphicsItemGroup();
			uiGraphicsItemGroup(QGraphicsItemGroup*);
			uiGraphicsItemGroup(const ObjectSet<uiGraphicsItem>&);
			~uiGraphicsItemGroup();

    void		add(uiGraphicsItem*);
    void		remove(uiGraphicsItem*,bool);
    void		removeAll(bool);
    bool		isEmpty() const		{ return items_.isEmpty(); }
    int			getSize() const		{ return items_.size(); }
    uiGraphicsItem* 	getUiItem( int idx )	{ return gtItm(idx); }
    const uiGraphicsItem* getUiItem( int idx ) const	{ return gtItm(idx); }

    virtual bool	isVisible() const;
    virtual void	setVisible(bool);
    virtual uiRect	boundingRect() const;
    QGraphicsItemGroup*	qGraphicsItemGroup()	{ return qgraphicsitemgrp_; }

protected:

    QGraphicsItem*	mkQtObj();

    bool		isvisible_;
    QGraphicsItemGroup*	qgraphicsitemgrp_;
    ObjectSet<uiGraphicsItem>	items_;

    uiGraphicsItem*	gtItm( int idx ) const
			{ return !items_.validIdx(idx) ? 0
			: const_cast<uiGraphicsItemGroup*>(this)->items_[idx]; }
};


#endif

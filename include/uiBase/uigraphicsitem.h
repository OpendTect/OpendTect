#ifndef uigraphicsitem_h
#define uigraphicsitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2007
 RCS:		$Id: uigraphicsitem.h,v 1.20 2010-02-11 07:12:37 cvsnanne Exp $
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
    const QGraphicsItem* qGraphicsItem() const	{ return qgraphicsitem_; }

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
    virtual void	setFillColor(const Color&);

    void		setCursor(const MouseCursor&);

    void		setParent(uiGraphicsItem*);
    uiGraphicsItem*	addToScene(uiGraphicsScene*);

    int			id() const			{ return id_; }

protected:
    			uiGraphicsItem( QGraphicsItem* itm )
			    : qgraphicsitem_(itm)
			    , id_(getNewID())
			    , selected_(false)			{}

    QGraphicsItem*	qgraphicsitem_;

    virtual QGraphicsItem* mkQtObj()			= 0;
    bool		selected_; // Remove when things in Qt works

private:
    static int		getNewID();
    const int		id_;

};



mClass uiGraphicsItemGroup : public uiGraphicsItem
{
public:
    			uiGraphicsItemGroup(bool owner=false);
			uiGraphicsItemGroup(const ObjectSet<uiGraphicsItem>&);
			~uiGraphicsItemGroup();
			//!<If owner, it deletes all items

    void		setIsOwner( bool own )	{ owner_ = own; }
    bool		isOwner() const		{ return owner_; }

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

    bool		owner_;
    bool		isvisible_;
    QGraphicsItemGroup*	qgraphicsitemgrp_;
    ObjectSet<uiGraphicsItem>	items_;
    ObjectSet<uiGraphicsItem>	items2bdel_;

    uiGraphicsItem*	gtItm( int idx ) const
			{ return !items_.validIdx(idx) ? 0
			: const_cast<uiGraphicsItemGroup*>(this)->items_[idx]; }
};


#endif

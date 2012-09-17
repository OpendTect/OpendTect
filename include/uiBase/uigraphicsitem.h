#ifndef uigraphicsitem_h
#define uigraphicsitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2007
 RCS:		$Id: uigraphicsitem.h,v 1.31 2012/09/17 08:42:08 cvsbert Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "uigeom.h"
#include "manobjectset.h"

class Color;
class LineStyle;
class FillPattern;
class MouseCursor;
class uiGraphicsScene;

class QGraphicsItem;
class QGraphicsItemGroup;


mClass uiGraphicsItem : public CallBacker
{
public:
			~uiGraphicsItem();

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
    void		setPos( const uiPoint& p )	{ stPos(p.x,p.y); }
    void		setPos( int x, int y )		{ stPos(x,y); }
    void		moveBy(float x,float y);
    void		rotate(float angle);
    void		scale(float sx,float sy);
    void		scaleAroundXY(float sx,float sy,int x,int y);
    void		setZValue(int); //<! z value decides the stacking order

    uiPoint		transformToScenePos(const uiPoint& itmpos) const;
    void		setItemIgnoresTransformations(bool);
    virtual uiRect	boundingRect() const;

    virtual void	setPenStyle(const LineStyle&,bool colwithalpha=false);
    virtual void	setPenColor(const Color&,bool withalpha=false);
    virtual void	setFillColor(const Color&,bool withalpha=false);
    virtual void	setFillPattern(const FillPattern&);

    void		setCursor(const MouseCursor&);

    void		setScene(uiGraphicsScene*);
    void		setParent(uiGraphicsItem*);
    uiGraphicsItem*	addToScene(uiGraphicsScene*);

    int			id() const			{ return id_; }

protected:

    			uiGraphicsItem(QGraphicsItem*);

    QGraphicsItem*	qgraphicsitem_;

    virtual QGraphicsItem* mkQtObj()			{ return 0; }
    bool		selected_; // Remove when things in Qt works
    uiGraphicsScene*	scene_;

private:

    			uiGraphicsItem() : id_(0)	{}

    static int		getNewID();
    const int		id_;

    virtual void	stPos(int,int);

};


mClass uiGraphicsItemSet : public ManagedObjectSet<uiGraphicsItem>
{
public:
			uiGraphicsItemSet()
			    : ManagedObjectSet<uiGraphicsItem>(false)	{}

    void		add( uiGraphicsItem* itm )	{ (*this) += itm; }
    uiGraphicsItem*	get( int idx )			{ return (*this)[idx]; }
    const uiGraphicsItem* get( int idx ) const		{ return (*this)[idx]; }

    void		setZValue(int); //<! z value decides the stacking order
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
    int			size() const		{ return items_.size(); }
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

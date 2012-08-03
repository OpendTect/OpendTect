#ifndef uigraphicsitem_h
#define uigraphicsitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2007
 RCS:		$Id: uigraphicsitem.h,v 1.37 2012-08-03 13:00:51 cvskris Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "callback.h"
#include "uigeom.h"
#include "manobjectset.h"

class Color;
class LineStyle;
class MouseCursor;
class uiGraphicsScene;

class QGraphicsItem;
class QGraphicsItemGroup;


mClass(uiBase) uiGraphicsItem : public CallBacker
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
    void		setPos( const uiWorldPoint&);
    void		setPos( const uiPoint& p );
    void		setPos( const Geom::Point2D<float>& );
    void		setPos( float x, float y );
    void		moveBy(float x,float y);
    void		setRotation(float angle);
    void		setScale(float sx,float sy);
    void		setZValue(int); //<! z value decides the stacking order

    uiPoint		transformToScenePos(const uiPoint& itmpos) const;
    void		setItemIgnoresTransformations(bool);
    virtual uiRect	boundingRect() const;

    virtual void	setPenStyle(const LineStyle&,bool colwithalpha=false);
    virtual void	setPenColor(const Color&,bool withalpha=false);
    virtual void	setFillColor(const Color&,bool withalpha=false);

    void		setCursor(const MouseCursor&);

    virtual void	setScene(uiGraphicsScene*);
    void		setParent(uiGraphicsItem*);

    int			id() const			{ return id_; }

    			//Old, will be remove once all dep code is changed
    void		rotate(float angle) { setRotation(angle); }
    void		scale(float sx,float sy) { setScale( sx, sy ); }
protected:

    			uiGraphicsItem(QGraphicsItem*);

    QGraphicsItem*	qgraphicsitem_;

    virtual QGraphicsItem* mkQtObj()			{ return 0; }
    bool		selected_; // Remove when things in Qt works
    uiGraphicsScene*	scene_;

private:

    			uiGraphicsItem() : id_(0)	{}
    void		updateTransform();

    static int		getNewID();
    const int		id_;

    virtual void	stPos(float,float);

    uiWorldPoint	translation_;
    uiWorldPoint	scale_;
    double		angle_;
};


mClass(uiBase) uiGraphicsItemSet : public ManagedObjectSet<uiGraphicsItem>
{
public:
			uiGraphicsItemSet()
			    : ManagedObjectSet<uiGraphicsItem>(false)	{}

    void		add( uiGraphicsItem* itm )	{ (*this) += itm; }
    uiGraphicsItem*	get( int idx )			{ return (*this)[idx]; }
    const uiGraphicsItem* get( int idx ) const		{ return (*this)[idx]; }

    void		setZValue(int); //<! z value decides the stacking order
};



mClass(uiBase) uiGraphicsItemGroup : public uiGraphicsItem
{
public:
    			uiGraphicsItemGroup(bool owner=false);
			uiGraphicsItemGroup(const ObjectSet<uiGraphicsItem>&);
			~uiGraphicsItemGroup();
			//!<If owner, it deletes all items

    void		setScene(uiGraphicsScene*);

    void		setIsOwner( bool own )	{ owner_ = own; }
    bool		isOwner() const		{ return owner_; }

    void		add(uiGraphicsItem*);
    void		remove(uiGraphicsItem*,bool withdelete);
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


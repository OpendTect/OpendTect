#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "callback.h"
#include "keyenum.h"
#include "uigeom.h"
#include "manobjectset.h"
#include "uistring.h"

class FillPattern;
class MouseCursor;
class MouseEvent;
class uiGraphicsItemSet;
class uiGraphicsScene;
namespace OD { class LineStyle; }

mFDQtclass(QGraphicsItem)
mFDQtclass(QGraphicsItemGroup)

mExpClass(uiBase) uiGraphicsItem : public CallBacker
{
public:
			uiGraphicsItem();
			~uiGraphicsItem();

    mQtclass(QGraphicsItem*)	qGraphicsItem()	{ return qgraphicsitem_; }
    const mQtclass(QGraphicsItem*) qGraphicsItem()
				   const { return qgraphicsitem_; }

    void		show();
    void		hide();

    virtual void	setAcceptHoverEvents(bool);
    virtual void	setAcceptedMouseButtons(OD::ButtonState);
    virtual void	setFiltersChildEvents(bool);
    virtual void	setMovable(bool);
    virtual void	setSelectable(bool);
    virtual void	setSelected(bool);
    virtual void	setVisible(bool);

    virtual OD::ButtonState acceptedMouseButtonsEnabled() const;
    virtual bool	isFiltersChildEventsEnabled() const;
    virtual bool	isHoverEventsAccepted() const;
    virtual bool	isMovable() const;
    virtual bool	isSelectable() const;
    virtual bool	isSelected() const		{ return selected_; }
    virtual bool	isVisible() const;

    Geom::Point2D<float> getPos() const;
    void		setPos( const uiWorldPoint&);
    void		setPos( const uiPoint& p );
    void		setPos( const Geom::Point2D<float>& );
    void		setPos( float x, float y );
    void		moveBy(float x,float y);
    float		getRotation();
    void		getScale(float& sx,float& sy);
    void		setRotation(float angle);
    void		setScale(float sx,float sy);
    void		setZValue(int); //<! z value decides the stacking order


    uiGraphicsItem*	getChild(int);
    bool		isPresent(const uiGraphicsItem&) const;
    int			nrChildren() const;
    void		removeChild(uiGraphicsItem*,bool withdelete);
    void		removeAll(bool withdelete);
    void		addChild(uiGraphicsItem*);
    void		addChildSet(uiGraphicsItemSet&);
    void		removeChildSet(uiGraphicsItemSet&);

    uiPoint		transformToScenePos(const uiPoint& itmpos) const;
    void		setItemIgnoresTransformations(bool);
    bool		isItemIgnoresTransformationsEnabled() const;
    virtual uiRect	boundingRect() const;

    virtual void	setPenStyle(const OD::LineStyle&,bool usetransp=false);
    virtual void	setPenColor(const OD::Color&,bool usetransp=false);
    virtual void	setFillColor(const OD::Color&,bool usetransp=false);
    virtual void	setGradientFill(int x1,int y1,int x2, int y2,
					const TypeSet<float>& stops,
					const TypeSet<OD::Color>& colors);
			/*!< Creates a linear gradient from (x1,y1) to (x2,y2).
			 stops are values between 0 and 1 on the gradient line
			 where colors are defined */
    virtual void	setFillPattern(const FillPattern&);
    virtual void	setTransparency(float);
			/*!< To set the overall transparency of graphics item.
			 Passed value should be between 0 and 1. If the item
			 contains colors with some transparency, resultant
			 transparency of the color will be an effective value
			 of both transparencies. */
    float		getTransparency() const;

    void		setCursor(const MouseCursor&);
    void		setToolTip(const uiString&);

    virtual void	setScene(uiGraphicsScene*);
    virtual uiGraphicsItem*	findItem(QGraphicsItem*);

    int			id() const			{ return id_; }
    int			getZValue() const;

			//Old, will be remove once all dep code is changed
    void		rotate(float angle) { setRotation(angle); }
    void		scale(float sx,float sy) { setScale( sx, sy ); }

    virtual void	translateText();

    CNotifier<uiGraphicsItem,const MouseEvent&> leftClicked;
    CNotifier<uiGraphicsItem,const MouseEvent&> rightClicked;
protected:

			uiGraphicsItem(QGraphicsItem*);

    mQtclass(QGraphicsItem*) qgraphicsitem_;

    virtual mQtclass(QGraphicsItem*) mkQtObj()		{ return nullptr; }
    bool		selected_; // Remove when things in Qt works
    mQtclass(uiGraphicsScene*)	scene_;
    ObjectSet<uiGraphicsItem> children_;
    uiGraphicsItem*	parent_;

    virtual void	stPos(float,float);

private:

    void		updateTransform();

    static int		getNewID();
    const int		id_;

    uiString		tooltip_;

    uiWorldPoint	translation_;
    uiWorldPoint	scale_;
    double		angle_;
};


mExpClass(uiBase) uiGraphicsItemSet : public ManagedObjectSet<uiGraphicsItem>
{
public:
			uiGraphicsItemSet();
			~uiGraphicsItemSet();

    void		add( uiGraphicsItem* itm )	{ (*this) += itm; }

    void		setZValue(int); //<! z value decides the stacking order
    void		setVisible(bool yn);
};



mExpClass(uiBase) uiGraphicsItemGroup : public uiGraphicsItem
{
public:
			uiGraphicsItemGroup(bool owner=false);
			uiGraphicsItemGroup(const ObjectSet<uiGraphicsItem>&);
			~uiGraphicsItemGroup();
			//!<If owner, it deletes all items

    void		setScene(uiGraphicsScene*) override;

    void		setIsOwner( bool own )	{ owner_ = own; }
    bool		isOwner() const		{ return owner_; }

    void		add(uiGraphicsItem*);
    void		remove(uiGraphicsItem*,bool withdelete);
    void		removeAll(bool withdelete);
    bool		isEmpty() const		{ return items_.isEmpty(); }
    int			size() const		{ return items_.size(); }
    uiGraphicsItem*	getUiItem( int idx )	{ return gtItm(idx); }
    const uiGraphicsItem* getUiItem( int idx ) const	{ return gtItm(idx); }
    bool		isPresent(const uiGraphicsItem&) const;
    uiGraphicsItem*	findItem(QGraphicsItem*) override;

    bool		isVisible() const override;
    void		setVisible(bool) override;
    uiRect		boundingRect() const override;
    mQtclass(QGraphicsItemGroup*)	qGraphicsItemGroup()
					{ return qgraphicsitemgrp_; }

protected:

    void				translateText() override;

    mQtclass(QGraphicsItem*)		mkQtObj() override;

    bool				owner_;
    bool				isvisible_;
    mQtclass(QGraphicsItemGroup*)	qgraphicsitemgrp_;
    ObjectSet<uiGraphicsItem>		items_;
    ObjectSet<uiGraphicsItem>		items2bdel_;

    uiGraphicsItem*	gtItm( int idx ) const
			{ return !items_.validIdx(idx) ? 0
			: const_cast<uiGraphicsItemGroup*>(this)->items_[idx]; }
};

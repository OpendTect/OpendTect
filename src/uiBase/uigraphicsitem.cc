/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsitem.cc,v 1.13 2009-04-06 13:56:03 cvsnanne Exp $";


#include "uigraphicsitem.h"
#include "uigraphicsscene.h"

#include "uicursor.h"

#include "draw.h"

#include <QBrush>
#include <QCursor>
#include <QGraphicsItemGroup>
#include <QPen>
#include <QTransform>


void uiGraphicsItem::show()	{ qgraphicsitem_->show(); }
void uiGraphicsItem::hide()	{ qgraphicsitem_->hide(); }

bool uiGraphicsItem::isVisible() const
{ return qgraphicsitem_->isVisible(); }

void uiGraphicsItem::setVisible( bool yn )
{ qgraphicsitem_->setVisible( yn ); }


uiPoint uiGraphicsItem::getPos() const
{
    return uiPoint( (int)qgraphicsitem_->pos().x(),
		    (int)qgraphicsitem_->pos().y() );
}


void uiGraphicsItem::setPos( int x, int y )
{ setPos( uiPoint(x,y) ); }

void uiGraphicsItem::setPos( const uiPoint& pt )
{ qgraphicsitem_->setPos( pt.x, pt.y ); }

void uiGraphicsItem::moveBy( float x, float y )
{ qgraphicsitem_->moveBy( x, y ); }

void uiGraphicsItem::rotate( float angle )
{ qgraphicsitem_->rotate( angle ); }

void uiGraphicsItem::scale( float sx, float sy )
{ qgraphicsitem_->scale( sx, sy ); }

void uiGraphicsItem::scaleAroundXY( float sx, float sy, int x, int y )
{
    qgraphicsitem_->setTransform( QTransform().translate(x,y)
	   			  .scale(sx,sy).translate(-x,-y) );
}

void uiGraphicsItem::setZValue( int x )
{ qgraphicsitem_->setZValue( x ); }


uiPoint uiGraphicsItem::transformToScenePos( const uiPoint& pt ) const
{
    QPointF qpt = qgraphicsitem_->mapToScene( pt.x, pt.y );
    return uiPoint( qpt.x(), qpt.y() );
}


void uiGraphicsItem::setPenColor( const Color& col )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QPen qpen( QColor(QRgb(col.rgb())) );
    agsitm->setPen( qpen );
}


bool uiGraphicsItem::isSelectable()
{
    return qgraphicsitem_->flags().testFlag( QGraphicsItem::ItemIsSelectable );
}


void uiGraphicsItem::setSelected( bool yn )
{
    selected_ = yn;
}


void uiGraphicsItem::setSelectable( bool yn )
{
    qgraphicsitem_->setFlag( QGraphicsItem::ItemIsSelectable, yn );
}


void uiGraphicsItem::setParent( uiGraphicsItem* item )
{
    qgraphicsitem_->setParentItem( item->qgraphicsitem_ );
}


uiGraphicsItem* uiGraphicsItem::addToScene( uiGraphicsScene* sc )
{
    if ( sc ) sc->doAddItem( this );
    return this;
}


void uiGraphicsItem::setPenStyle( const LineStyle& ls )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QBrush qbrush( QColor(QRgb(ls.color_.rgb())) );
    QPen qpen( qbrush, ls.width_, (Qt::PenStyle)ls.type_ );
    agsitm->setPen( qpen );
}


void uiGraphicsItem::setFillColor( const Color& col )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QBrush qbrush( QColor(QRgb(col.rgb())) );
    agsitm->setBrush( qbrush );
}


void uiGraphicsItem::setCursor( const MouseCursor& cursor )
{
    QCursor qcursor;
    uiCursorManager::fillQCursor( cursor, qcursor );
    qgraphicsitem_->setCursor( qcursor );
}


// +++++ uiGraphicsItemGroup +++++

uiGraphicsItemGroup::uiGraphicsItemGroup()
    : uiGraphicsItem(mkQtObj())
    , isvisible_(true)
{}


uiGraphicsItemGroup::uiGraphicsItemGroup( QGraphicsItemGroup* qtobj )
    : uiGraphicsItem(qtobj)
    , qgraphicsitemgrp_(qtobj)
    , isvisible_(true)
{}


uiGraphicsItemGroup::uiGraphicsItemGroup( const ObjectSet<uiGraphicsItem>& grp )
    : uiGraphicsItem(mkQtObj())
    , isvisible_(true)
{
    ObjectSet<uiGraphicsItem>& itms =
				const_cast<ObjectSet<uiGraphicsItem>&>(grp);
    for ( int idx=0; idx<itms.size(); idx++ )
	add( itms[idx] );
}


uiGraphicsItemGroup::~uiGraphicsItemGroup()
{
    delete qgraphicsitemgrp_;
}


QGraphicsItem* uiGraphicsItemGroup::mkQtObj()
{
    qgraphicsitemgrp_ = new QGraphicsItemGroup;
    return qgraphicsitemgrp_;
}


void uiGraphicsItemGroup::add( uiGraphicsItem* itm )
{
    items_ += itm;
    qgraphicsitemgrp_->addToGroup( itm->qGraphicsItem() );
}


void uiGraphicsItemGroup::remove( uiGraphicsItem* itm, bool withdelete )
{
    items_ -= itm;
    qgraphicsitemgrp_->removeFromGroup( itm->qGraphicsItem() );
    if ( withdelete )
	delete itm;
}


void uiGraphicsItemGroup::removeAll( bool withdelete )
{
    while ( !items_.isEmpty() )
	remove( items_[0], withdelete );
}


void uiGraphicsItemGroup::setVisible( bool yn )
{
    isvisible_ = yn;
    for ( int idx=0; idx<getSize(); idx++ )
	getUiItem(idx)->setVisible( yn );
}


bool uiGraphicsItemGroup::isVisible() const
{
    return isvisible_;
}

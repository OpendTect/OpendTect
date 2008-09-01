/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: uigraphicsitem.cc,v 1.4 2008-09-01 07:41:19 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "uigraphicsitem.h"

#include "color.h"
#include "draw.h"

#include <QBrush>
#include <QGraphicsItemGroup>
#include <QPen>


void uiGraphicsItem::show()	{ qgraphicsitem_->show(); }
void uiGraphicsItem::hide()	{ qgraphicsitem_->hide(); }

bool uiGraphicsItem::isVisible() const
{ return qgraphicsitem_->isVisible(); }

void uiGraphicsItem::setPos( float x, float y )
{ qgraphicsitem_->setPos( x, y ); }

void uiGraphicsItem::moveBy( float x, float y )
{ qgraphicsitem_->moveBy( x, y ); }

void uiGraphicsItem::rotate( float angle )
{ qgraphicsitem_->rotate( angle ); }

void uiGraphicsItem::scale( float x, float y )
{ qgraphicsitem_->scale( x, y ); }

void uiGraphicsItem::setZValue( int x )
{ qgraphicsitem_->setZValue( x ); }

void uiGraphicsItem::setPenColor( const Color& col )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    agsitm->pen().setColor( QColor(QRgb(col.rgb())) );
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


// +++++ uiGraphicsItemGroup +++++

uiGraphicsItemGroup::uiGraphicsItemGroup()
    : uiGraphicsItem(mkQtObj())
{}


uiGraphicsItemGroup::uiGraphicsItemGroup( QGraphicsItemGroup* qtobj )
    : uiGraphicsItem(qtobj)
    , qgraphicsitemgrp_(qtobj)
{}


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

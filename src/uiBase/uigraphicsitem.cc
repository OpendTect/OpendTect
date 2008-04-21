/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: uigraphicsitem.cc,v 1.2 2008-04-21 04:27:34 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uigraphicsitem.h"

#include <QGraphicsItemGroup>


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


// +++++ uiGraphicsItemGroup +++++

uiGraphicsItemGroup::uiGraphicsItemGroup()
    : uiGraphicsItem(mkQtObj())
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


void uiGraphicsItemGroup::remove( uiGraphicsItem* itm )
{
    items_ -= itm;
    qgraphicsitemgrp_->removeFromGroup( itm->qGraphicsItem() );
}

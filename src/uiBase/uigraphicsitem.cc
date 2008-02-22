/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: uigraphicsitem.cc,v 1.1 2008-02-22 12:26:24 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uigraphicsitem.h"

#include <QGraphicsItem>


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



/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: uigraphicsscene.cc,v 1.1 2008-02-22 12:26:24 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uigraphicsscene.h"

#include "pixmap.h"
#include <QGraphicsScene>


uiGraphicsScene::uiGraphicsScene( const char* nm )
    : NamedObject(nm)
    , qgraphicsscene_(new QGraphicsScene)
{
    qgraphicsscene_->setObjectName( nm );
}


uiGraphicsScene::~uiGraphicsScene()
{
    delete qgraphicsscene_;
}


void uiGraphicsScene::addPixmap( const ioPixmap& pm )
{ qgraphicsscene_->addPixmap( *pm.qpixmap() ); }

void uiGraphicsScene::addText( const char* txt )
{ qgraphicsscene_->addText( txt ); }

void uiGraphicsScene::addRect( float x, float y, float w, float h )
{ qgraphicsscene_->addRect( x, y, w, h ); }

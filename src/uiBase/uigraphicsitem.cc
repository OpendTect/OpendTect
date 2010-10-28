/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsitem.cc,v 1.26 2010-10-28 06:01:34 cvsnanne Exp $";


#include "uigraphicsitem.h"
#include "uigraphicsscene.h"

#include "uicursor.h"

#include "draw.h"

#include <QBrush>
#include <QCursor>
#include <QGraphicsItemGroup>
#include <QGraphicsScene>
#include <QPen>
#include <QTransform>


uiGraphicsItem::uiGraphicsItem( QGraphicsItem* itm )
    : qgraphicsitem_(itm)
    , scene_(0)
    , id_(getNewID())
    , selected_(false)
{
}


uiGraphicsItem::~uiGraphicsItem()
{
    if ( scene_ )
	scene_->removeItem( this );
    delete qgraphicsitem_;
}


void uiGraphicsItem::setScene( uiGraphicsScene& scene )
{ scene_ = &scene; }


int uiGraphicsItem::getNewID()
{
    static Threads::Mutex mutex;
    Threads::MutexLocker lock( mutex );
    static int curid = 1;
    return curid++;
}


void uiGraphicsItem::show()	{ qgraphicsitem_->show(); }
void uiGraphicsItem::hide()	{ qgraphicsitem_->hide(); }

bool uiGraphicsItem::isVisible() const
{ return qgraphicsitem_->isVisible(); }

void uiGraphicsItem::setVisible( bool yn )
{ qgraphicsitem_->setVisible( yn ); }


uiPoint uiGraphicsItem::getPos() const
{
    return uiPoint( mNINT(qgraphicsitem_->pos().x()),
		    mNINT(qgraphicsitem_->pos().y()) );
}


uiRect uiGraphicsItem::boundingRect() const
{
    QRectF qr( qgraphicsitem_->sceneBoundingRect() );
    return uiRect( mNINT(qr.left()), mNINT(qr.top()),
	    	   mNINT(qr.right()), mNINT(qr.bottom()) );
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
    return uiPoint( mNINT(qpt.x()),  mNINT( qpt.y()) );
}


void uiGraphicsItem::setItemIgnoresTransformations( bool yn )
{ qgraphicsitem_->setFlag( QGraphicsItem::ItemIgnoresTransformations, yn ); }

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
    qgraphicsitem_->setParentItem( item ? item->qgraphicsitem_ : 0 );
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


void uiGraphicsItem::setFillColor( const Color& col, bool withalpha )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QColor color = QColor(QRgb(col.rgb()));
    if ( withalpha ) color.setAlpha( col.t() );
    QBrush qbrush( color );
    agsitm->setBrush( qbrush );
}


void uiGraphicsItem::setCursor( const MouseCursor& cursor )
{
    QCursor qcursor;
    uiCursorManager::fillQCursor( cursor, qcursor );
    qgraphicsitem_->setCursor( qcursor );
}


// +++++ uiGraphicsItemGroup +++++

uiGraphicsItemGroup::uiGraphicsItemGroup( bool owner )
    : uiGraphicsItem(mkQtObj())
    , isvisible_(true)
    , owner_(owner)
{}


uiGraphicsItemGroup::uiGraphicsItemGroup( const ObjectSet<uiGraphicsItem>& grp )
    : uiGraphicsItem(mkQtObj())
    , isvisible_(true)
    , owner_(false)
{
    ObjectSet<uiGraphicsItem>& itms =
				const_cast<ObjectSet<uiGraphicsItem>&>(grp);
    for ( int idx=0; idx<itms.size(); idx++ )
	add( itms[idx] );
}


uiGraphicsItemGroup::~uiGraphicsItemGroup()
{
    removeAll( owner_ );
    delete qgraphicsitemgrp_;
    deepErase( items2bdel_ );
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
    if ( !itm ) return;

    items_ -= itm;
    QGraphicsItem* qitm = itm->qGraphicsItem();
    qgraphicsitemgrp_->removeFromGroup( qitm );
    if ( withdelete )
    {
	if ( qitm && qitm->scene() )
	    qitm->scene()->removeItem( qitm );
//	delete itm; TODO: This delete leads to crash in Qt 4.4.3
	itm->setVisible( false );
	items2bdel_ += itm;
    }
}


void uiGraphicsItemGroup::removeAll( bool withdelete )
{
    while ( !items_.isEmpty() )
	remove( items_[0], withdelete );
}


void uiGraphicsItemGroup::setVisible( bool yn )
{
    isvisible_ = yn;
    for ( int idx=0; idx<items_.size(); idx++ )
	items_[idx]->setVisible( yn );
}


bool uiGraphicsItemGroup::isVisible() const
{
    return isvisible_;
}


uiRect uiGraphicsItemGroup::boundingRect() const
{
    if ( isEmpty() ) return uiRect();

    uiRect ret( getUiItem(0)->boundingRect() );
    for ( int idx=1; idx<items_.size(); idx++ )
	ret.include( items_[idx]->boundingRect() );
    return ret;
}

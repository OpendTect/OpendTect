/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: uigraphicsscene.cc,v 1.3 2008-09-01 07:41:19 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "uigraphicsscene.h"

#include "color.h"
#include "keyboardevent.h"
#include "math.h"
#include "mouseevent.h"
#include "odgraphicsitem.h"
#include "pixmap.h"
#include "uigeom.h"
#include "uigraphicsitem.h"
#include "uigraphicsitemimpl.h"

#include <QBrush>
#include <QColor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QList>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QPolygonF>
#include <QVector>


class ODGraphicsScene : public QGraphicsScene
{
public:
    			ODGraphicsScene( uiGraphicsScene& scene )
			    : uiscene_(scene)
			    , bgopaque_(false)	{}

    void		setBackgroundOpaque( bool yn )	{ bgopaque_ = yn; }
protected:
    virtual void	mouseMoveEvent(QGraphicsSceneMouseEvent*);
    virtual void	mousePressEvent(QGraphicsSceneMouseEvent*);
    virtual void	mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    virtual void	mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);

    virtual void	drawBackground(QPainter*,const QRectF&);

private:

    bool		bgopaque_;

    uiGraphicsScene&	uiscene_;
};


void ODGraphicsScene::drawBackground( QPainter* painter, const QRectF& rect )
{
    painter->setBackgroundMode( bgopaque_ ? Qt::OpaqueMode 
	    				  : Qt::TransparentMode );
    QGraphicsScene::drawBackground( painter, rect );
}


void ODGraphicsScene::mouseMoveEvent( QGraphicsSceneMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    MouseEvent mev( bs, (int)qev->scenePos().x(), (int)qev->scenePos().y() );
    uiscene_.getMouseEventHandler().triggerMovement( mev );
    QGraphicsScene::mouseMoveEvent( qev );
}


void ODGraphicsScene::mousePressEvent( QGraphicsSceneMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    MouseEvent mev( bs, (int)qev->scenePos().x(), (int)qev->scenePos().y() );
    uiscene_.getMouseEventHandler().triggerButtonPressed( mev );
    QGraphicsScene::mousePressEvent( qev );
}


void ODGraphicsScene::mouseReleaseEvent( QGraphicsSceneMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    MouseEvent mev( bs, (int)qev->scenePos().x(), (int)qev->scenePos().y() );
    uiscene_.getMouseEventHandler().triggerButtonReleased( mev );
    QGraphicsScene::mouseReleaseEvent( qev );
}


void ODGraphicsScene::mouseDoubleClickEvent( QGraphicsSceneMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    MouseEvent mev( bs, (int)qev->scenePos().x(), (int)qev->scenePos().y() );
    uiscene_.getMouseEventHandler().triggerDoubleClick( mev );
    QGraphicsScene::mouseDoubleClickEvent( qev );
}


uiGraphicsScene::uiGraphicsScene( const char* nm )
    : NamedObject(nm)
    , qgraphicsscene_(new ODGraphicsScene(*this))
    , odgraphicsscene_(new ODGraphicsScene(*this))
{
    qgraphicsscene_->setObjectName( nm );
}


uiGraphicsScene::~uiGraphicsScene()
{
    delete qgraphicsscene_;
}


void uiGraphicsScene::addItem( uiGraphicsItem* itm )
{
   qgraphicsscene_->addItem( itm->qGraphicsItem() );
}


void uiGraphicsScene::addItemGrp( uiGraphicsItemGroup* itmgrp )
{
   qgraphicsscene_->addItem( itmgrp->qGraphicsItemGroup() );
}


void uiGraphicsScene::removeItem( uiGraphicsItem* itm )
{
    qgraphicsscene_->removeItem( itm->qGraphicsItem() );
}


int uiGraphicsScene::sceneitemsz()
{
    return qgraphicsscene_->items().size();
}


uiGraphicsItemGroup* uiGraphicsScene::addItemGrp( ObjectSet<uiGraphicsItem> grp)
{
    QList<QGraphicsItem*> qitmlist;
    for ( int idx=0; idx<grp.size(); idx++ )
	qitmlist += grp[idx]->qGraphicsItem();
    uiGraphicsItemGroup* itemgrp = new uiGraphicsItemGroup(
	qgraphicsscene_->createItemGroup(qitmlist) );
    return itemgrp;
}


uiPixmapItem* uiGraphicsScene::addPixmap( const ioPixmap& pm )
{
    uiPixmapItem* uipixitem =  new uiPixmapItem(
	qgraphicsscene_->addPixmap( *pm.qpixmap() ) );
    return uipixitem;
}


uiTextItem* uiGraphicsScene::addText( const char* txt )
{ 
    uiTextItem* uitextitem = new uiTextItem( qgraphicsscene_->addText(txt) );
    return uitextitem;
}


uiRectItem* uiGraphicsScene::addRect( float x, float y, float w, float h )
{ 
    uiRectItem* uirectitem =
	new uiRectItem( qgraphicsscene_->addRect(x,y,w,h) );
    uirectitem->moveBy( -w/2, -h/2 );
    return uirectitem;
}


uiEllipseItem* uiGraphicsScene::addEllipse( float x, float y, float w, float h )
{
    uiEllipseItem* uieclipseitem =
	new uiEllipseItem( qgraphicsscene_->addEllipse(x,y,w,h) );
    uieclipseitem->moveBy( -w/2, -h/2 );
    return uieclipseitem;
}


uiEllipseItem* uiGraphicsScene::addEllipse( const uiPoint& center,
					    const uiSize& sz )
{
    return addEllipse( center.x, center.y, sz.hNrPics(), sz.vNrPics() );
}


inline
static uiPoint getEndPoint( const uiPoint& pt, double angle, double len )
{
    uiPoint endpt( pt );
    double delta = len * cos( angle );
    endpt.x += mNINT(delta);
    // In UI, Y is positive downward
    delta = -len * sin( angle );
    endpt.y += mNINT(delta);
    return endpt;
}


uiLineItem* uiGraphicsScene::addLine( float x, float y, float w, float h )
{
    uiLineItem* uilineitem =
	new uiLineItem( qgraphicsscene_->addLine(x,y,w,h) );
    return uilineitem;
}    


uiLineItem* uiGraphicsScene::addLine( const uiPoint& p1,
				      const uiPoint& p2 )
{
    return addLine( p1.x, p1.y, p2.x, p2.y );
}


uiLineItem* uiGraphicsScene::addLine( const uiPoint& pt, double angle,
				      double len )
{
    return addLine( pt, getEndPoint(pt,angle,len) );
}


uiPolygonItem* uiGraphicsScene::addPolygon( const TypeSet<uiPoint>& pts,
					    bool fill )
{
    QVector<QPointF> qpts;
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	QPointF pt( QPoint(pts[idx].x,pts[idx].y) );
	qpts += pt;
    }

    uiPolygonItem* uipolyitem = new uiPolygonItem(
	qgraphicsscene_->addPolygon(QPolygonF(qpts)) );
    if ( fill )
	uipolyitem->fill();
    return uipolyitem;
}


uiPointItem* uiGraphicsScene::addPoint( bool hl )
{
    uiPointItem* uiptitem = new uiPointItem();
    uiptitem->qPointItem()->setHighLight( hl );
    return uiptitem;
}


uiMarkerItem* uiGraphicsScene::addMarker( const MarkerStyle2D& mstyl, int side )
{
    uiMarkerItem* markeritem = new uiMarkerItem();
    markeritem->qMarkerItem()->setMarkerStyle( mstyl );
    markeritem->qMarkerItem()->setSideLength( side );
    return markeritem;
}


uiArrowItem* uiGraphicsScene::addArrow( const uiPoint& tail,
					const uiPoint& head,
					const ArrowStyle& arrstyle )
{
    uiArrowItem* arritem = new uiArrowItem();
    arritem->setArrowStyle( arrstyle );
    arritem->setPos( head.x, head.y );
    const uiPoint relvec( head.x - tail.x, tail.y - head.y );
    const double ang( atan2(relvec.y,relvec.x) );
    arritem->rotate( ang );
    return arritem;
}


void uiGraphicsScene::setBackGroundColor( const Color& color )
{
    QBrush brush( QColor(color.r(),color.g(),color.b()) );
    qgraphicsscene_->setBackgroundBrush( brush );
}


const Color uiGraphicsScene::backGroundColor() const
{
    QColor color( qgraphicsscene_->backgroundBrush().color() ); 
    return Color( color.red() , color.green(), color.blue() );
}


void uiGraphicsScene::removeAllItems()
{
    QList<QGraphicsItem *> items = qgraphicsscene_->items();
    for ( int idx=0; idx<items.size(); idx++ )
	qgraphicsscene_->removeItem( items[idx] );
}


void uiGraphicsScene::useBackgroundPattern( bool usebgpattern )
{
    odgraphicsscene_->setBackgroundOpaque( usebgpattern );

    if ( usebgpattern )
    {
	QBrush brush;
	brush.setColor( QColor(0,0,0) );
	brush.setStyle( Qt::DiagCrossPattern );
	qgraphicsscene_->setBackgroundBrush( brush );
    }
}

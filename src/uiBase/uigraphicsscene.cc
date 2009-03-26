/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsscene.cc,v 1.21 2009-03-26 08:26:41 cvssatyaki Exp $";


#include "uigraphicsscene.h"

#include "draw.h"
#include "oddirs.h"
#include "filepath.h"
#include "odgraphicsitem.h"
#include "uidialog.h"
#include "uigraphicsitemimpl.h"

#include <QList>
#include <QByteArray>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QImageWriter>
#include <QKeyEvent>
#include <QPainter>
#include <QPoint>
#include <QPrinter>
#include <QString>
#include <math.h>

#if !defined( __win__ ) && !defined( __mac__ )
# include <QX11Info>
#endif



class ODGraphicsScene : public QGraphicsScene
{
public:
    			ODGraphicsScene( uiGraphicsScene& scene )
			    : uiscene_(scene)
			    , bgopaque_(false)
			    , mousepressedbs_(OD::NoButton)
			    , startpos_( *new QPoint() )	{}

    void		setBackgroundOpaque( bool yn )	{ bgopaque_ = yn; }
protected:
    virtual void	keyPressEvent(QKeyEvent* qkeyevent);
    virtual void	mouseMoveEvent(QGraphicsSceneMouseEvent*);
    virtual void	mousePressEvent(QGraphicsSceneMouseEvent*);
    virtual void	mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    virtual void	mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);

    virtual void	drawBackground(QPainter*,const QRectF&);
    OD::ButtonState	mousepressedbs_;

private:

    bool		bgopaque_;

    QPoint&		startpos_;
    uiGraphicsScene&	uiscene_;
};


void ODGraphicsScene::drawBackground( QPainter* painter, const QRectF& rect )
{
    painter->setBackgroundMode( bgopaque_ ? Qt::OpaqueMode 
	    				  : Qt::TransparentMode );
    QGraphicsScene::drawBackground( painter, rect );
}


void ODGraphicsScene::keyPressEvent( QKeyEvent* qkeyevent )
{
    OD::KeyboardKey key = OD::KeyboardKey( qkeyevent->key() );
    OD::ButtonState modifier = OD::ButtonState( (int)qkeyevent->modifiers() );
    if ( key == OD::P && modifier == OD::ControlButton )
	uiscene_.ctrlPPressed.trigger();
}


void ODGraphicsScene::mouseMoveEvent( QGraphicsSceneMouseEvent* qev )
{
    MouseEvent mev( mousepressedbs_, (int)qev->scenePos().x(),
	    	    (int)qev->scenePos().y() );
    if ( uiscene_.isMouseEventActive() )
	uiscene_.getMouseEventHandler().triggerMovement( mev );
    QGraphicsScene::mouseMoveEvent( qev );
}


void ODGraphicsScene::mousePressEvent( QGraphicsSceneMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    if ( bs == OD::LeftButton )
	startpos_ = QPoint( (int)qev->scenePos().x(),(int)qev->scenePos().y() );
    mousepressedbs_ = bs;
    MouseEvent mev( bs, (int)qev->scenePos().x(), (int)qev->scenePos().y() );
    if ( uiscene_.isMouseEventActive() )
	uiscene_.getMouseEventHandler().triggerButtonPressed( mev );
    QGraphicsScene::mousePressEvent( qev );
}


void ODGraphicsScene::mouseReleaseEvent( QGraphicsSceneMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    mousepressedbs_ = OD::NoButton;
    if ( bs == OD::LeftButton )
    {
	const QPoint& stoppos = QPoint( (int)qev->scenePos().x(),
					(int)qev->scenePos().y() );
	const QRectF selrect( startpos_, stoppos );
	QPainterPath selareapath;
	selareapath.addRect( selrect );
	setSelectionArea( selareapath );
    }
    MouseEvent mev( bs, (int)qev->scenePos().x(), (int)qev->scenePos().y() );
    if ( uiscene_.isMouseEventActive() )
	uiscene_.getMouseEventHandler().triggerButtonReleased( mev );
    QGraphicsScene::mouseReleaseEvent( qev );
}


void ODGraphicsScene::mouseDoubleClickEvent( QGraphicsSceneMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    MouseEvent mev( bs, (int)qev->scenePos().x(), (int)qev->scenePos().y() );
    if ( uiscene_.isMouseEventActive() )
	uiscene_.getMouseEventHandler().triggerDoubleClick( mev );
    QGraphicsScene::mouseDoubleClickEvent( qev );
}



uiGraphicsScene::uiGraphicsScene( const char* nm )
    : NamedObject(nm)
    , mousehandler_(MouseEventHandler())
    , keyboardhandler_(KeyboardEventHandler())
    , ismouseeventactive_(true)
    , odgraphicsscene_(new ODGraphicsScene(*this))
    , ctrlPPressed(this)
{
    odgraphicsscene_->setObjectName( nm );
    odgraphicsscene_->setBackgroundBrush( Qt::white );
}


uiGraphicsScene::~uiGraphicsScene()
{
    delete odgraphicsscene_;
}


int uiGraphicsScene::nrItems() const
{
    return odgraphicsscene_->items().size();
}


BufferStringSet uiGraphicsScene::supportedImageFormat()
{
    BufferStringSet imageformats;
    QList<QByteArray> imgfrmts = QImageWriter::supportedImageFormats();
    for ( int idx=0; idx<imgfrmts.size(); idx++ )
	imageformats.add( imgfrmts[idx].data() );
    return imageformats;
}


void uiGraphicsScene::addItem( uiGraphicsItem* itm )
{
    if ( !itm ) return;
    odgraphicsscene_->addItem( itm->qGraphicsItem() );
    items_ += itm;
}


void uiGraphicsScene::addItemGrp( uiGraphicsItemGroup* itmgrp )
{
    if ( !itmgrp ) return;
    odgraphicsscene_->addItem( itmgrp->qGraphicsItemGroup() );
    items_ += itmgrp;
}


void uiGraphicsScene::removeItem( uiGraphicsItem* itm )
{
    if ( !itm ) return;
    odgraphicsscene_->removeItem( itm->qGraphicsItem() );
    items_ -= itm;
}


uiPixmapItem* uiGraphicsScene::addPixmap( const ioPixmap& pm )
{
    uiPixmapItem* uipixitem = new uiPixmapItem( new ODGraphicsPixmapItem(pm) );
    addItem( uipixitem );
    return uipixitem;
}


uiTextItem* uiGraphicsScene::addText( const char* txt )
{ 
    uiTextItem* uitextitem = new uiTextItem( odgraphicsscene_->addText(txt) );
    items_ += uitextitem;
    return uitextitem;
}

uiTextItem* uiGraphicsScene::addText( int x, int y, const char* text,
       				      const Alignment& al )
{
    uiTextItem* uitextitem = new uiTextItem( odgraphicsscene_->addText(text) );
    uitextitem->setPos( x, y );
    uitextitem->setAlignment( al );
    items_ += uitextitem;
    return uitextitem;
}


uiRectItem* uiGraphicsScene::addRect( float x, float y, float w, float h )
{ 
    uiRectItem* uirectitem =
	new uiRectItem( odgraphicsscene_->addRect(x,y,w,h) );
    uirectitem->moveBy( -w/2, -h/2 );
    items_ += uirectitem;
    return uirectitem;
}


uiEllipseItem* uiGraphicsScene::addEllipse( float x, float y, float w, float h )
{
    uiEllipseItem* uiellipseitem =
	new uiEllipseItem( odgraphicsscene_->addEllipse(x,y,w,h) );
    uiellipseitem->moveBy( -w/2, -h/2 );
    items_ += uiellipseitem;
    return uiellipseitem;
}


uiEllipseItem* uiGraphicsScene::addEllipse( const uiPoint& center,
					    const uiSize& sz )
{
    return addEllipse( center.x, center.y, sz.hNrPics(), sz.vNrPics() );
}


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
	new uiLineItem( odgraphicsscene_->addLine(x,y,w,h) );
    items_ += uilineitem;
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
	odgraphicsscene_->addPolygon(QPolygonF(qpts)) );
    if ( fill )
	uipolyitem->fill();
    items_ += uipolyitem;
    return uipolyitem;
}

uiPolyLineItem* uiGraphicsScene::addPolyLine( const TypeSet<uiPoint>& ptlist )
{
    uiPolyLineItem* polylineitem = new uiPolyLineItem();
    polylineitem->setPolyLine( ptlist );
    addItem( polylineitem );
    return polylineitem;
}


uiPointItem* uiGraphicsScene::addPoint( bool hl )
{
    uiPointItem* uiptitem = new uiPointItem();
    uiptitem->qPointItem()->setHighLight( hl );
    addItem( uiptitem );
    return uiptitem;
}


uiMarkerItem* uiGraphicsScene::addMarker( const MarkerStyle2D& mstyl, int side )
{
    uiMarkerItem* markeritem = new uiMarkerItem( mstyl );
    markeritem->qMarkerItem()->setSideLength( side );
    addItem( markeritem );
    return markeritem;
}


uiArrowItem* uiGraphicsScene::addArrow( const uiPoint& tail,
					const uiPoint& head,
					const ArrowStyle& arrstyle )
{
    uiArrowItem* arritem = new uiArrowItem();
    arritem->setArrowStyle( arrstyle );
    const int arrsz = (int)sqrt( (float)(head.x - tail.x)*(head.x - tail.x) +
		      	         (float)(tail.y - head.y)*(tail.y - head.y) );
    arritem->setArrowSize( arrsz );
    arritem->setPos( head.x, head.y );
    const uiPoint relvec( head.x - tail.x, tail.y - head.y );
    const float ang = atan2((float)relvec.y,(float)relvec.x) *180/M_PI;
    arritem->rotate( ang );
    addItem( arritem );
    return arritem;
}


void uiGraphicsScene::setBackGroundColor( const Color& color )
{
    QBrush brush( QColor(color.r(),color.g(),color.b()) );
    odgraphicsscene_->setBackgroundBrush( brush );
}


const Color uiGraphicsScene::backGroundColor() const
{
    QColor color( odgraphicsscene_->backgroundBrush().color() ); 
    return Color( color.red() , color.green(), color.blue() );
}


void uiGraphicsScene::removeAllItems()
{
    QList<QGraphicsItem *> items = odgraphicsscene_->items();
    for ( int idx=0; idx<items.size(); idx++ )
	odgraphicsscene_->removeItem( items[idx] );
    deepErase( items_ );
}


int uiGraphicsScene::getSelItemSize() const
{
    QList<QGraphicsItem*> items = odgraphicsscene_->selectedItems();
    return items.size();
}


uiRect uiGraphicsScene::getSelectedArea() const
{
    QRectF selarea( odgraphicsscene_->selectionArea().boundingRect().toRect() );
    return uiRect( (int)selarea.topLeft().x(), (int)selarea.topLeft().y(),
	   	   (int)selarea.bottomRight().x(),
		   (int)selarea.bottomRight().y() );
}


void uiGraphicsScene::setSelectionArea( const uiRect& rect )
{
    const QRectF selrect( rect.topLeft().x, rect.topLeft().y, rect.width(),
	    		  rect.height() );
    QPainterPath selareapath;
    selareapath.addRect( selrect );
    odgraphicsscene_->setSelectionArea( selareapath );
}


void uiGraphicsScene::useBackgroundPattern( bool usebgpattern )
{
    odgraphicsscene_->setBackgroundOpaque( usebgpattern );

    if ( usebgpattern )
    {
	QBrush brush;
	brush.setColor( QColor(0,0,0) );
	brush.setStyle( Qt::DiagCrossPattern );
	odgraphicsscene_->setBackgroundBrush( brush );
    }
}


double uiGraphicsScene::width() const
{ return odgraphicsscene_->width(); }

double uiGraphicsScene::height() const
{ return odgraphicsscene_->height(); }

int uiGraphicsScene::getDPI() const
{
// TODO: move to Basic
#if defined( __win__ ) || defined( __mac__ )
    return 100;
#else
    return QX11Info::appDpiX();
#endif
}


void uiGraphicsScene::setSceneRect( float x, float y, float w, float h )
{ odgraphicsscene_->setSceneRect( x, y, w, h ); }


uiRect uiGraphicsScene::sceneRect()
{
    QRectF qrect = odgraphicsscene_->sceneRect();
    return uiRect( (int)qrect.x(), (int)qrect.y(), (int)qrect.width(), (int)qrect.height() );
}


void uiGraphicsScene::saveAsImage( const char* filename, int width,
				   int height, int resolution )
{
    QString fileName( filename );
    QPainter *imagepainter = new QPainter();
    QImage* image = new QImage( QSize(width,height), QImage::Format_ARGB32);
    image->setDotsPerMeterX( resolution*254 );
    image->setDotsPerMeterY( resolution*254 );
    imagepainter->begin(image);
    qGraphicsScene()->render(imagepainter);
    imagepainter->end();
    image->save(fileName);
    delete imagepainter;
    delete image;
}


void uiGraphicsScene::saveAsPDF_PS( const char* filename, bool aspdf,
       				    int resolution )
{
    QString fileName( filename );
    QPrinter* pdfprinter = new QPrinter();
    pdfprinter->setOutputFormat( aspdf ? QPrinter::PdfFormat
				       : QPrinter::PostScriptFormat );
    pdfprinter->setPageSize( QPrinter::A4 );
    pdfprinter->setFullPage( true );
    pdfprinter->setOutputFileName( filename );

    QPainter* pdfpainter = new QPainter();
    pdfpainter->begin( pdfprinter );
    qGraphicsScene()->render( pdfpainter );
    pdfpainter->end();
    delete pdfpainter;
    delete pdfprinter;
}


void uiGraphicsScene::saveAsPDF( const char* filename, int resolution )
{ saveAsPDF_PS( filename, true, resolution ); }

void uiGraphicsScene::saveAsPS( const char* filename, int resolution )
{ saveAsPDF_PS( filename, false, resolution ); }

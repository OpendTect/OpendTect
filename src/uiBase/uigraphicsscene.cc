/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/


#include "uigraphicsscene.h"

#include "draw.h"
#include "threadwork.h"
#include "uimain.h"
#include "uigraphicsitemimpl.h"

#include <QApplication>
#include <QByteArray>
#include <QClipboard>
#include <QGraphicsLinearLayout>
#include <QGraphicsItemGroup>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QImageWriter>
#include <QKeyEvent>
#include <QList>
#include <QPainter>
#include <QPoint>
#include <QPrinter>
#include <QString>
#include <math.h>

mUseQtnamespace

class ODGraphicsScene : public QGraphicsScene
{
public:
			ODGraphicsScene( uiGraphicsScene& scene )
			    : uiscene_(scene)
			    , bgopaque_(false)
			    , mousepressedbs_(OD::NoButton) {}

    void		setBackgroundOpaque( bool yn )	{ bgopaque_ = yn; }
protected:
    virtual void	wheelEvent(QGraphicsSceneWheelEvent* qevent);
    virtual void	keyPressEvent(QKeyEvent* qkeyevent);
    virtual void	mouseMoveEvent(QGraphicsSceneMouseEvent*);
    virtual void	mousePressEvent(QGraphicsSceneMouseEvent*);
    virtual void	mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    virtual void	mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);

    virtual void	drawBackground(QPainter*,const QRectF&);
    OD::ButtonState	mousepressedbs_;

private:

    uiGraphicsScene&	uiscene_;
    bool		bgopaque_;
};


void ODGraphicsScene::drawBackground( QPainter* painter, const QRectF& rect )
{
    uiscene_.setPixelDensity( (painter->device()->logicalDpiX()+
				painter->device()->logicalDpiY())/2 );
    uiscene_.executePendingUpdates();
    painter->setBackgroundMode( bgopaque_ ? Qt::OpaqueMode
					  : Qt::TransparentMode );
    QGraphicsScene::drawBackground( painter, rect );
}


void ODGraphicsScene::keyPressEvent( QKeyEvent* qkeyevent )
{
    const OD::KeyboardKey key = OD::KeyboardKey( qkeyevent->key() );
    const OD::ButtonState modifier =
		OD::ButtonState( sCast(int,qkeyevent->modifiers()) );
    if ( key == OD::KB_P && modifier == OD::ControlButton )
	uiscene_.ctrlPPressed.trigger();
    else if ( key == OD::KB_C && modifier == OD::ControlButton )
	uiscene_.ctrlCPressed.trigger();
    QGraphicsScene::keyPressEvent( qkeyevent );
}


void ODGraphicsScene::wheelEvent( QGraphicsSceneWheelEvent* ev )
{
    if ( !ev ) return;

    MouseEvent me( OD::ButtonState(ev->modifiers() | ev->buttons()),
		   ev->pos().x(), ev->pos().y(), ev->delta() );
    uiscene_.getMouseEventHandler().triggerWheel( me );
    ev->accept();
}


void ODGraphicsScene::mouseMoveEvent( QGraphicsSceneMouseEvent* qev )
{
    if ( !qev ) return;

    MouseEvent mev( mousepressedbs_, qev->scenePos().x(), qev->scenePos().y() );
    if ( uiscene_.isMouseEventActive() )
	uiscene_.getMouseEventHandler().triggerMovement( mev );
    QGraphicsScene::mouseMoveEvent( qev );
}


void ODGraphicsScene::mousePressEvent( QGraphicsSceneMouseEvent* qev )
{
    if ( !qev ) return;

    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    const Qt::MouseButtons qtmbs = qev->buttons();
    if ( qtmbs&Qt::LeftButton && qtmbs&Qt::RightButton )
	bs = OD::ButtonState( qev->modifiers() | Qt::MiddleButton );

    mousepressedbs_ = bs;
    MouseEvent mev( bs, qev->scenePos().x(), qev->scenePos().y() );
    if ( uiscene_.isMouseEventActive() )
	uiscene_.getMouseEventHandler().triggerButtonPressed( mev );
    QGraphicsScene::mousePressEvent( qev );
}


void ODGraphicsScene::mouseReleaseEvent( QGraphicsSceneMouseEvent* qev )
{
    if ( !qev ) return;

    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    if ( mousepressedbs_ & OD::MidButton )
	bs = OD::ButtonState( qev->modifiers() | Qt::MiddleButton );

    mousepressedbs_ = OD::NoButton;
    MouseEvent mev( bs, qev->scenePos().x(), qev->scenePos().y() );
    if ( uiscene_.isMouseEventActive() )
	uiscene_.getMouseEventHandler().triggerButtonReleased( mev );

    QGraphicsScene::mouseReleaseEvent( qev );
}


void ODGraphicsScene::mouseDoubleClickEvent( QGraphicsSceneMouseEvent* qev )
{
    if ( !qev ) return;

    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    MouseEvent mev( bs, qev->scenePos().x(), qev->scenePos().y() );
    if ( uiscene_.isMouseEventActive() )
	uiscene_.getMouseEventHandler().triggerDoubleClick( mev );
    QGraphicsScene::mouseDoubleClickEvent( qev );
}


uiGraphicsScene::uiGraphicsScene( const char* nm )
    : NamedCallBacker(nm)
    , mousehandler_(MouseEventHandler())
    , ismouseeventactive_(true)
    , odgraphicsscene_(new ODGraphicsScene(*this))
    , ctrlPPressed(this)
    , ctrlCPressed(this)
    , pixeldensity_( getDefaultPixelDensity() )
    , pixelDensityChange( this )
{
    odgraphicsscene_->setObjectName( nm );
    odgraphicsscene_->setBackgroundBrush( Qt::white );
    ctrlCPressed.notify( mCB(this,uiGraphicsScene,CtrlCPressedCB) );

    BufferString queuename("Graphics Scene ", nm );
    queueid_ = Threads::WorkManager::twm().addQueue(
	    Threads::WorkManager::Manual, queuename );
}


uiGraphicsScene::~uiGraphicsScene()
{
    removeAllItems();
    delete odgraphicsscene_;
    Threads::WorkManager::twm().removeQueue( queueid_, false );
}


int uiGraphicsScene::nrItems() const
{
    return odgraphicsscene_->items().size();
}


uiGraphicsSceneChanger::uiGraphicsSceneChanger( uiGraphicsScene& scene,
	uiGraphicsItem& itm, bool remove )
    : scene_( &scene )
    , group_( 0 )
    , itm_( itm )
    , remove_( remove )
{}


uiGraphicsSceneChanger::uiGraphicsSceneChanger( uiGraphicsItemGroup& group,
	uiGraphicsItem& itm, bool remove )
    : group_( &group )
    , itm_( itm )
    , scene_( 0 )
    , remove_( remove )
{}


bool uiGraphicsSceneChanger::execute()
{
    if ( !isMainThreadCurrent() )
    {
	pErrMsg("Violating QT threading");
	return false;
    }

    if ( remove_ )
    {
	if ( scene_ ) scene_->removeItem( &itm_ );
	else group_->remove( &itm_, false );
    }
    else
    {
	if ( scene_ ) scene_->addItem( &itm_ );
	else group_->add( &itm_ );
    }

    return true;
}


bool uiGraphicsScene::executePendingUpdates()
{
    return Threads::WorkManager::twm().executeQueue( queueid_ );
}


void uiGraphicsScene::translateText()
{
    for ( int idx=0; idx<items_.size(); idx++ )
    {
	items_[idx]->translateText();
    }
}


void uiGraphicsScene::addUpdateToQueue( Task* t )
{
    Threads::WorkManager::twm().addWork(
	Threads::Work( *t, true ), 0, queueid_, false );

    odgraphicsscene_->update();
}


uiGraphicsItem* uiGraphicsScene::doAddItem( uiGraphicsItem* itm )
{
    if ( !itm ) return 0;

    if ( !isMainThreadCurrent() )
    {
	addUpdateToQueue( new uiGraphicsSceneChanger(*this,*itm,false) );
    }
    else
    {
	itm->setScene( this );
	odgraphicsscene_->addItem( itm->qGraphicsItem() );
	items_ += itm;
    }

    return itm;
}


uiGraphicsItemSet* uiGraphicsScene::addItems( uiGraphicsItemSet* itms )
{
    if ( !itms ) return 0;

    for ( int idx=0; idx<itms->size(); idx++ )
	doAddItem( itms->get(idx) );

    return itms;
}


uiGraphicsItemGroup* uiGraphicsScene::addItemGrp( uiGraphicsItemGroup* itmgrp )
{
    if ( !itmgrp ) return 0;

    itmgrp->setScene( this );
    odgraphicsscene_->addItem( itmgrp->qGraphicsItemGroup() );
    items_ += itmgrp;
    return itmgrp;
}


uiGraphicsItem* uiGraphicsScene::removeItem( uiGraphicsItem* itm )
{
    if ( !items_.isPresent(itm) || !itm )
	return 0;

    odgraphicsscene_->removeItem( itm->qGraphicsItem() );
    items_ -= itm;
    return itm;
}


uiGraphicsItemSet* uiGraphicsScene::removeItems( uiGraphicsItemSet* itms )
{
    if ( !itms ) return 0;

    for ( int idx=0; idx<itms->size(); idx++ )
	removeItem( (*itms)[idx] );

    return itms;
}


// TODO: remove
uiRectItem* uiGraphicsScene::addRect( float x, float y, float w, float h )
{
    uiRectItem* uirectitem =
	new uiRectItem( odgraphicsscene_->addRect(x,y,w,h) );
    uirectitem->setScene( this );
    items_ += uirectitem;
    return uirectitem;
}


// TODO: remove
uiPolygonItem* uiGraphicsScene::addPolygon( const TypeSet<uiPoint>& pts,
					    bool fill )
{
    uiPolygonItem* uipolyitem = new uiPolygonItem;
    uipolyitem->setPolygon( pts );
    if ( fill )
	uipolyitem->fill();

    addItem( uipolyitem );

    return uipolyitem;
}


// TODO: remove
uiPolyLineItem* uiGraphicsScene::addPolyLine( const TypeSet<uiPoint>& ptlist )
{
    uiPolyLineItem* polylineitem = new uiPolyLineItem();
    polylineitem->setPolyLine( ptlist );
    addItem( polylineitem );
    return polylineitem;
}


void uiGraphicsScene::setBackGroundColor( const Color& color )
{
    QBrush brush( QColor(color.r(),color.g(),color.b(),255-color.t()) );
    odgraphicsscene_->setBackgroundBrush( brush );
}


const Color uiGraphicsScene::backGroundColor() const
{
    QColor color( odgraphicsscene_->backgroundBrush().color() );
    return Color( color.red() , color.green(),
		  color.blue(), 255-color.alpha() );
}


void uiGraphicsScene::removeAllItems()
{
    for ( int idx=0; idx<items_.size(); idx++ )
    {
	uiGraphicsItem* itm = items_[idx];
	odgraphicsscene_->removeItem( itm->qGraphicsItem() );
	itm->setScene( 0 );
    }

    deepErase( items_ );

    QList<QGraphicsItem *> qitems = odgraphicsscene_->items();
    for ( int idx=0; idx<qitems.size(); idx++ )
	odgraphicsscene_->removeItem( qitems[idx] );
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


void uiGraphicsScene::setSelectionArea( const uiRect& uirect )
{
    uiRect rect = uirect;
    rect.sortCorners();
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


void uiGraphicsScene::setPixelDensity( float dpi )
{
    if ( dpi==pixeldensity_ )
	return;

    pixeldensity_ = dpi;

    pixelDensityChange.trigger();
}


float uiGraphicsScene::getDefaultPixelDensity()
{
    return uiMain::getMinDPI();
}


double uiGraphicsScene::width() const
{ return odgraphicsscene_->width(); }

double uiGraphicsScene::height() const
{ return odgraphicsscene_->height(); }


void uiGraphicsScene::setSceneRect( float x, float y, float w, float h )
{ odgraphicsscene_->setSceneRect( x, y, w, h ); }


uiRect uiGraphicsScene::sceneRect()
{
    QRectF qrect = odgraphicsscene_->sceneRect();
    return uiRect( (int)qrect.left(), (int)qrect.top(),
		   (int)qrect.right(), (int)qrect.bottom() );
}


void uiGraphicsScene::CtrlCPressedCB( CallBacker* )
{
    copyToClipBoard();
}


void uiGraphicsScene::copyToClipBoard()
{
    QPainter* imagepainter = new QPainter();
    QImage* image = new QImage( QSize((int)width(), (int)height()),
				QImage::Format_ARGB32 );
    QColor qcol( 255, 255, 255 );
    image->fill( qcol.rgb() );
    const IdxPair dpi = uiMain::getDPI();
    image->setDotsPerMeterX( mNINT32(dpi.first/0.0254) );
    image->setDotsPerMeterY( mNINT32(dpi.second/0.0254) );
    imagepainter->begin( image );

    QGraphicsView* view = qGraphicsScene()->views()[0];
    QRectF sourcerect( view->mapToScene(0,0),
		       view->mapToScene(view->width(),view->height()) );
    qGraphicsScene()->render( imagepainter, QRectF(0,0,width(),height()),
			      sourcerect );
    imagepainter->end();

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setImage( *image );
    delete imagepainter;
    delete image;
}


void uiGraphicsScene::saveAsImage( const char* fnm, int w, int h, int res )
{
    QString fname( fnm );
    QPainter* imagepainter = new QPainter();
    QImage* image = new QImage( QSize(w,h), QImage::Format_ARGB32 );
    QColor qcol( 255, 255, 255 );
    image->fill( qcol.rgb() );
    image->setDotsPerMeterX( sCast(int,(res/0.0254)) );
    image->setDotsPerMeterY( sCast(int,(res/0.0254)) );
    imagepainter->begin( image );

    QGraphicsView* view = qGraphicsScene()->views()[0];
    const int viewwidth = view->viewport()->width();
    const int viewheight = view->viewport()->height();
    QRectF sourcerect( view->mapToScene(0,0),
		       view->mapToScene(viewwidth-1,viewheight-1) );
    qGraphicsScene()->render( imagepainter, QRectF(0,0,w,h), sourcerect );
    imagepainter->end();
    image->save( fname );
    delete imagepainter;
    delete image;
}


void uiGraphicsScene::saveAsPDF_PS( const char* filename, bool aspdf,
				    int w, int h, int res )
{
    QString fileName( filename );
    auto* pdfprinter = new QPrinter();
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    pdfprinter->setOutputFormat( QPrinter::PdfFormat );
#else
    pdfprinter->setOutputFormat( aspdf ? QPrinter::PdfFormat
				       : QPrinter::PostScriptFormat );
#endif
    const QPageSize pgsz( QSizeF(w,h), QPageSize::Point );
    pdfprinter->setPageSize( pgsz );
    pdfprinter->setFullPage( false );
    pdfprinter->setOutputFileName( filename );
    pdfprinter->setResolution( res );

    auto* pdfpainter = new QPainter();
    pdfpainter->begin( pdfprinter );
    QGraphicsView* view = qGraphicsScene()->views()[0];
    QRectF sourcerect( view->mapToScene(0,0),
		       view->mapToScene(view->width(),view->height()) );
    qGraphicsScene()->render( pdfpainter,
	    QRectF(0,0,pdfprinter->width(),pdfprinter->height()) ,sourcerect );
    pdfpainter->end();
    delete pdfpainter;
    delete pdfprinter;
}


void uiGraphicsScene::saveAsPDF( const char* filename, int w, int h, int res )
{ saveAsPDF_PS( filename, true, w, h, res ); }


void uiGraphicsScene::saveAsPS( const char* filename, int w, int h, int res )
{ saveAsPDF_PS( filename, false, w, h, res ); }


int uiGraphicsScene::indexOf( int id ) const
{
    for ( int idx=0; idx<items_.size(); idx++ )
    {
	if ( items_[idx]->id() == id )
	    return idx;
    }

    return -1;
}


uiGraphicsItem* uiGraphicsScene::getItem( int id )
{
    const int idx = indexOf( id );
    return items_.validIdx(idx) ? items_[idx] : 0;
}


const uiGraphicsItem* uiGraphicsScene::getItem( int id ) const
{ return const_cast<uiGraphicsScene*>(this)->getItem(id); }


uiGraphicsItem* uiGraphicsScene::itemAt( const Geom::Point2D<float>& pos )
{
#if QT_VERSION >= 0x050000
    QGraphicsItem* qitm = odgraphicsscene_->itemAt( pos.x, pos.y, QTransform());
#else
    QGraphicsItem* qitm = odgraphicsscene_->itemAt( pos.x, pos.y );
#endif
    if ( !qitm ) return 0;

    for ( int idx=0; idx<items_.size(); idx++ )
    {
	uiGraphicsItem* oditm = items_[idx]->findItem( qitm );
	if ( oditm ) return oditm;
    }

    return 0;
}


const uiGraphicsItem*
	uiGraphicsScene::itemAt( const Geom::Point2D<float>& pos ) const
{ return const_cast<uiGraphicsScene*>(this)->itemAt( pos ); }



// uiGraphicsObjectScene
uiGraphicsObjectScene::uiGraphicsObjectScene( const char* nm )
    : uiGraphicsScene(nm)
    , layout_(new QGraphicsLinearLayout)
    , layoutitem_(new QGraphicsWidget)
{
    layoutitem_->setLayout( layout_ );
    qGraphicsScene()->addItem( layoutitem_ );
    layout_->setSpacing( 0 );
    layout_->setContentsMargins( 0, 0, 0, 0 );
    setLayoutPos( uiPoint(0, 0) );
}


void uiGraphicsObjectScene::setLayoutPos( const uiPoint& pt )
{
    layoutitem_->setPos( pt.x, pt.y );
}


const uiPoint uiGraphicsObjectScene::layoutPos() const
{
    return uiPoint( mNINT32(layoutitem_->pos().x()),
		    mNINT32(layoutitem_->pos().y()) );
}


void uiGraphicsObjectScene::resizeLayoutToContent()
{
    float w = 0; float h = 0;
    for ( int idx=0; idx<layout_->count(); idx++ )
    {
	mDynamicCastGet(uiObjectItem*,item,items_[idx]);
	if ( !item ) continue;

	w += item->objectSize().width();
	h = item->objectSize().height();
    }

    layoutitem_->resize( w, h );
}


#define mAddItm \
    item->setScene( this ); \
    items_ += item; \
    resizeLayoutToContent();


void uiGraphicsObjectScene::addObjectItem( uiObjectItem* item  )
{
    if ( !item ) return;

    layout_->addItem( item->qWidgetItem() );
    mAddItm
}


void uiGraphicsObjectScene::insertObjectItem( int idx, uiObjectItem* item )
{
    if ( !item ) return;

    layout_->insertItem( idx, item->qWidgetItem() );
    mAddItm
}


void uiGraphicsObjectScene::removeObjectItem( uiObjectItem* item )
{
    if ( !item ) return;

    layout_->removeItem( item->qWidgetItem() );
    removeItem( item );
    items_ -= item;
    resizeLayoutToContent();
}


void uiGraphicsObjectScene::setItemStretch( uiObjectItem* item, int stretch )
{
    if ( !item ) return;

    layout_->setStretchFactor( item->qWidgetItem(), stretch );
    resizeLayoutToContent();
}


int uiGraphicsObjectScene::stretchFactor( uiObjectItem* item ) const
{
    return item ? layout_->stretchFactor( item->qWidgetItem() ) : -1;
}


const uiSize uiGraphicsObjectScene::layoutSize() const
{
     return ( layoutitem_ ? uiSize( (int)layoutitem_->size().rwidth(),
				    (int)layoutitem_->size().rheight() )
			  : uiSize(0,0) );
}

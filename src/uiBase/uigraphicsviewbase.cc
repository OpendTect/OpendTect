/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsviewbase.cc,v 1.6 2009-04-13 10:29:15 cvsranojay Exp $";


#include "uigraphicsviewbase.h"

#include "mouseevent.h"
#include "uigraphicsscene.h"
#include "uiobjbody.h"

#include <QApplication>
#include <QGraphicsView>
#include <QWheelEvent>

const int sDefaultWidth  = 1;
const int sDefaultHeight = 1;


class uiGraphicsViewBody :
    public uiObjBodyImpl<uiGraphicsViewBase,QGraphicsView>
{
public:

uiGraphicsViewBody( uiGraphicsViewBase& handle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiGraphicsViewBase,QGraphicsView>(handle,p,nm)
    , mousehandler_( *new MouseEventHandler() )
    , startpos_(0)
    , handle_(handle)
{
    setStretch( 2, 2 );
    setPrefWidth( sDefaultWidth );
    setPrefHeight( sDefaultHeight );
    setTransformationAnchor( QGraphicsView::AnchorUnderMouse );
}

    MouseEventHandler&	mouseEventHandler()	{ return mousehandler_; }

    void		activateMenu();
    bool		event(QEvent*);

protected:

    uiPoint*		startpos_;
    OD::ButtonState	buttonstate_;
    MouseEventHandler&	mousehandler_;
    uiGraphicsViewBase&	handle_;

    void		paintEvent(QPaintEvent*);
    void		wheelEvent(QWheelEvent*);
    void		resizeEvent(QResizeEvent*);
    void		mouseMoveEvent(QMouseEvent*);
    void		mouseReleaseEvent(QMouseEvent*);
    void		mousePressEvent(QMouseEvent*);
    void		mouseDoubleClickEvent(QMouseEvent*);
};


static const QEvent::Type sQEventActMenu = (QEvent::Type) (QEvent::User+0);

void uiGraphicsViewBody::activateMenu()
{
    QEvent* actevent = new QEvent( sQEventActMenu );
    QApplication::postEvent( this, actevent );
}


bool uiGraphicsViewBody::event( QEvent* ev )
{
    if ( ev->type() == sQEventActMenu )
    {
	const MouseEvent right( OD::RightButton );
	mouseEventHandler().triggerButtonPressed( right );
    }
    else
	return QGraphicsView::event( ev );

    handle_.activatedone.trigger();
    return true;
}


void uiGraphicsViewBody::mouseMoveEvent( QMouseEvent* event )
{
    MouseEvent me( buttonstate_, event->x(), event->y() );
    mousehandler_.triggerMovement( me );
    QGraphicsView::mouseMoveEvent( event );
}


void uiGraphicsViewBody::mousePressEvent( QMouseEvent* event )
{
    if ( !event ) return;

    MouseEvent me;
    if ( event->button() == Qt::RightButton )
    {
	buttonstate_ = OD::RightButton;
	MouseEvent me( buttonstate_, event->x(), event->y() );
	mousehandler_.triggerButtonPressed( me );
    }
    else if ( event->button() == Qt::LeftButton )
    {
	startpos_ = new uiPoint( event->x(), event->y() );
	buttonstate_ = OD::LeftButton;
	MouseEvent me( buttonstate_, event->x(), event->y() );
	mousehandler_.triggerButtonPressed( me );
    }
    else
	buttonstate_ = OD::NoButton;

    QGraphicsView::mousePressEvent( event );
}


void uiGraphicsViewBody::mouseDoubleClickEvent( QMouseEvent* event )
{
    if ( !event | handle_.isRubberBandingOn() ) return;
    if ( event->button() == Qt::LeftButton )
    {
	MouseEvent me( OD::LeftButton, event->x(), event->y() );
	mousehandler_.triggerDoubleClick( me );
    }
    QGraphicsView::mouseDoubleClickEvent( event );
}


void uiGraphicsViewBody::mouseReleaseEvent( QMouseEvent* event )
{
    if ( !event ) return;
    if ( event->button() == Qt::LeftButton )
    {
	if ( event->modifiers() == Qt::ControlModifier )
	    buttonstate_ = OD::ControlButton;
	MouseEvent me( buttonstate_, event->x(), event->y() );
	mousehandler_.triggerButtonReleased( me );
	if ( handle_.isRubberBandingOn() )
	{
	    uiPoint* stoppos = new uiPoint( event->x(), event->y() );
	    uiRect selrect( *startpos_, *stoppos );
	    handle_.scene().setSelectionArea( selrect );
	}
    }

    QList<QGraphicsItem*> items = scene()->selectedItems();
    const int sz = items.size();

    QGraphicsView::mouseReleaseEvent( event );
}

static const int sBorder = 5;

void uiGraphicsViewBody::paintEvent( QPaintEvent* event )
{
    QGraphicsView::paintEvent( event );
}


void uiGraphicsViewBody::resizeEvent( QResizeEvent* event )
{
    if ( !event ) return;

    if ( handle_.scene_ )
    {
#if defined(__win__) && !defined(__msvc__)
	QSize newsz = event->size();
	handle_.scene_->setSceneRect( sBorder, sBorder,
				      newsz.width()-2*sBorder,
				      newsz.height()-2*sBorder );
#else
	handle_.scene_->setSceneRect( sBorder, sBorder,
				      width()-2*sBorder, height()-2*sBorder );
#endif
    }

    handle_.reSize.trigger();
}


void uiGraphicsViewBody::wheelEvent( QWheelEvent* ev )
{
    if ( ev && handle_.scrollZoomEnabled() )
    {
	int numsteps = ( ev->delta() / 8 ) / 15;

	QMatrix mat = matrix();
	QPointF mouseposition = ev->pos();

	mat.translate( (width()/2) - mouseposition.x(),
		       (height()/2) - mouseposition.y() );

	if ( numsteps > 0 )
	    mat.scale( numsteps * 1.2, numsteps * 1.2 );
	else
	    mat.scale( -1 / (numsteps*1.2), -1 / (numsteps*1.2) );

	mat.translate( mouseposition.x() - (width()/2),
		       mouseposition.y() - (height()/2) );
	setMatrix( mat );
	ev->accept();
    }
    else
	QGraphicsView::wheelEvent( ev );
}


uiGraphicsViewBase::uiGraphicsViewBase( uiParent* p, const char* nm )
    : uiObject( p, nm, mkbody(p,nm) )
    , reDrawNeeded(this)
    , reSize(this)
    , rubberBandUsed(this)
    , activatedone(this)
    , scene_(0)
    , selectedarea_(0)
    , enabscrollzoom_(true)
{
    setScene( *new uiGraphicsScene(nm) );
    setDragMode( uiGraphicsViewBase::NoDrag );
    getMouseEventHandler().buttonReleased.notify(
	    mCB(this,uiGraphicsViewBase,rubberBandCB) );
}


uiGraphicsViewBody& uiGraphicsViewBase::mkbody( uiParent* p, const char* nm )
{
    body_ = new uiGraphicsViewBody( *this, p, nm );
    return *body_;
}


uiGraphicsViewBase::~uiGraphicsViewBase()
{
    delete body_;
}


MouseEventHandler& uiGraphicsViewBase::getMouseEventHandler()
{
    return body_->mouseEventHandler();
}


void uiGraphicsViewBase::rePaintRect( const uiRect* rect )
{
    body_->repaint();
}


void uiGraphicsViewBase::setDragMode( ODDragMode dragmode )
{
    body_->setDragMode( (QGraphicsView::DragMode)int(dragmode) );
    if ( dragmode == uiGraphicsViewBase::RubberBandDrag )
	scene().setMouseEventActive( false );
    else
	scene().setMouseEventActive( true );
}


uiGraphicsViewBase::ODDragMode uiGraphicsViewBase::dragMode() const
{ return (ODDragMode)int(body_->dragMode()); }


bool uiGraphicsViewBase::isRubberBandingOn() const
{ return dragMode() == uiGraphicsViewBase::RubberBandDrag; }


void uiGraphicsViewBase::rubberBandCB( CallBacker* )
{
    if ( !isRubberBandingOn() )
	return;

    selectedarea_ = new uiRect( scene().getSelectedArea() );
    rubberBandUsed.trigger();
}


void uiGraphicsViewBase::setMouseTracking( bool yn )
{ body_->setMouseTracking( yn ); }


bool uiGraphicsViewBase::hasMouseTracking() const
{ return body_->hasMouseTracking(); }


int uiGraphicsViewBase::width() const
{
#if defined(__win__) && !defined(__msvc__)
    const int prefwidth = prefHNrPics();
    const int viewwidth = getViewArea().width();
    return prefwidth > viewwidth ? prefwidth : viewwidth;
#else
    return body_->width();
#endif
}


int uiGraphicsViewBase::height() const
{
#if defined(__win__) && !defined(__msvc__)
    const int prefheight = prefVNrPics();
    const int viewheight = getViewArea().height();
    return prefheight > viewheight ? prefheight : viewheight;
#else
    return body_->height();
#endif
}


void uiGraphicsViewBase::setScrollBarPolicy( bool hor, ScrollBarPolicy sbp )
{
    if ( hor )
	body_->setHorizontalScrollBarPolicy( (Qt::ScrollBarPolicy)int(sbp) );
    else
	body_->setVerticalScrollBarPolicy( (Qt::ScrollBarPolicy)int(sbp) );
}


void uiGraphicsViewBase::setViewArea( double x, double y, double w, double h )
{ body_->setSceneRect( x, y, w, h ); }


uiRect uiGraphicsViewBase::getViewArea() const
{
    QRectF qselrect( body_->sceneRect() );
    return uiRect( (int)qselrect.left(), (int)qselrect.top(),
	    	   (int)qselrect.right(), (int)qselrect.bottom() );
}


void uiGraphicsViewBase::setScene( uiGraphicsScene& scene )
{
    scene_ = &scene;
    scene_->setSceneRect( sBorder, sBorder,
			  width()-2*sBorder, height()-2*sBorder );
    body_->setScene( scene.qGraphicsScene() );
}


uiGraphicsScene& uiGraphicsViewBase::scene()
{
    return *scene_;
}


uiPoint uiGraphicsViewBase::getCursorPos() const
{
    return getScenePos( body_->cursor().pos().x(), body_->cursor().pos().y() );
}


uiPoint uiGraphicsViewBase::getScenePos( float x, float y ) const
{
    QPoint globalpos( (int)x, (int)y );
    QPoint viewpos( (int)body_->mapFromGlobal(globalpos).x(),
		    (int)body_->mapFromGlobal(globalpos).y() );
    return uiPoint( (int)body_->mapToScene(viewpos).x(),
	    	    (int)body_->mapToScene(viewpos).y() );
}


void uiGraphicsViewBase::show()
{ body_->show(); }


void uiGraphicsViewBase::setBackgroundColor( const Color& color )
{
    QBrush brush( QColor(color.r(),color.g(),color.b()) );
    body_->setBackgroundBrush( brush );
}


Color uiGraphicsViewBase::backgroundColor() const
{
    QColor color( body_->backgroundBrush().color() );
    return Color( color.red(), color.green(), color.blue() );
}


void uiGraphicsViewBase::activateMenu()
{ body_->activateMenu(); }

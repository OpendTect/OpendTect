/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________

-*/


#include "uigraphicsviewbase.h"

#include "draw.h"
#include "mouseevent.h"
#include "settingsaccess.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uimouseeventblockerbygesture.h"
#include "uiobjbodyimpl.h"

#include <QApplication>
#include <QGesture>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QPrinter>
#include <QPrintDialog>
#include <QScrollBar>
#include <QTouchEvent>
#include <QWheelEvent>

mUseQtnamespace

static const int cDefaultWidth  = 1;
static const int cDefaultHeight = 1;

static ObjectSet<uiGraphicsViewBase> allviewers;


class TouchSceneEventFilter : public QObject
{
public:

bool eventFilter(QObject* obj, QEvent* ev )
{
    if ( ev->type()==QEvent::TouchBegin ||
	ev->type()==QEvent::TouchUpdate ||
	ev->type()==QEvent::TouchEnd )
    {
	ev->accept();
	return true;
    }

    return false;
}

};


class uiGraphicsViewBody :
    public uiObjBodyImpl<uiGraphicsViewBase,QGraphicsView>
{
public:

uiGraphicsViewBody( uiGraphicsViewBase& hndle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiGraphicsViewBase,QGraphicsView>(hndle,p,nm)
    , mousehandler_(*new MouseEventHandler)
    , keyboardhandler_(*new KeyboardEventHandler)
    , gestureeventhandler_(*new GestureEventHandler)
    , startpos_(-1,-1)
    , handle_(hndle)
    , currentpinchscale_(0)
    , mouseeventblocker_(*new uiMouseEventBlockerByGestures(1000))
    , reversemousewheel_(false)
    , midmousebutfordrag_(true)
{
    Settings::common().getYN( SettingsAccess::sKeyMouseWheelReversal(),
			      reversemousewheel_ );

    mouseeventblocker_.attachToQObj( this );
    setStretch( 2, 2 );
    setPrefWidth( cDefaultWidth );
    setPrefHeight( cDefaultHeight );
    setTransformationAnchor( QGraphicsView::AnchorUnderMouse );
    viewport()->setAttribute( Qt::WA_AcceptTouchEvents );
    grabGesture( Qt::PinchGesture );
}

~uiGraphicsViewBody()
{
    delete &mousehandler_;
    delete &keyboardhandler_;
    delete &gestureeventhandler_;
    delete &mouseeventblocker_;
}

MouseEventHandler& mouseEventHandler()
{ return mousehandler_; }

KeyboardEventHandler& keyboardEventHandler()
{ return keyboardhandler_; }

GestureEventHandler& gestureEventHandler()
{ return gestureeventhandler_; }

void setMouseWheelReversal(bool yn)	{ reversemousewheel_ = yn; }
bool getMouseWheelReversal() const	{ return reversemousewheel_; }

void setMidMouseButtonForDrag( bool yn ) { midmousebutfordrag_ = yn; }
bool hasMidMouseButtonForDrag() const	{ return midmousebutfordrag_; }

int viewWidth() const			{ return viewport()->width(); }
int viewHeight() const			{ return viewport()->height(); }

const uiPoint& getStartPos() const	{ return startpos_; }

protected:

    uiPoint			startpos_;
    OD::ButtonState		buttonstate_;
    MouseEventHandler&		mousehandler_;
    KeyboardEventHandler&	keyboardhandler_;
    GestureEventHandler&	gestureeventhandler_;
    uiGraphicsViewBase&		handle_;
    float			currentpinchscale_;
    bool			reversemousewheel_;
    bool			midmousebutfordrag_;

    uiMouseEventBlockerByGestures&   mouseeventblocker_;

    void			wheelEvent(QWheelEvent*);
    void			resizeEvent(QResizeEvent*);
    void			paintEvent(QPaintEvent*);
    void			mouseMoveEvent(QMouseEvent*);
    void			mouseReleaseEvent(QMouseEvent*);
    void			mousePressEvent(QMouseEvent*);
    void			mouseDoubleClickEvent(QMouseEvent*);
    void			keyPressEvent(QKeyEvent*);
    void			keyReleaseEvent(QKeyEvent*);
    bool			viewportEvent(QEvent*);
    void			scrollContentsBy (int,int);
    bool			event(QEvent*);
    bool			gestureEvent(QGestureEvent*);
};


void uiGraphicsViewBody::mouseMoveEvent( QMouseEvent* ev )
{
    MouseEvent me( buttonstate_, ev->pos().x(), ev->pos().y() );
    mousehandler_.triggerMovement( me );
    QGraphicsView::mouseMoveEvent( ev );
}


bool uiGraphicsViewBody::viewportEvent( QEvent* ev )
{
    if ( mouseeventblocker_.updateFromEvent(ev) )
    {
	ev->accept();
	return true;
    }
    else
    {
	return QGraphicsView::viewportEvent( ev );
    }
}


void uiGraphicsViewBody::mousePressEvent( QMouseEvent* ev )
{
    if ( ev->modifiers() == Qt::ControlModifier )
	handle_.setCtrlPressed( true );

    const Qt::MouseButtons qtmbs = ev->buttons();
    const bool leftrightbut = qtmbs&Qt::RightButton && qtmbs&Qt::LeftButton;
    if ( leftrightbut || ev->button() == Qt::MiddleButton )
    {
	uiPoint viewpt = handle_.getScenePos( ev->pos().x(), ev->pos().y() );
	startpos_ = uiPoint( viewpt.x, viewpt.y );
	buttonstate_ = OD::MidButton;
	MouseEvent me( buttonstate_, ev->pos().x(), ev->pos().y() );
	mousehandler_.triggerButtonPressed( me );

	if ( midmousebutfordrag_ )
	{
	    setDragMode( ScrollHandDrag );
	    QMouseEvent fake( ev->type(), ev->pos(), Qt::LeftButton,
			      Qt::LeftButton, ev->modifiers() );
	    QGraphicsView::mousePressEvent( &fake );
	}
    }
    else if ( ev->button() == Qt::RightButton )
    {
	QGraphicsView::DragMode dragmode = dragMode();
	setDragMode( QGraphicsView::NoDrag );

	buttonstate_ = OD::RightButton;
	MouseEvent me( buttonstate_, ev->pos().x(), ev->pos().y() );
	const int refnr = handle_.beginCmdRecEvent( "rightButtonPressed" );
	mousehandler_.triggerButtonPressed( me );
	QGraphicsView::mousePressEvent( ev );
	handle_.endCmdRecEvent( refnr, "rightButtonPressed" );
	setDragMode( dragmode );
	return;
    }
    else if ( ev->button() == Qt::LeftButton )
    {
	uiPoint viewpt = handle_.getScenePos( ev->pos().x(), ev->pos().y() );
	startpos_ = uiPoint( viewpt.x, viewpt.y );
	buttonstate_ = OD::LeftButton;
	MouseEvent me( buttonstate_, ev->pos().x(), ev->pos().y() );
	mousehandler_.triggerButtonPressed( me );
    }
    else
	buttonstate_ = OD::NoButton;

    QGraphicsView::mousePressEvent( ev );
}


void uiGraphicsViewBody::mouseDoubleClickEvent( QMouseEvent* ev )
{
    if ( !ev | handle_.isRubberBandingOn() )
	return;

    if ( ev->button() == Qt::LeftButton )
    {
	MouseEvent me( OD::LeftButton, ev->pos().x(), ev->pos().y() );
	mousehandler_.triggerDoubleClick( me );
    }
    QGraphicsView::mouseDoubleClickEvent( ev );
}


void uiGraphicsViewBody::mouseReleaseEvent( QMouseEvent* ev )
{
    if ( ev->button() == Qt::LeftButton )
    {
	buttonstate_ = OD::LeftButton;
	MouseEvent me( buttonstate_, ev->pos().x(), ev->pos().y() );
	mousehandler_.triggerButtonReleased( me );
	if ( handle_.isRubberBandingOn() )
	{
	    uiPoint viewpt = handle_.getScenePos( ev->pos().x(), ev->pos().y());
	    uiPoint stoppos( viewpt.x, viewpt.y );
	    uiRect selrect( startpos_, stoppos );
	    handle_.scene().setSelectionArea( selrect );
	}
    }
    else if ( ev->button() == Qt::MiddleButton )
    {
	buttonstate_ = OD::MidButton;
	MouseEvent me( buttonstate_, ev->pos().x(), ev->pos().y() );
	mousehandler_.triggerButtonReleased( me );

	if ( midmousebutfordrag_ )
	    setDragMode( NoDrag );
    }

    handle_.setCtrlPressed( false );

    buttonstate_ = OD::NoButton;
    QGraphicsView::mouseReleaseEvent( ev );
}


void uiGraphicsViewBody::keyPressEvent( QKeyEvent* ev )
{
    if ( !ev ) return;

    QGraphicsItem* itm = scene()->focusItem();
    if ( !itm )
    {
	// TODO: impl modifier
	KeyboardEvent ke;
	ke.key_ = (OD::KeyboardKey)ev->key();
	ke.modifier_ = OD::ButtonState( (int)ev->modifiers() );
	keyboardhandler_.triggerKeyPressed( ke );
    }

    QGraphicsView::keyPressEvent( ev );
}


void uiGraphicsViewBody::keyReleaseEvent( QKeyEvent* ev )
{
    if ( !ev ) return;

    QGraphicsItem* itm = scene()->focusItem();
    if ( !itm )
    {
	// TODO: impl modifier
	KeyboardEvent ke;
	ke.key_ = (OD::KeyboardKey)ev->key();
	ke.modifier_ = OD::ButtonState( (int)ev->modifiers() );
	keyboardhandler_.triggerKeyReleased( ke );
    }

    QGraphicsView::keyReleaseEvent( ev );
}


void uiGraphicsViewBody::paintEvent( QPaintEvent* ev )
{
    handle_.preDraw.trigger();
    QGraphicsView::paintEvent( ev );
}


void uiGraphicsViewBody::resizeEvent( QResizeEvent* ev )
{
    if ( handle_.scene_ )
    {
	const int sceneborder = handle_.getSceneBorder();
#if defined(__win__) && !defined(__msvc__)
	QSize newsz = ev->size();
	handle_.scene_->setSceneRect( sceneborder,sceneborder,
				      newsz.width()-2*sceneborder,
				      newsz.height()-2*sceneborder );
#else
	handle_.scene_->setSceneRect( sceneborder, sceneborder,
				      viewWidth()-2*sceneborder,
				      viewHeight()-2*sceneborder );
#endif
    }

    uiSize oldsize( ev->oldSize().width(), ev->oldSize().height() );
    handle_.reSize.trigger( oldsize );
    handle_.reDrawn.trigger();
}


static QPointF getWheelPosition( const QWheelEvent& ev )
{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
    return ev.position();
#else
    return ev.posF();
#endif
}


void uiGraphicsViewBody::wheelEvent( QWheelEvent* ev )
{
    if ( !ev )
	return QGraphicsView::wheelEvent( ev );

    const QPoint delta = reversemousewheel_ ? -ev->angleDelta()
					    : ev->angleDelta();
    if ( handle_.scrollZoomEnabled() )
    {
	const bool isvertical = abs(delta.y()) > abs(delta.x());
	const bool zoomin = isvertical && delta.y()>0;
	const QPoint numsteps = delta / 8 / 15;

	QTransform qtrans = transform();
	const QPointF mousepos = getWheelPosition( *ev );
	qtrans.translate( (viewWidth()/2) - mousepos.x(),
			 (viewHeight()/2) - mousepos.y() );

	const double m11 = qtrans.m11();
	const double m22 = qtrans.m22();
	const int inumsteps = numsteps.manhattanLength();
	for ( int idx=0; idx<inumsteps; idx++ )
	{
	    if ( zoomin || (m11-mDefEps>1 && m22-mDefEps>1) )
	    {
		if ( zoomin )
		    qtrans.scale( 1.2, 1.2 );
		else
		    qtrans.scale( 1./1.2, 1./1.2 );
	    }
	}

	qtrans.translate( mousepos.x() - (viewWidth()/2),
			  mousepos.y() - (viewHeight()/2) );
	setTransform( qtrans );
	ev->accept();
    }

    const QPointF mousepos = getWheelPosition( *ev );
    MouseEvent me( OD::ButtonState(ev->modifiers() | ev->buttons()),
		   mousepos.x(), mousepos.y(), delta.y() );
    mousehandler_.triggerWheel( me );
/*
  uncomment this conditional to have the default wheel event behaviour, that is,
  when scrolling up, scene moves down. When scrolling down, scene moves up.
*/
//    if ( !handle_.scrollZoomEnabled() )
//	QGraphicsView::wheelEvent( ev );
}


void uiGraphicsViewBody::scrollContentsBy( int dx, int dy )
{
    QGraphicsView::scrollContentsBy(dx,dy);
    handle_.scrollBarUsed.trigger();
}


bool uiGraphicsViewBody::event( QEvent* ev )
{
    if ( !ev )
	return false;

    if ( ev->type() == QEvent::Gesture )
         return gestureEvent( static_cast<QGestureEvent*>( ev ) );

    return QGraphicsView::event( ev );
}


bool uiGraphicsViewBody::gestureEvent( QGestureEvent* ev )
{
    if ( QPinchGesture* pinch =
	    static_cast<QPinchGesture*>(ev->gesture(Qt::PinchGesture)) )
    {
	const QPointF qcenter = pinch->centerPoint();
	const QPoint pinchcenter = qwidget()->mapFromGlobal(qcenter.toPoint());
	GestureEvent gevent( mCast(int,pinchcenter.x()),
			     mCast(int,pinchcenter.y()),
			     mCast(float,pinch->scaleFactor()),
			     mCast(float,pinch->rotationAngle()));

	if ( pinch->state() == Qt::GestureStarted )
	    gevent.setState( GestureEvent::Started );
	else if ( pinch->state() == Qt::GestureUpdated )
	{
	    gevent.setState( GestureEvent::Moving );
	    if ( mIsEqual(gevent.scale(),1.0f,mDefEps) )
		return false;

	    if ( mIsEqual(currentpinchscale_,gevent.scale(),mDefEps) )
		return false;

	    currentpinchscale_ = gevent.scale();
	}
	else
	    gevent.setState( GestureEvent::Finished );

	gestureeventhandler_.triggerPinchEvent( gevent );
	ev->accept();
	return true;
    }

    ev->accept();
    return false;
}


uiGraphicsViewBase::uiGraphicsViewBase( uiParent* p, const char* nm )
    : uiObject( p, nm, mkbody(p,nm) )
    , reDrawNeeded(this)
    , reSize(this)
    , reDrawn(this)
    , preDraw(this)
    , rubberBandUsed(this)
    , scrollBarUsed(this)
    , scene_(0)
    , selectedarea_(0)
    , sceneborder_(0)
    , enabscrollzoom_(false)
    , isctrlpressed_(false)
{
    enableScrollZoom( enabscrollzoom_ );
    setScene( *new uiGraphicsScene(nm) );
    setDragMode( uiGraphicsViewBase::NoDrag );
    getMouseEventHandler().buttonReleased.notify(
	    mCB(this,uiGraphicsViewBase,rubberBandCB) );

    allviewers += this;
}


uiGraphicsViewBody& uiGraphicsViewBase::mkbody( uiParent* p, const char* nm )
{
    body_ = new uiGraphicsViewBody( *this, p, nm );
    return *body_;
}


uiGraphicsViewBase::~uiGraphicsViewBase()
{
    allviewers -= this;
    delete body_;
    delete scene_;
}


MouseEventHandler& uiGraphicsViewBase::getNavigationMouseEventHandler()
{ return body_->mouseEventHandler(); }

MouseEventHandler& uiGraphicsViewBase::getMouseEventHandler()
{ return scene_->getMouseEventHandler(); }

KeyboardEventHandler& uiGraphicsViewBase::getKeyboardEventHandler()
{ return body_->keyboardEventHandler(); }

GestureEventHandler& uiGraphicsViewBase::gestureEventHandler()
{ return body_->gestureEventHandler(); }

void uiGraphicsViewBase::rePaint()
{ body_->viewport()->repaint(); }


const ObjectSet<uiGraphicsViewBase>& uiGraphicsViewBase::allInstances()
{ return allviewers; }


void uiGraphicsViewBase::enableScrollZoom( bool yn )
{
    enabscrollzoom_ = yn;
    setScrollBarPolicy( true, enabscrollzoom_
				? uiGraphicsViewBase::ScrollBarAsNeeded
				: uiGraphicsViewBase::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, enabscrollzoom_
				? uiGraphicsViewBase::ScrollBarAsNeeded
				: uiGraphicsViewBase::ScrollBarAlwaysOff );
}


void uiGraphicsViewBase::disableScrollZoom()
{ enableScrollZoom(false); }


void uiGraphicsViewBase::setDragMode( ODDragMode dragmode )
{
    body_->setDragMode( (QGraphicsView::DragMode)int(dragmode) );
}


uiGraphicsViewBase::ODDragMode uiGraphicsViewBase::dragMode() const
{ return (ODDragMode)int(body_->dragMode()); }


bool uiGraphicsViewBase::isRubberBandingOn() const
{ return dragMode() == uiGraphicsViewBase::RubberBandDrag; }


void uiGraphicsViewBase::rubberBandCB( CallBacker* )
{
    if ( !isRubberBandingOn() )
	return;

    const MouseEvent& ev = getMouseEventHandler().event();
    if ( ev.rightButton() || ev.middleButton() )
	return;

    selectedarea_ = new uiRect( body_->getStartPos(), getCursorPos() );
    rubberBandUsed.trigger();
}


void uiGraphicsViewBase::setMouseTracking( bool yn )
{ body_->setMouseTracking( yn ); }

bool uiGraphicsViewBase::hasMouseTracking() const
{ return body_->hasMouseTracking(); }

void uiGraphicsViewBase::setMouseWheelReversal( bool yn )
{ body_->setMouseWheelReversal( yn ); }

bool uiGraphicsViewBase::getMouseWheelReversal() const
{ return body_->getMouseWheelReversal(); }

void uiGraphicsViewBase::setMidMouseButtonForDrag( bool yn )
{ body_->setMidMouseButtonForDrag( yn ); }

bool uiGraphicsViewBase::hasMidMouseButtonForDrag() const
{ return body_->hasMidMouseButtonForDrag(); }


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


int uiGraphicsViewBase::viewWidth() const
{
    return body_->viewWidth();
}


int uiGraphicsViewBase::viewHeight() const
{
    return body_->viewHeight();
}


void uiGraphicsViewBase::centreOn( uiPoint centre )
{ body_->centerOn( centre.x, centre.y ); }


void uiGraphicsViewBase::setScrollBarPolicy( bool hor, ScrollBarPolicy sbp )
{
    if ( hor )
	body_->setHorizontalScrollBarPolicy(
				      (Qt::ScrollBarPolicy)int(sbp) );
    else
	body_->setVerticalScrollBarPolicy(
				      (Qt::ScrollBarPolicy)int(sbp) );
}


void uiGraphicsViewBase::setViewArea( double x, double y, double w, double h )
{ body_->setSceneRect( x, y, w, h ); }


uiRect uiGraphicsViewBase::getViewArea() const
{
    QRectF qselrect( body_->mapToScene(0,0),
		     body_->mapToScene(viewWidth(),viewHeight()) );
    return uiRect( (int)qselrect.left(), (int)qselrect.top(),
		   (int)qselrect.right(), (int)qselrect.bottom() );
}


void uiGraphicsViewBase::setScene( uiGraphicsScene& scn )
{
    if ( scene_ ) delete scene_;
    scene_ = &scn;
    scene_->setSceneRect( sceneborder_, sceneborder_,
			  viewWidth()-2*sceneborder_,
			  viewHeight()-2*sceneborder_ );
    scn.qGraphicsScene()->installEventFilter( new TouchSceneEventFilter );
    body_->setScene( scn.qGraphicsScene() );
}


uiGraphicsScene& uiGraphicsViewBase::scene()
{
    return *scene_;
}


uiRect uiGraphicsViewBase::getSceneRect() const
{
    QRectF scenerect = body_->sceneRect();
    return uiRect( (int)scenerect.left(), (int)scenerect.top(),
		   (int)scenerect.right(), (int)scenerect.bottom() );
}


void uiGraphicsViewBase::setSceneRect( const uiRect& rect )
{ body_->setSceneRect( rect.left(), rect.top(), rect.width(), rect.height() ); }


uiPoint uiGraphicsViewBase::getCursorPos() const
{
    QPoint globalpos( body_->cursor().pos().x(),
				body_->cursor().pos().y() );
    QPoint viewpos( (int)body_->mapFromGlobal(globalpos).x(),
			      (int)body_->mapFromGlobal(globalpos).y() );
    return getScenePos( (float)viewpos.x(), (float)viewpos.y() );
}


void uiGraphicsViewBase::setScaleFactor( float scalex, float scaley )
{
    body_->setTransform( QTransform::fromScale(scalex,scaley) );
}


void uiGraphicsViewBase::getScaleFactor( float& scalex, float& scaley ) const
{
    scalex = body_->transform().m11();
    scaley = body_->transform().m22();
}


uiPoint uiGraphicsViewBase::getScenePos( float x, float y ) const
{
    QPoint viewpos( (int)x, (int)y );
    return uiPoint( (int)body_->mapToScene(viewpos).x(),
		    (int)body_->mapToScene(viewpos).y() );
}

const uiPoint& uiGraphicsViewBase::getStartPos() const
{ return body_->getStartPos(); }


void uiGraphicsViewBase::show()
{ body_->show(); }


void uiGraphicsViewBase::setBackgroundColor( const OD::Color& color )
{
    QBrush brush( QColor(color.r(),color.g(),color.b(),255-color.t()) );
    body_->setBackgroundBrush( brush );
}


OD::Color uiGraphicsViewBase::backgroundColor() const
{
    QColor qcol( body_->backgroundBrush().color() );
    return OD::Color( qcol.red(), qcol.green(), qcol.blue(), 255-qcol.alpha() );
}


void uiGraphicsViewBase::setNoBackGround()
{
    OD::Color col = backgroundColor();
    col.setTransparency( 255 );
    setBackgroundColor( col );
    scene_->setBackGroundColor( col );

    QPalette qpal( qwidget()->palette() );
    qpal.setColor( QPalette::Base, QColor(col.r(),col.g(),col.b(),0) );
    body_->setPalette( qpal );
    scene_->qGraphicsScene()->setPalette( qpal );
}


void uiGraphicsViewBase::setSceneAlignment( const Alignment& al )
{
    Qt::Alignment qal;
    if ( al.vPos() == Alignment::Top )
	qal = Qt::AlignTop;
    else if ( al.vPos() == Alignment::Bottom )
	qal = Qt::AlignBottom;
    else
	qal = Qt::AlignVCenter;

    if ( al.hPos() == Alignment::Left )
	qal = qal | Qt::AlignLeft;
    else if ( al.hPos() == Alignment::Right )
	qal = qal | Qt::AlignRight;
    else
	qal = qal | Qt::AlignHCenter;

    body_->setAlignment( qal );
}


void uiGraphicsViewBase::setSceneBorder( int border )
{
    sceneborder_ = border;
}


int uiGraphicsViewBase::getSceneBorder() const
{
    return sceneborder_;
}


uiSize uiGraphicsViewBase::scrollBarSize( bool hor ) const
{
    const QScrollBar* sb = hor ? body_->horizontalScrollBar()
			       : body_->verticalScrollBar();
    return sb ? uiSize( (int)sb->sizeHint().width(),
			(int)sb->sizeHint().height())
	      : uiSize(0,0);
}


uiPoint uiGraphicsViewBase::mapFromScene( const Geom::Point2D<float>& pt ) const
{
    QPoint qp = body_->mapFromScene( pt.x, pt.y );
    return uiPoint( qp.x(), qp.y() );

}


Geom::Point2D<float> uiGraphicsViewBase::mapToScene( const uiPoint& pt ) const
{
    QPointF qp = body_->mapToScene( pt.x, pt.y );
    return Geom::Point2D<float>( qp.x(), qp.y() );

}


Geom::RectF uiGraphicsViewBase::mapToScene( const uiRect& r ) const
{
    const Geom::Point2D<float> tlpt = mapToScene( r.topLeft() );
    const Geom::Point2D<float> rbpt = mapToScene( r.bottomRight() );
    return Geom::RectF( tlpt, rbpt );
}


void uiGraphicsViewBase::translateText()
{
    uiObject::translateText();
    scene_->translateText();
}


bool uiGraphicsViewBase::print()
{
    QPrinter printer;
    QPrintDialog printdlg( &printer );
    if ( printdlg.exec() == QDialog::Rejected )
	return false;

    QPainter painter( &printer );
    painter.setRenderHint( QPainter::Antialiasing );
    body_->render( &painter );
    return true;
}


// uiGraphicsViewMask
uiGraphicsViewMask::uiGraphicsViewMask( uiGraphicsViewBase& vw )
    : uiGraphicsItem()
    , view_(vw)
    , border_(100)
{
    OD::LineStyle lst; lst.type_ = OD::LineStyle::None;
    topmask_ = new uiRectItem(); addChild( topmask_ );
    topmask_->setPenStyle( lst );
    bottommask_ = new uiRectItem(); addChild( bottommask_ );
    bottommask_->setPenStyle( lst );
    leftmask_ = new uiRectItem(); addChild( leftmask_ );
    leftmask_->setPenStyle( lst );
    rightmask_ = new uiRectItem(); addChild( rightmask_ );
    rightmask_->setPenStyle( lst );
    setMaskColor( OD::Color::White() );
}


uiGraphicsViewMask::~uiGraphicsViewMask()
{}


void uiGraphicsViewMask::setBorder( int b )
{
    border_ = b;
}


void uiGraphicsViewMask::setMaskColor( const OD::Color& col )
{
    maskcolor_ = col;
    topmask_->setFillColor( col );
    bottommask_->setFillColor( col );
    leftmask_->setFillColor( col );
    rightmask_->setFillColor( col );
}


void uiGraphicsViewMask::update()
{
    const uiRect vwrect( 0, 0, view_.viewWidth(), view_.viewHeight() );

    uiRect mask = vwrect; mask.setBottom( mask.top()+border_ );
    const Geom::RectF topmaskrect = view_.mapToScene(mask);
    topmask_->setRect( topmaskrect );

    mask = vwrect; mask.setTop( mask.bottom()-border_ );
    const Geom::RectF bottommaskrect = view_.mapToScene(mask);
    bottommask_->setRect( bottommaskrect );

    mask = vwrect; mask.setRight( mask.left()+border_ );
    const Geom::RectF leftmaskrect = view_.mapToScene(mask);
    leftmask_->setRect( leftmaskrect );

    mask = vwrect; mask.setLeft( mask.right()-border_ );
    const Geom::RectF rightmaskrect = view_.mapToScene(mask);
    rightmask_->setRect( rightmaskrect );
}

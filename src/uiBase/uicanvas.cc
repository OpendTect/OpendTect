/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.cc,v 1.43 2008-09-08 12:31:06 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uicanvas.h"
#include "errh.h"
#include "uiobj.h"
#include "i_uidrwbody.h"

#include "iodrawtool.h"
#include "mouseevent.h"

#include <QApplication>
#include <QEvent>
#include <Q3ScrollView>
#include <QPainter>
#include <QRubberBand>


#define mButState( e ) ( e->state() | e->button() )

class uiScrollViewBody;

static const int sDefaultWidth  = 1;
static const int sDefaultHeight = 1;


class uiCanvasBody : public uiDrawableObjBody<uiCanvas,QFrame>
{

public:
uiCanvasBody( uiCanvas& handle, uiParent* p, const char *nm="uiCanvasBody")
    : uiDrawableObjBody<uiCanvas,QFrame>( handle, p, nm ) 
    , handle_( handle )
{
    setStretch( 2, 2 );
    setPrefWidth( sDefaultWidth );
    setPrefHeight( sDefaultHeight );
    setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
}

    virtual		~uiCanvasBody()		{}
    void		updateCanvas()		{ QWidget::update(); }

    void		activateMenu();
    bool		event(QEvent*);

private:
    uiCanvas&		handle_;
};


static const QEvent::Type sQEventActMenu = (QEvent::Type) (QEvent::User+0);

void uiCanvasBody::activateMenu()
{
    QEvent* actevent = new QEvent( sQEventActMenu );
    QApplication::postEvent( this, actevent );
}


bool uiCanvasBody::event( QEvent* ev )
{
    if ( ev->type() == sQEventActMenu )
    {
	const MouseEvent right( OD::RightButton );
	handle_.getMouseEventHandler().triggerButtonPressed( right ); 
    }
    else
	return QFrame::event( ev );

    handle_.activatedone.trigger();
    return true;
}


//! Derived class from QScrollView in order to handle 'n relay Qt's events
/*
    defined locally in .cc file.
*/
class uiScrollViewBody : public uiDrawableObjBody<uiScrollView,Q3ScrollView>
{
public:

			uiScrollViewBody( uiScrollView& handle,
					  uiParent* p=0,
					  const char *nm="uiScrollViewBody" )
			    : uiDrawableObjBody<uiScrollView,Q3ScrollView> 
				( handle, p, nm )
			    , rubberstate( OD::NoButton )
			    , qrubber( 0 )
			    , aspectrat( 0.0 ), rbidx( 0 ) 
			    {
				setStretch( 2, 2 );
				setPrefContentsWidth( sDefaultWidth );
				setPrefContentsHeight( sDefaultHeight );
			    }

    OD::ButtonState 	rubberstate;
    float		aspectrat;

    virtual QPaintDevice* mQPaintDevice()	{ return viewport(); }

    uiRect		visibleArea() const;

    void		setPrefContentsWidth( int w )      
			{ setPrefWidth( w + 2*frameWidth() ); }

    void		setPrefContentsHeight( int h )     
			{ setPrefHeight( h + 2*frameWidth() ); }

    virtual uiSize	actualsize( bool include_border = true) const
			{
			    uiSize sz= uiObjectBody::actualsize(include_border);
			    if ( include_border ) return sz;

			    int fw = frameWidth();
			    int w = sz.hNrPics()-2*fw;
			    if ( w<1 ) w=1;
			    int h = sz.vNrPics()-2*fw;
			    if ( h<1 ) h=1;
			    return uiSize( w, h );
			}


    void		setRubberBandingOn(OD::ButtonState b)
    					{ rubberstate = b; }
    OD::ButtonState	rubberBandingOn() const		{ return rubberstate; }
    void		setAspectRatio(float r)		{ aspectrat = r; }
    float		aspectRatio()			{ return aspectrat; }

    virtual void	reDraw( bool deep )		{ updateContents(); }
    void		updateCanvas()		{ Q3ScrollView::update(); }
protected:

    virtual void	drawContents( QPainter* ptr )
			{
			    const QRect qr = contentsRect();
			    drawContents( ptr, qr.left(), qr.top(),
				          qr.right(), qr.bottom() );
			}
    virtual void	drawContents ( QPainter * p, int clipx,
			    int clipy, int clipw, int cliph );
    virtual void	resizeEvent( QResizeEvent * );


    //! over-ride DrawableObjBody impl, because we want to use drawContents()
    virtual void	paintEvent( QPaintEvent* ev )
			{ Q3ScrollView::paintEvent(ev); }

    virtual void	contentsMousePressEvent(QMouseEvent*);
    virtual void	contentsMouseMoveEvent(QMouseEvent*);
    virtual void	contentsMouseReleaseEvent(QMouseEvent*);
    virtual void	contentsMouseDoubleClickEvent(QMouseEvent*);
    virtual void	contentsWheelEvent(QWheelEvent*);

    uiRect		rubber;
    int			rbidx;
    QRubberBand*	qrubber;

};


uiRect uiScrollViewBody::visibleArea() const
{

    uiSize vpSize = actualsize(false);

    uiPoint tl( contentsX(), contentsY() );
    return uiRect( contentsX(), contentsY(),
	    mMAX( visibleWidth(), vpSize.hNrPics()-1),
	    mMAX( visibleHeight(), vpSize.vNrPics())-1 );
}


void uiScrollViewBody::drawContents( QPainter * p, int clipx,
				     int clipy, int clipw, int cliph )
{
    drawTool().setActivePainter( p );
    handlePaintEvent( uiRect(clipx,clipy,clipw-1,cliph-1) );
}


void uiScrollViewBody::resizeEvent( QResizeEvent *QREv )
{
    const QSize& os = QREv->oldSize();
    uiSize oldSize( os.width(), os.height() );

    const QSize& ns = QREv->size();
    uiSize nwSize( ns.width()  - 2 * frameWidth(), 
                   ns.height() - 2 * frameWidth() );

    handleResizeEvent( QREv, oldSize, nwSize );

    viewport()->update();
}


void uiScrollViewBody::contentsMousePressEvent ( QMouseEvent * e )
{
    if ( mButState( e ) == rubberstate )
    {
	rbidx = 0;
	qrubber = new QRubberBand( QRubberBand::Rectangle, viewport() );
	rubber.setRight( e->x() );
	rubber.setLeft( e->x() );
	rubber.setBottom(  e->y() );
	rubber.setTop( e->y() );

	int xvp, yvp;
	contentsToViewport ( rubber.left(), rubber.top(), xvp, yvp );
	qrubber->setGeometry( xvp, yvp, rubber.right()-rubber.left(), 
			      rubber.bottom()-rubber.top() ); 
    }
    else
    {
	OD::ButtonState bSt = ( OD::ButtonState )(  mButState( e ) );
	MouseEvent evt( bSt, e->x(), e->y() );

	handle_.getMouseEventHandler().triggerButtonPressed( evt );
    } 
}


void uiScrollViewBody::contentsMouseMoveEvent( QMouseEvent* e )
{
// TODO: start a timer, so no move-events required to continue scrolling

    if ( qrubber && mButState( e ) == rubberstate )
    {
	int xvp, yvp;
	if ( aspectrat )
	{
	    int xfac = e->x() > rubber.left() ? 1 : -1;
	    int yfac = e->y() > rubber.top() ? 1 : -1;

	    while ( (e->x() - rubber.left()) * xfac > (rbidx+1) ) 
		rbidx++;	

	    while ( (e->y() - rubber.top() ) * yfac > ( aspectrat*(rbidx+1) ) )
		rbidx++;	

	    while ( ( ((e->x() - rubber.left())*xfac) < (rbidx-1) ) && 
		    ( ((e->y()- rubber.top())*yfac) < ( aspectrat*(rbidx-1))) )
		rbidx--;

	    rubber.setHNrPics( rbidx*xfac );
	    rubber.setVNrPics( mNINT(rbidx*aspectrat*yfac) );
	}
	else
	{
	    rubber.setRight( e->x() );
	    rubber.setBottom(  e->y() );
	}

	ensureVisible( rubber.right(), rubber.bottom() );

	contentsToViewport ( rubber.left(), rubber.top(), xvp, yvp );
	qrubber->setGeometry( xvp, yvp, rubber.right()-rubber.left(), 
			      rubber.bottom()-rubber.top() ); 
    }
    else
    {
	OD::ButtonState bSt = ( OD::ButtonState )(  mButState( e ) );
	MouseEvent evt( bSt, e->x(), e->y() );

	handle_.getMouseEventHandler().triggerMovement( evt );
    } 
}


void uiScrollViewBody::contentsMouseReleaseEvent ( QMouseEvent* e )
{
    if ( qrubber && mButState( e ) == rubberstate )
    {
	delete qrubber; qrubber = 0;
	rubber.checkCorners();
	if( rubber.hNrPics() > 10 && rubber.vNrPics() > 10 )
	    handle_.rubberBandHandler( rubber );
    }
    else
    {
	OD::ButtonState bSt = ( OD::ButtonState )(  mButState( e ) );
	MouseEvent evt( bSt, e->x(), e->y() );

	handle_.getMouseEventHandler().triggerButtonReleased( evt );
    } 
}


void uiScrollViewBody::contentsMouseDoubleClickEvent ( QMouseEvent* e )
{
    OD::ButtonState bSt = ( OD::ButtonState )(  mButState( e ) );
    MouseEvent evt( bSt, e->x(), e->y() );
    handle_.getMouseEventHandler().triggerDoubleClick( evt );
}


void uiScrollViewBody::contentsWheelEvent( QWheelEvent* e )
{
    OD::ButtonState bSt = (OD::ButtonState)( e->state() | e->buttons() );
    static const float delta2angle = M_PI / (180 * 8);
    MouseEvent evt( bSt, e->x(), e->y(), e->delta()*delta2angle );
    handle_.getMouseEventHandler().triggerWheel( evt );
}



uiCanvas::uiCanvas( uiParent* p, const Color& col, const char *nm )
    : uiDrawableObj( p,nm, mkbody(p,nm) )
    , activatedone( this )
{
    drawTool().setDrawAreaBackgroundColor( col );
}


uiCanvasBody& uiCanvas::mkbody( uiParent* p,const char* nm)
{
    body_ = new uiCanvasBody( *this,p,nm );
    return *body_;
}


void uiCanvas::update()
{ body_->updateCanvas(); }


void uiCanvas::setMouseTracking( bool yn )
{ body_->setMouseTracking( yn ); }


bool uiCanvas::hasMouseTracking() const
{ return body_->hasMouseTracking(); }


void uiCanvas::setBackgroundColor( const Color& col )
{ drawTool().setDrawAreaBackgroundColor( col ); }


void uiCanvas::activateMenu()
{ body_->activateMenu(); }


uiScrollView::uiScrollView( uiParent* p, const char *nm )
    : uiDrawableObj( p,nm, mkbody(p,nm) )
{}


uiScrollViewBody& uiScrollView::mkbody( uiParent* p, const char* nm )
{
    body_ = new uiScrollViewBody( *this, p, nm );
    return *body_;
}


void uiScrollView::update()
{ body_->updateCanvas(); }


void uiScrollView::setScrollBarMode( ScrollBarMode mode, bool hor )
{
    Q3ScrollView::ScrollBarMode qmode = Q3ScrollView::Auto;
    if ( mode == AlwaysOff )
	qmode = Q3ScrollView::AlwaysOff;
    else if ( mode == AlwaysOn )
	qmode = Q3ScrollView::AlwaysOn;
    if ( hor )
	body_->setHScrollBarMode( qmode );
    else
	body_->setVScrollBarMode( qmode );
}


uiScrollView::ScrollBarMode uiScrollView::getScrollBarMode( bool hor ) const
{
    Q3ScrollView::ScrollBarMode qmode;
    qmode = hor ? body_->hScrollBarMode() : body_->vScrollBarMode();
    if ( qmode == Q3ScrollView::AlwaysOff )
	return AlwaysOff;
    if ( qmode == Q3ScrollView::AlwaysOn )
	return AlwaysOn;
    return Auto;
}


void uiScrollView::updateContents()	
{ body_->viewport()->update(); }


void uiScrollView::updateContents( uiRect area, bool erase )
{ 
    body_->updateContents( area.left(), area.top(),
			   area.hNrPics(), area.vNrPics() );

}

void uiScrollView::resizeContents( int w, int h )
    { body_->resizeContents(w,h); }

void uiScrollView::setContentsPos( uiPoint topLeft )
    { body_->setContentsPos(topLeft.x,topLeft.y); }


uiRect uiScrollView::visibleArea() const
    { return body_->visibleArea(); }


void uiScrollView::setPrefWidth( int w )        
    { body_->setPrefWidth( w + 2*frameWidth() ); }


void uiScrollView::setPrefHeight( int h )        
    { body_->setPrefHeight( h + 2*frameWidth() ); }


void uiScrollView::setMaximumWidth( int w )
    { body_->setMaximumWidth( w ); }


void uiScrollView::setMaximumHeight( int h )
    { body_->setMaximumHeight( h ); }


int  uiScrollView::frameWidth() const
    { return body_->frameWidth(); }


void  uiScrollView::setRubberBandingOn(OD::ButtonState st)
    { body_->setRubberBandingOn(st); }

OD::ButtonState uiScrollView::rubberBandingOn() const
    { return body_->rubberBandingOn(); }

void  uiScrollView::setAspectRatio( float r )
    { body_->setAspectRatio(r); }

float  uiScrollView::aspectRatio()
    { return body_->aspectRatio(); }

void uiScrollView::setMouseTracking( bool yn )
    { body_->viewport()->setMouseTracking(yn); }

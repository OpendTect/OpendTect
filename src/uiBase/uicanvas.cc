/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.cc,v 1.4 2001-05-16 14:57:17 arend Exp $
________________________________________________________________________

-*/

#include <uicanvas.h>
#include <errh.h>
#include <uiobj.h>
#include <uidrawable.h>

#include <i_qtdrawable.h>
#include <i_qobjwrap.h>

#include <qscrollview.h>
#include <qpainter.h>

mTemplTypeDef( i_drwblQObj, QWidget, drwblQCanvas )
mTemplTypeDefT( i_drwblQObj, QScrollView, i_QObjWrapper_QScrollView )

class i_QCanvas : public drwblQCanvas
{
public:
                        i_QCanvas ( uiCanvas& uiWrapper,
                                       uiObject* parent=0,
                                       const char *name= "i_QCanvas" )
                            : drwblQCanvas (uiWrapper, parent, name )
                            , mUiScrollClient( uiWrapper ) {}


    virtual QSize       sizeHint () const;

protected:
    uiCanvas&           mUiScrollClient;
};

//! Derived class from QScrollView in order to handle 'n relay Qt's events
/*
    defined locally in .cc file.
*/
class i_ScrollableCanvas : public i_QObjWrapper_QScrollView
{

    friend class        uiDrawableObj;

protected:

public:
                        i_ScrollableCanvas( uiScrollView& uiWrapper,
                                          uiObject* parnt=0,
                                          const char *nm="i_ScrollableCanvas" )
                            : i_QObjWrapper_QScrollView( uiWrapper, parnt, nm )
                            , mUiScrollClient( uiWrapper )
                            , rubberstate( uiMouseEvent::NoButton )
                            , rbnding( false )
                            , aspectrat( 0.0 ), rbidx( 0 ) {}

//    void                update();
    QSize               sizeHint() const;

    uiMouseEvent::ButtonState rubberstate;
    float               aspectrat;

protected:
    uiScrollView&       mUiScrollClient;


    virtual void        drawContents ( QPainter * p, int clipx,
                            int clipy, int clipw, int cliph );
    virtual void        resizeEvent( QResizeEvent * );
    virtual void        paintEvent( QPaintEvent* ev )
                        { QScrollView::paintEvent(ev); }
    virtual void        contentsMousePressEvent( QMouseEvent * e );
    virtual void        contentsMouseMoveEvent ( QMouseEvent * e );
    virtual void        contentsMouseReleaseEvent ( QMouseEvent * e );
    virtual void        contentsMouseDoubleClickEvent ( QMouseEvent * e );

    uiRect              rubber;
    int                 rbidx;
    bool                rbnding;


};

int uiCanvasABC::defaultWidth  = 600;
int uiCanvasABC::defaultHeight = 400;

#define mButState( e ) ( e->state() | e->button() )

//------ ui (client) widgets impl.  --------

uiCanvasABC::uiCanvasABC( uiParent* parnt, const char *nm )
	: uiDrawableObj( parnt, nm ) 
{
    setStretch( 2, 2 );
    setPrefWidth( defaultWidth );
    setPrefHeight( defaultHeight );
}


uiCanvas::uiCanvas( uiObject* parnt, const char* nm )
	: uiMltplWrapObj<uiCanvasABC,i_QCanvas>(
			new i_QCanvas( *this, parnt, nm ), parnt, nm) 
{
}


uiCanvas::~uiCanvas()
{}

const QWidget* 	uiCanvas::qWidget_() const 	{ return mQtThing(); } 

uiScrollView::uiScrollView( uiObject* parnt, const char *nm)
	: uiMltplWrapObj<uiCanvasABC,i_ScrollableCanvas>(
			new i_ScrollableCanvas( *this, parnt, nm ), parnt, nm ) 
	, mousepressed(this)
	, mousemoved(this)
	, mousereleased(this)
	, mousedoubleclicked(this)
{}


uiScrollView::~uiScrollView()
{}


QPaintDevice* uiScrollView::mQPaintDevice() 
{ 
    return mQtThing()->viewport(); 
}


const QWidget* 	uiScrollView::qWidget_() const 	{ return mQtThing(); } 

void uiScrollView::resizeContents ( int w, int h )
{
    mQtThing()->resizeContents( w, h );
}


void uiScrollView::updateContents( uiRect area, bool erase )
{
    mQtThing()->updateContents ( area.topLeft().x(),  area.topLeft().y(),
			       area.width(), area.height() );
}


void uiScrollView::updateContents()
{
    mQtThing()->viewport()->update();
}


void uiScrollView::setContentsPos ( uiPoint topLeft )
/*!<
    Scrolls the content so that the point (x, y) is in the top-left corner. 
    \sa void QScrollView::setContentsPos ( int x, int y )
*/
{
    mQtThing()->setContentsPos( topLeft.x(), topLeft.y() );
}


uiRect uiScrollView::visibleArea() const
{
    QSize vpSize = mQtThing()->viewport()->size();
    uiPoint tl( mQtThing()->contentsX(), mQtThing()->contentsY() );

    return uiRect( tl.x(), tl.y(), 
		   tl.x()+vpSize.width(), tl.y()+vpSize.height() );
}


int uiScrollView::frameWidth() const
{
    return mQtThing()->frameWidth();
}


uiMouseEvent::ButtonState uiScrollView::rubberBandingOn() const
{ return mQtThing()->rubberstate; }


void uiScrollView::setRubberBandingOn( uiMouseEvent::ButtonState b )
{ mQtThing()->rubberstate = b; }


void uiScrollView::setAspectRatio( float r)
{ mQtThing()->aspectrat = r; }


float uiScrollView::aspectRatio()
{ return mQtThing()->aspectrat; }

//------ Qt widgets impl. --------


QSize i_QCanvas::sizeHint() const
{
    return QSize( mUiScrollClient.pref_width, mUiScrollClient.pref_height );
}


QSize i_ScrollableCanvas::sizeHint() const
{
    return QSize( mUiScrollClient.pref_width, mUiScrollClient.pref_height );
}

/*
void i_ScrollableCanvas::update()
{
    viewport()->update();
}
*/

void i_ScrollableCanvas::drawContents ( QPainter * p, int clipx,
                                            int clipy, int clipw, int cliph )
{

    if ( !mUiScrollClient.drawTool()->setActivePainter( p ))
    {
        pErrMsg( "Could not make Qpainter active." );
        return;
    }

    handlePaintEvent( uiRect(clipx,clipy,clipx+clipw,clipy+cliph) );
}


void i_ScrollableCanvas::resizeEvent( QResizeEvent *QREv )
{
    const QSize& os = QREv->oldSize();
    uiSize oldSize( os.width(), os.height() );

    const QSize& ns = QREv->size();
    uiSize nwSize( ns.width() - 2 * frameWidth(), 
                   ns.height() - 2 * frameWidth() );

    handleResizeEvent( QREv,  oldSize, nwSize );

    viewport()->update();
}


void i_ScrollableCanvas::contentsMousePressEvent ( QMouseEvent * e )
{
    if ( mButState( e ) == rubberstate )
    {
	int xvp, yvp;
	rbnding = true;
	rbidx = 0;

	QPainter paint; 
	paint.begin( viewport() );
	paint.setRasterOp( NotROP );
	paint.setPen( QPen( Qt::black, 1, Qt::DotLine ) );
	paint.setBrush( Qt::NoBrush );
	rubber.setRight( e->x() );
	rubber.setLeft( e->x() );
	rubber.setBottom(  e->y() );
	rubber.setTop( e->y() );


	contentsToViewport ( rubber.left(), rubber.top(), xvp, yvp );
	paint.drawRect( xvp, yvp, rubber.right() - rubber.left(), 
			rubber.bottom() - rubber.top() ); 
	
	paint.end();
    }
    else
    {
	uiMouseEvent::ButtonState bSt = 
		    ( uiMouseEvent::ButtonState )(  mButState( e ) );
	uiMouseEvent evt( bSt, e->x(), e->y() );
	mUiScrollClient.contentsMousePressHandler( evt );
    } 
}


void i_ScrollableCanvas::contentsMouseMoveEvent( QMouseEvent * e )
{
// TODO: start a timer, so no move-events required to continue scrolling

    if ( rbnding && mButState( e ) == rubberstate )
    {
	int xvp, yvp;

	QPainter paint; 
	paint.begin( viewport() );
	paint.setRasterOp( NotROP );
	paint.setPen( QPen( Qt::black, 1, Qt::DotLine ) );
	paint.setBrush( Qt::NoBrush );
       
	
	contentsToViewport ( rubber.left(), rubber.top(), xvp, yvp ); 
	paint.drawRect( xvp, yvp, rubber.right() - rubber.left(), 
			rubber.bottom() - rubber.top() ); // erases old rubber
	if( aspectrat )
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

	    rubber.setWidth( rbidx*xfac );
	    rubber.setHeight( rbidx * aspectrat*yfac );
	}
	else
	{
	    rubber.setRight( e->x() );
	    rubber.setBottom(  e->y() );
	}

	ensureVisible( rubber.right(), rubber.bottom() );

	contentsToViewport ( rubber.left(), rubber.top(), xvp, yvp ); 
	paint.drawRect( xvp, yvp, rubber.right() - rubber.left(), 
			rubber.bottom() - rubber.top() ); 
	paint.end();
    }
    else
    {
	uiMouseEvent::ButtonState bSt = 
		    ( uiMouseEvent::ButtonState )(  mButState( e ) );
	uiMouseEvent evt( bSt, e->x(), e->y() );
	mUiScrollClient.contentsMouseMoveHandler( evt );
    } 
}


void i_ScrollableCanvas::contentsMouseReleaseEvent ( QMouseEvent * e )
{
    if ( rbnding && mButState( e ) == rubberstate )
    {
	int xvp, yvp;
	rbnding = false;

	QPainter paint; 
	paint.begin( viewport() );
	paint.setRasterOp( NotROP );
	paint.setPen( QPen( Qt::black, 1, Qt::DotLine ) );
	paint.setBrush( Qt::NoBrush );

	contentsToViewport ( rubber.left(), rubber.top(), xvp, yvp ); 
	paint.drawRect( xvp, yvp, rubber.right() - rubber.left(), 
			rubber.bottom() - rubber.top() ); 
	paint.end();

	rubber.checkCorners();
	if( rubber.width() > 10 && rubber.height() > 10 )
	    mUiScrollClient.rubberBandHandler( rubber );
    }
    else
    {
	uiMouseEvent::ButtonState bSt = 
		    ( uiMouseEvent::ButtonState )(  mButState( e ) );
	uiMouseEvent evt( bSt, e->x(), e->y() );
	mUiScrollClient.contentsMouseReleaseHandler( evt );
    } 
}


void i_ScrollableCanvas::contentsMouseDoubleClickEvent ( QMouseEvent * e )
{
    uiMouseEvent::ButtonState bSt = 
		( uiMouseEvent::ButtonState )(  mButState( e ) );
    uiMouseEvent evt( bSt, e->x(), e->y() );
    mUiScrollClient.contentsMouseDoubleClickHandler( evt );
}

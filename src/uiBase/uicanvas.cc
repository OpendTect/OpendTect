/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.cc,v 1.18 2003-02-25 15:12:33 arend Exp $
________________________________________________________________________

-*/

#include <uicanvas.h>
#include <errh.h>
#include <uiobj.h>
#include <i_uidrwbody.h>

#include <iodrawimpl.h>

#include <qscrollview.h>
#include <qpainter.h>

#include <uimouse.h>

#define mButState( e ) ( e->state() | e->button() )

class uiScrollViewBody;
class uiMouseEvent;


int uiCanvasDefaults::defaultWidth  = 600;
int uiCanvasDefaults::defaultHeight = 400;


class uiCanvasBody : public uiDrawableObjBody<uiCanvas,QWidget>
{

public:
                        uiCanvasBody( uiCanvas& handle, uiParent* p,
				      const char *nm="uiCanvasBody")
			    : uiDrawableObjBody<uiCanvas,QWidget>
				( handle, p, nm ) 
			    {
				setStretch( 2, 2 );
				setPrefWidth( uiCanvasDefaults::defaultWidth );
				setPrefHeight( uiCanvasDefaults::defaultHeight);
			    }

    virtual             ~uiCanvasBody() {};
};



//! Derived class from QScrollView in order to handle 'n relay Qt's events
/*
    defined locally in .cc file.
*/
class uiScrollViewBody : public uiDrawableObjBody<uiScrollView,QScrollView>
{
public:

                        uiScrollViewBody( uiScrollView& handle,
                                          uiParent* p=0,
                                          const char *nm="uiScrollViewBody" )
			    : uiDrawableObjBody<uiScrollView,QScrollView> 
				( handle, p, nm )
                            , rubberstate( uiMouseEvent::NoButton )
                            , rbnding( false )
                            , aspectrat( 0.0 ), rbidx( 0 ) 
			    {
				setStretch( 2, 2 );
				setPrefContentsWidth(
					    uiCanvasDefaults::defaultWidth );
				setPrefContentsHeight( 
					    uiCanvasDefaults::defaultHeight);
			    }

//    void                update();

    uiMouseEvent::ButtonState rubberstate;
    float               aspectrat;

    virtual QPaintDevice* mQPaintDevice()	{ return viewport(); }

    uiRect		visibleArea() const;

    void		setPrefContentsWidth( int w )      
			{ 
			    setPrefWidth( w + 2*frameWidth() ); 
			}
    void		setPrefContentsHeight( int h )     
			{ 
			    setPrefHeight( h + 2*frameWidth() ); 
			}

    virtual uiSize      actualsize( bool include_border = true) const
			{
			    uiSize sz= uiObjectBody::actualsize(include_border);
			    if ( include_border ) return sz;

			    int fw = frameWidth();
			    int w = sz.hNrPics()-2*fw;
			    if ( w<1 ) w=1;
			    int h = sz.vNrPics()-2*fw;
			    if ( h<1 ) h=1;
			    return uiSize( w, h, true );
			}


    void		setRubberBandingOn(uiMouseEvent::ButtonState b)
				    { rubberstate = b; }
    uiMouseEvent::ButtonState	rubberBandingOn() const	{ return rubberstate; }
    void			setAspectRatio(float r)	{ aspectrat = r; }
    float			aspectRatio()		{ return aspectrat; }

    virtual void        reDraw( bool deep )
			    { 
				//uiObjectBody::reDraw( deep ); 
				//viewport()->update();
				//updateContents();
				//repaintContents(true);
				updateContents();
			    }
protected:

    virtual void        drawContents ( QPainter * p, int clipx,
                            int clipy, int clipw, int cliph );
    virtual void        resizeEvent( QResizeEvent * );


    //! over-ride DrawableObjBody impl, because we want to use drawContents()
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


uiRect uiScrollViewBody::visibleArea() const
{

    uiSize vpSize = actualsize(false);

    uiPoint tl( contentsX(), contentsY() );
    return uiRect( tl, mMAX( visibleWidth(), vpSize.hNrPics()),
                       mMAX( visibleHeight(), vpSize.vNrPics()) );
}


void uiScrollViewBody::drawContents ( QPainter * p, int clipx,
                                            int clipy, int clipw, int cliph )
{

    if ( !drawTool()->setActivePainter( p ))
    {
        pErrMsg( "Could not make Qpainter active." );
        return;
    }

    handlePaintEvent( uiRect( uiPoint(clipx,clipy), clipw, cliph) );
}


void uiScrollViewBody::resizeEvent( QResizeEvent *QREv )
{
    const QSize& os = QREv->oldSize();
    uiSize oldSize( os.width(), os.height(), true );

    const QSize& ns = QREv->size();
    uiSize nwSize( ns.width()  - 2 * frameWidth(), 
                   ns.height() - 2 * frameWidth(), true );

    handleResizeEvent( QREv, oldSize, nwSize );

    viewport()->update();
}


void uiScrollViewBody::contentsMousePressEvent ( QMouseEvent * e )
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

	handle_.mousepressed.trigger( evt, handle_ );
    } 
}


void uiScrollViewBody::contentsMouseMoveEvent( QMouseEvent * e )
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

	    rubber.setHNrPics( rbidx*xfac );
	    rubber.setVNrPics( mNINT(rbidx * aspectrat*yfac) );
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

	handle_.mousemoved.trigger( evt, handle_ );
    } 
}


void uiScrollViewBody::contentsMouseReleaseEvent ( QMouseEvent * e )
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
	if( rubber.hNrPics() > 10 && rubber.vNrPics() > 10 )
	    handle_.rubberBandHandler( rubber );
    }
    else
    {
	uiMouseEvent::ButtonState bSt = 
		    ( uiMouseEvent::ButtonState )(  mButState( e ) );
	uiMouseEvent evt( bSt, e->x(), e->y() );

	handle_.mousereleased.trigger( evt, handle_ );
    } 
}


void uiScrollViewBody::contentsMouseDoubleClickEvent ( QMouseEvent * e )
{
    uiMouseEvent::ButtonState bSt =
                ( uiMouseEvent::ButtonState )(  mButState( e ) );
    uiMouseEvent evt( bSt, e->x(), e->y() );
    handle_.mousedoubleclicked.trigger( evt, handle_ );
}



uiCanvas::uiCanvas( uiParent* p, const char *nm )
    : uiDrawableObj( p,nm, mkbody(p,nm) ) {}

uiCanvasBody& uiCanvas::mkbody( uiParent* p,const char* nm)
{
    body_ = new uiCanvasBody( *this,p,nm );
    return *body_;
}



uiScrollView::uiScrollView( uiParent* p, const char *nm )
    : uiDrawableObj( p,nm, mkbody(p,nm) )
    , mousepressed(this)
    , mousemoved( this )
    , mousereleased( this )
    , mousedoubleclicked( this ) {}

uiScrollViewBody& uiScrollView::mkbody( uiParent* p,const char* nm)
{
    body_ = new uiScrollViewBody( *this,p,nm );
    return *body_;
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
    { body_->setContentsPos(topLeft.x(),topLeft.y()); }


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


void  uiScrollView::setRubberBandingOn(uiMouseEvent::ButtonState st)
    { body_->setRubberBandingOn(st); }

uiMouseEvent::ButtonState uiScrollView::rubberBandingOn() const
    { return body_->rubberBandingOn(); }

void  uiScrollView::setAspectRatio( float r )
    { body_->setAspectRatio(r); }

float  uiScrollView::aspectRatio()
    { return body_->aspectRatio(); }




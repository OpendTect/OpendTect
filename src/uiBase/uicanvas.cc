/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.cc,v 1.8 2001-08-27 12:58:50 windev Exp $
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
				setPrefWidth( uiCanvasDefaults::defaultWidth );
				setPrefHeight( uiCanvasDefaults::defaultHeight);
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

    virtual uiSize      actualSize( bool include_border = true) const
			{
			    uiSize sz= uiObjectBody::actualSize(include_border);
			    if ( include_border ) return sz;

			    int fw = frameWidth();
			    int w = sz.width()-2*fw;
			    if ( w<0 ) w=0;
			    int h = sz.height()-2*fw;
			    if ( h<0 ) h=0;
			    return uiSize( w, h );
			}


    void		setRubberBandingOn(uiMouseEvent::ButtonState b)
				    { rubberstate = b; }
    uiMouseEvent::ButtonState	rubberBandingOn() const	{ return rubberstate; }
    void			setAspectRatio(float r)	{ aspectrat = r; }
    float			aspectRatio()		{ return aspectrat; }

    virtual void        reDraw( bool deep )
			    { 
				uiObjectBody::reDraw( deep ); 
				viewport()->update();
			    }
protected:

    virtual void        drawContents ( QPainter * p, int clipx,
                            int clipy, int clipw, int cliph );
    virtual void        resizeEvent( QResizeEvent * );
/*
    virtual void        paintEvent( QPaintEvent* ev )
                        { QScrollView::paintEvent(ev); }
*/
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
    QSize vpSize = clipper()->size();
    uiPoint tl( contentsX(), contentsY() );
    return uiRect( tl.x(), tl.y(), 
		   tl.x()+vpSize.width(), tl.y()+vpSize.height() );
}


void uiScrollViewBody::drawContents ( QPainter * p, int clipx,
                                            int clipy, int clipw, int cliph )
{

    if ( !drawTool()->setActivePainter( p ))
    {
        pErrMsg( "Could not make Qpainter active." );
        return;
    }

    handlePaintEvent( uiRect(clipx,clipy,clipx+clipw,clipy+cliph) );
}


void uiScrollViewBody::resizeEvent( QResizeEvent *QREv )
{
    const QSize& os = QREv->oldSize();
    uiSize oldSize( os.width(), os.height() );

    const QSize& ns = QREv->size();
    uiSize nwSize( ns.width() - 2 * frameWidth(), 
                   ns.height() - 2 * frameWidth() );

    handleResizeEvent( QREv,  oldSize, nwSize );

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

	    rubber.setWidth( rbidx*xfac );
	    rubber.setHeight( mNINT(rbidx * aspectrat*yfac) );
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
	if( rubber.width() > 10 && rubber.height() > 10 )
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
    body_->updateContents ( area.topLeft().x(),  area.topLeft().y(),
                               area.width(), area.height() );

}






void uiScrollView::resizeContents( int w, int h )
    { body_->resizeContents(w,h); }

void uiScrollView::setContentsPos( uiPoint topLeft )
    { body_->setContentsPos(topLeft.x(),topLeft.y()); }


uiRect uiScrollView::visibleArea() const
    { return body_->visibleArea(); }


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




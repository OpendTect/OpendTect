#ifndef i_uidrwbody_h
#define i_uidrwbody_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          03/07/2001
 RCS:           $Id: i_uidrwbody.h,v 1.19 2007-03-08 13:32:01 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiobjbody.h"
#include "iodrawimpl.h"
#include <qwidget.h>

#ifndef USEQT3
# include <QPaintEvent>
# include "uirubberband.h"
# include "mouseevent.h"
#endif

static const int c_minnrpix = 10;


/*! \brief template implementation for drawable objects

    Each Qt drawable object has a paint device.
    It also receives paint and resize events, which are relayed to its
    ui handle object.
*/
template <class C,class T>
class uiDrawableObjBody : public uiObjectBody, public T, public ioDrawAreaImpl
{
public:
                        uiDrawableObjBody( C& handle, 
					   uiParent* parent, const char* nm )
			    : uiObjectBody(parent,nm)
			    , T( parent && parent->pbody()? 
				    parent->pbody()->managewidg() : 0 , nm )
                            , handle_(handle)
#ifndef USEQT3
			    , rubberband_(0)
#endif
                            {}

#include		"i_uiobjqtbody.h"

    virtual             ~uiDrawableObjBody()
    			{
#ifndef USEQT3
			    delete rubberband_;
#endif
			}

    virtual QPaintDevice* 	mQPaintDevice()		{ return this; }
    virtual const QPaintDevice*	mQPaintDevice() const	{ return this; }

#ifndef USEQT3
public:
    void		setMouseTracking( bool yn )
			{ T::setMouseTracking( yn ); }
    bool		hasMouseTracking() const
			{ return T::hasMouseTracking(); }
#endif

protected:
#ifndef USEQT3
    virtual void	drawContents(QPainter*);
#endif

    virtual void	paintEvent(QPaintEvent*);
    void		handlePaintEvent(uiRect,QPaintEvent* ev=0);
    virtual void	resizeEvent(QResizeEvent*);
    void		handleResizeEvent(QResizeEvent*,uiSize old,uiSize nw);

#ifndef USEQT3
    virtual void	mousePressEvent(QMouseEvent*);
    virtual void	mouseMoveEvent(QMouseEvent*);
    virtual void	mouseReleaseEvent(QMouseEvent*);
    virtual void	mouseDoubleClickEvent(QMouseEvent*);

    uiRubberBand*	rubberband_;
#endif
};


#ifndef USEQT3
template <class C,class T>
void uiDrawableObjBody<C,T>::drawContents( QPainter* ptr )
{
    const QRect qr = T::contentsRect();
    uiRect rect( qr.left(), qr.top(), qr.right(), qr.bottom() );
    handlePaintEvent( rect );
}
#endif


template <class C,class T>
void uiDrawableObjBody<C,T>::paintEvent( QPaintEvent* ev )
{
    const QRect& qr = ev->rect();
    uiRect rect( qr.left() , qr.top(), qr.right(), qr.bottom() );
    handlePaintEvent( rect, ev );
}


template <class C,class T>
void uiDrawableObjBody<C,T>::handlePaintEvent( uiRect r, QPaintEvent* ev )
{
    handle_.preDraw.trigger( handle_ );
    if ( ev ) T::paintEvent( ev );

    handle_.reDrawHandler( r );

    handle_.postDraw.trigger( handle_ );
    if ( handle_.drawTool()->active() )
	handle_.drawTool()->endDraw();
}


template <class C,class T>
void uiDrawableObjBody<C,T>::resizeEvent( QResizeEvent* ev )
{
    const QSize& os = ev->oldSize();
    uiSize oldsize( os.width(), os.height() );

    const QSize& ns = ev->size();
    uiSize nwsize( ns.width(), ns.height() );

    handleResizeEvent( ev, oldsize, nwsize );
}


template <class C,class T>
void uiDrawableObjBody<C,T>::handleResizeEvent( QResizeEvent* ev,
						uiSize old, uiSize nw )
{
    T::resizeEvent( ev );
    handle_.reSized.trigger( handle_ );
}


#ifndef USEQT3

template <class C,class T>
void uiDrawableObjBody<C,T>::mousePressEvent( QMouseEvent* qev )
{
    if ( handle_.isRubberBandingOn() )
    {
	if ( !rubberband_ ) rubberband_ = new uiRubberBand( this );
	rubberband_->start( qev );
    }
    else
    {
	OD::ButtonState bs = OD::ButtonState( qev->state() | qev->button() );
	MouseEvent mev( bs, qev->x(), qev->y() );
	handle_.getMouseEventHandler().triggerButtonPressed( mev );
    }
}


template <class C,class T>
void uiDrawableObjBody<C,T>::mouseMoveEvent( QMouseEvent* qev )
{
    if ( handle_.isRubberBandingOn() && rubberband_ )
	rubberband_->extend( qev );
    else
    {
	OD::ButtonState bs = OD::ButtonState( qev->state() | qev->button() );
	MouseEvent mev( bs, qev->x(), qev->y() );
	handle_.getMouseEventHandler().triggerMovement( mev );
    }
}


template <class C,class T>
void uiDrawableObjBody<C,T>::mouseReleaseEvent( QMouseEvent* qev )
{
    bool ishandled = false;
    if ( handle_.isRubberBandingOn() )
    {
	rubberband_->stop( qev );
	uiRect newarea = rubberband_->area();
	bool sizeok = newarea.hNrPics() > c_minnrpix 
	    	      && newarea.vNrPics() > c_minnrpix;
	if ( sizeok )
	{
	    ishandled = true;
	    handle_.rubberBandHandler( newarea );
	}
    }
    
    if ( !ishandled )
    {
	OD::ButtonState bs = OD::ButtonState( qev->state() | qev->button() );
	MouseEvent mev( bs, qev->x(), qev->y() );
	handle_.getMouseEventHandler().triggerButtonReleased( mev );
    }
}


template <class C,class T>
void uiDrawableObjBody<C,T>::mouseDoubleClickEvent( QMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->state() | qev->button() );
    MouseEvent mev( bs, qev->x(), qev->y() );
    handle_.getMouseEventHandler().triggerDoubleClick( mev );
}
#endif

#endif

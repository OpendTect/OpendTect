#ifndef i_uidrwbody_h
#define i_uidrwbody_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          03/07/2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiobjbody.h"
#include "iodrawimpl.h"
#include "mouseevent.h"
#include "uirubberband.h"

#include <QPaintEvent>
#include <QWidget>

static const int cMinNrPix = 10;


/*! \brief template implementation for drawable objects

    Each Qt drawable object has a paint device.
    It also receives paint and resize events, which are relayed to its
    ui handle object.
*/
template <class C,class T>
class uiDrawableObjBody : public uiObjectBody, public T, public ioDrawAreaImpl
{
public:
                        uiDrawableObjBody( C& hndle, 
					   uiParent* p, const char* nm )
			    : uiObjectBody(p,nm)
			    , T(p && p->pbody() ? p->pbody()->managewidg() : 0)
                            , handle_(hndle)
			    , rubberband_(0)
			    , havemousetracking_(false)
                            {
				T::setObjectName( nm );
			    }

#include		"i_uiobjqtbody.h"

    virtual             ~uiDrawableObjBody()
    			{ delete rubberband_; }

    virtual QPaintDevice* qPaintDevice()		{ return this; }

public:
    void		setMouseTracking( bool yn )
			{ havemousetracking_ = yn; T::setMouseTracking( yn ); }
    bool		hasMouseTracking() const
			{ return havemousetracking_; }

protected:
    virtual void	drawContents(QPainter*);
    virtual void	paintEvent(QPaintEvent*);
    void		handlePaintEvent(uiRect,QPaintEvent* ev=0);
    virtual void	resizeEvent(QResizeEvent*);
    void		handleResizeEvent(QResizeEvent*,uiSize old,uiSize nw);

    virtual void	mousePressEvent(QMouseEvent*);
    virtual void	mouseMoveEvent(QMouseEvent*);
    virtual void	mouseReleaseEvent(QMouseEvent*);
    virtual void	mouseDoubleClickEvent(QMouseEvent*);
    virtual void	wheelEvent(QWheelEvent*);

    uiRubberBand*	rubberband_;
    bool		havemousetracking_;

    void		reSetMouseTracking()
			{ T::setMouseTracking( havemousetracking_ ); }
};


template <class C,class T>
void uiDrawableObjBody<C,T>::drawContents( QPainter* ptr )
{
    const QRect qr = T::contentsRect();
    uiRect r( qr.left(), qr.top(), qr.right(), qr.bottom() );
    handlePaintEvent( r );
}


template <class C,class T>
void uiDrawableObjBody<C,T>::paintEvent( QPaintEvent* ev )
{
    const QRect& qr = ev->rect();
    uiRect r( qr.left() , qr.top(), qr.right(), qr.bottom() );
    handlePaintEvent( r, ev );
}


template <class C,class T>
void uiDrawableObjBody<C,T>::handlePaintEvent( uiRect r, QPaintEvent* ev )
{
    handle_.preDraw.trigger( handle_ );
    if ( ev ) T::paintEvent( ev );
    handle_.reDrawHandler( r );
    handle_.postDraw.trigger( handle_ );
    handle_.drawTool().dismissPainter();
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
	OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button());
	MouseEvent mev( bs, qev->x(), qev->y() );
	handle_.getMouseEventHandler().triggerButtonPressed( mev );
    }
}


template <class C,class T>
void uiDrawableObjBody<C,T>::mouseMoveEvent( QMouseEvent* qev )
{
    if ( rubberband_ && handle_.isRubberBandingOn() )
	rubberband_->extend( qev );
    else
    {
	OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button());
	MouseEvent mev( bs, qev->x(), qev->y() );
	handle_.getMouseEventHandler().triggerMovement( mev );
    }
}


template <class C,class T>
void uiDrawableObjBody<C,T>::mouseReleaseEvent( QMouseEvent* qev )
{
    bool ishandled = false;
    if ( rubberband_ && handle_.isRubberBandingOn() )
    {
	rubberband_->stop( qev );
	uiRect newarea = rubberband_->area();
	bool sizeok = newarea.hNrPics() > cMinNrPix 
		      && newarea.vNrPics() > cMinNrPix;
	if ( sizeok )
	{
	    ishandled = true;
	    handle_.rubberBandHandler( newarea );
	}
	delete rubberband_; rubberband_ = 0;
	reSetMouseTracking();
    }
    
    if ( !ishandled )
    {
	OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button());
	MouseEvent mev( bs, qev->x(), qev->y() );
	handle_.getMouseEventHandler().triggerButtonReleased( mev );
    }
}


template <class C,class T>
void uiDrawableObjBody<C,T>::mouseDoubleClickEvent( QMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    MouseEvent mev( bs, qev->x(), qev->y() );
    handle_.getMouseEventHandler().triggerDoubleClick( mev );
}


template <class C,class T>
void uiDrawableObjBody<C,T>::wheelEvent( QWheelEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->buttons() );
    MouseEvent mev( bs, qev->x(), qev->y(), // see uicanvas.cc 'delta2angle'
		    qev->delta() * 0.002181661564992911886 );
    handle_.getMouseEventHandler().triggerWheel( mev );
}

#endif

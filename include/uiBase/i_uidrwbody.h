#ifndef i_uidrwbody_h
#define i_uidrwbody_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          03/07/2001
 RCS:           $Id: i_uidrwbody.h,v 1.11 2007-02-12 16:49:28 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobjbody.h"
#include "iodrawimpl.h"
#include <qwidget.h>

#ifdef USEQT4
# include <QPaintEvent>
# include "uirubberband.h"
#endif


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
#ifdef USEQT4
			    , rubberband_(0)
#endif
                            {}

#include		"i_uiobjqtbody.h"

    virtual             ~uiDrawableObjBody()
    			{
#ifdef USEQT4
			    delete rubberband_;
#endif
			}

    virtual QPaintDevice* 	mQPaintDevice()		{ return this; }
    virtual const QPaintDevice*	mQPaintDevice() const	{ return this; }

protected:
#ifdef USEQT4
    virtual void	drawContents(QPainter*);
#endif

    virtual void	paintEvent(QPaintEvent*);
    void		handlePaintEvent(uiRect,QPaintEvent* ev=0);
    virtual void	resizeEvent(QResizeEvent*);
    void		handleResizeEvent(QResizeEvent*,uiSize old,uiSize nw);

#ifdef USEQT4
    virtual void	mousePressEvent(QMouseEvent*);
    virtual void	mouseMoveEvent(QMouseEvent*);
    virtual void	mouseReleaseEvent(QMouseEvent*);

    uiRubberBand*	rubberband_;
#endif
};


#ifdef USEQT4
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
    uiSize oldsize( os.width(), os.height(), true );

    const QSize& ns = ev->size();
    uiSize nwsize( ns.width(), ns.height(), true );

    handleResizeEvent( ev, oldsize, nwsize );
}


template <class C,class T>
void uiDrawableObjBody<C,T>::handleResizeEvent( QResizeEvent* ev,
						uiSize old, uiSize nw )
{
    T::resizeEvent( ev );
    handle_.reSized.trigger( handle_ );
}


#ifdef USEQT4
template <class C,class T>
void uiDrawableObjBody<C,T>::mousePressEvent( QMouseEvent* ev )
{
    if ( handle_.isRubberBandingOn() )
    {
	if ( !rubberband_ ) rubberband_ = new uiRubberBand( this );
	rubberband_->start( ev );
    }
}


template <class C,class T>
void uiDrawableObjBody<C,T>::mouseMoveEvent( QMouseEvent* ev )
{
    if ( handle_.isRubberBandingOn() )
	rubberband_->extend( ev );
}


template <class C,class T>
void uiDrawableObjBody<C,T>::mouseReleaseEvent( QMouseEvent* ev )
{
    if ( handle_.isRubberBandingOn() )
	rubberband_->stop( ev );
}
#endif

#endif

#ifndef  i_uidrwbody_h
#define  i_uidrwbody_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          03/07/2001
 RCS:           $Id: i_uidrwbody.h,v 1.9 2007-02-07 16:46:22 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobjbody.h"
#include <qwidget.h>
#include <iodrawimpl.h>

#ifdef USEQT4
# include <QPaintEvent>
#endif


/*! \brief template implementation for drawable objects

    Each Qt drawable object has a paint device.
    It also receives paint and resize events, which are relayed to its
    ui handle object.
*/
template <class C, class T>
class uiDrawableObjBody : public uiObjectBody, public T, public ioDrawAreaImpl
{
public:
                        uiDrawableObjBody( C& handle, 
					   uiParent* parnt, const char* nm )
			    : uiObjectBody( parnt, nm )
			    , T( parnt && parnt->pbody()? 
				    parnt->pbody()->managewidg() : 0 , nm )
                            , handle_( handle )
                            {}

#include		"i_uiobjqtbody.h"

    virtual             ~uiDrawableObjBody()			{}

    virtual QPaintDevice* 	mQPaintDevice()		{ return this; }
    virtual const QPaintDevice*	mQPaintDevice() const	{ return this; }

protected:
#ifdef USEQT4
    virtual void	drawContents( QPainter* ptr )
			{
			    const QRect qr = T::contentsRect();
			    uiRect rect( qr.left(), qr.top(),
				         qr.right(), qr.bottom() );
			    handlePaintEvent( rect );
			}
#endif

    //! Relays paint events to it's ui handle (handle_)
    virtual void	paintEvent( QPaintEvent *QPEv )
			    {
				const QRect& qr = QPEv->rect();
				uiRect rect ( qr.left() , qr.top(), 
					      qr.right(), qr.bottom() );
				handlePaintEvent( rect, QPEv );
			    }

    //! Relays paint events to it's ui handle (handle_)
    void		handlePaintEvent( uiRect r, QPaintEvent* QPEv=0 )
			    {
				handle_.preDraw.trigger( handle_ );
				if ( QPEv ) T::paintEvent( QPEv );

				handle_.reDrawHandler( r );

				handle_.postDraw.trigger( handle_ );
				if ( handle_.drawTool()->active() )
				    handle_.drawTool()->endDraw();
			    }

    virtual void	resizeEvent( QResizeEvent* QREv )
			    {
				const QSize& os = QREv->oldSize();
				uiSize oldSize( os.width(), os.height(), true );

				const QSize& ns = QREv->size();
				uiSize nwSize( ns.width(), ns.height(), true );

				handleResizeEvent( QREv,  oldSize, nwSize );
			    }

    //! Relays resize events to it's ui handle (handle_)
    void		handleResizeEvent( QResizeEvent* QREv,
					   uiSize old, uiSize nw )
			    {
				T::resizeEvent( QREv );
				handle_.reSized.trigger( handle_ );
			    }
};


#endif

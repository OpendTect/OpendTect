#ifndef  I_UIDRAWBODY_H
#define  I_UIDRAWBODY_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          03/07/2001
 RCS:           $Id: i_uidrwbody.h,v 1.4 2002-05-17 11:34:54 arend Exp $
________________________________________________________________________

-*/

#include "uiobjbody.h"
#include <qwidget.h>
#include <iodrawimpl.h>


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
			    : uiObjectBody( parnt )
			    , T( parnt && parnt->body()? 
				    parnt->body()->managewidg() : 0 , nm )
                            , handle_( handle )
                            {}

#include		"i_uiobjqtbody.h"

    virtual             ~uiDrawableObjBody()			{}

    //! Relays paint events to it's ui handle (handle_)
    virtual void	paintEvent( QPaintEvent *QPEv )
			    {
				const QRect& r = QPEv->rect();
				uiRect rect ( r.left() , r.top(), 
					      r.right(), r.bottom() );
				handlePaintEvent( rect, QPEv );
			    }

    //! Relays paint events to it's ui handle (handle_)
    void		handlePaintEvent( uiRect r, QPaintEvent* QPEv=0 )
			    {
				handle_.preDraw.trigger( handle_ );
				if( QPEv ) T::paintEvent( QPEv );

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

    virtual QPaintDevice* 	mQPaintDevice()		{ return this; }
    virtual const QPaintDevice*	mQPaintDevice() const	{ return this; }

};


#endif

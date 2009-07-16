/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicanvas.cc,v 1.47 2009-07-16 07:24:15 cvsbert Exp $";

#include "uicanvas.h"
#include "i_uidrwbody.h"

#include <QApplication>
#include <QEvent>
#include <QFrame>


static const int cDefaultWidth  = 1;
static const int cDefaultHeight = 1;

class uiCanvasBody : public uiDrawableObjBody<uiCanvas,QFrame>
{

public:
uiCanvasBody( uiCanvas& handle, uiParent* p, const char *nm="uiCanvasBody")
    : uiDrawableObjBody<uiCanvas,QFrame>( handle, p, nm ) 
    , handle_( handle )
{
    setStretch( 2, 2 );
    setPrefWidth( cDefaultWidth );
    setPrefHeight( cDefaultHeight );
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

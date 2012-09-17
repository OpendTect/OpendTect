/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicanvas.cc,v 1.50 2011/04/21 13:09:13 cvsbert Exp $";

#include "uicanvas.h"
#include "i_uidrwbody.h"

#include <QFrame>


static const int cDefaultWidth  = 1;
static const int cDefaultHeight = 1;

class uiCanvasBody : public uiDrawableObjBody<uiCanvas,QFrame>
{

public:
uiCanvasBody( uiCanvas& hndle, uiParent* p, const char *nm="uiCanvasBody")
    : uiDrawableObjBody<uiCanvas,QFrame>( hndle, p, nm ) 
    , handle_( hndle )
{
    setStretch( 2, 2 );
    setPrefWidth( cDefaultWidth );
    setPrefHeight( cDefaultHeight );
    setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
}

    virtual		~uiCanvasBody()		{}
    void		updateCanvas()		{ QWidget::update(); }

private:
    uiCanvas&		handle_;
};


uiCanvas::uiCanvas( uiParent* p, const Color& col, const char *nm )
    : uiDrawableObj( p,nm, mkbody(p,nm) )
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

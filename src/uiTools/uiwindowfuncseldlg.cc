/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          August 2007
 RCS:           $Id: uiwindowfuncseldlg.cc,v 1.4 2008-04-04 04:29:05 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uiwindowfuncseldlg.h"

#include "uicanvas.h"
#include "uiworld2ui.h"
#include "iodraw.h"
#include "windowfunction.h"

#define mTransHeight    250
#define mTransWidth     500

uiWindowFuncSelDlg::uiWindowFuncSelDlg( uiParent* p )
    : uiDialog( p, uiDialog::Setup("Window/Taper display",0,0).modal(false) )
    , transform_(new uiWorld2Ui())
{
    setCtrlStyle( LeaveOnly );
    canvas_ = new uiCanvas( this, Color::White, "Window/Taper canvas" );
    canvas_->setPrefHeight( mTransHeight );
    canvas_->setPrefWidth( mTransWidth );
    canvas_->setStretch(10,0);
    canvas_->postDraw.notify( mCB(this,uiWindowFuncSelDlg,reDraw) );
    transform_->set( uiSize(mTransWidth,mTransHeight),
		     uiWorldRect(-1.2,1,1.2,0) );
    setColor( Color(175,47,200) );
}


uiWindowFuncSelDlg::~uiWindowFuncSelDlg()
{
    delete transform_;
}


void uiWindowFuncSelDlg::setCurrentWindowFunc( const char* nm )
{
    pointlist_.erase();
    const StepInterval<float> xrg( -1.2, 1.2, 0.01 );
    WindowFunction* winfunc = WinFuncs().create( nm );
    if ( !winfunc )
	return;

    for ( int idx=0; idx<xrg.nrSteps(); idx++ )
    {
	const float x = xrg.atIndex( idx );
	const float y = winfunc->getValue( x );
	pointlist_ += uiPoint( transform_->transform(uiWorldPoint(x,y)) );
    }

    canvas_->update();
}


const Color& uiWindowFuncSelDlg::getColor() const
{ return color_; }

void uiWindowFuncSelDlg::setColor( const Color& nc )
{ color_ = nc; }


void uiWindowFuncSelDlg::reDraw( CallBacker* )
{
    ioDrawTool& dt = canvas_->drawTool();
    dt.setPenColor( color_ );
    dt.setPenWidth( 2 );
    dt.drawPolyline( pointlist_ );
}

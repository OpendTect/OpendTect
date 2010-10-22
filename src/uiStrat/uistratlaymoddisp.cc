/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlaymoddisp.cc,v 1.1 2010-10-22 13:50:07 cvsbert Exp $";

#include "uistratlaymoddisp.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "stratlayermodel.h"
#include "property.h"


uiStratLayerModelDisp::uiStratLayerModelDisp( uiParent* p,
						const Strat::LayerModel& lm )
    : uiGraphicsView(p,"LayerModel display")
    , lm_(lm)
    , emptyitm_(0)
{
    setPrefWidth( 500 );
    setPrefHeight( 250 );
    reSize.notify( mCB(this,uiStratLayerModelDisp,reDraw) );
    reDrawNeeded.notify( mCB(this,uiStratLayerModelDisp,reDraw) );

    getMouseEventHandler().buttonReleased.notify(
	    			mCB(this,uiStratLayerModelDisp,usrClickCB) );
}


uiStratLayerModelDisp::~uiStratLayerModelDisp()
{
    eraseAll();
}


void uiStratLayerModelDisp::eraseAll()
{
    delete emptyitm_; emptyitm_ = 0;
}


void uiStratLayerModelDisp::usrClickCB( CallBacker* cb )
{
}


void uiStratLayerModelDisp::reDraw( CallBacker* )
{
    eraseAll();
    if ( lm_.isEmpty() )
    {
	if ( !emptyitm_ )
	{
	    emptyitm_ = new uiTextItem( "<---empty--->",
		    			mAlignment(HCenter,VCenter) );
	    emptyitm_->setPenColor( Color::Black() );
	    emptyitm_->setPos( uiPoint( width()/2, height() / 2 ) );
	    scene().addItem( emptyitm_ );
	}
    }
    else
    {
	delete emptyitm_; emptyitm_ = 0;
	doDraw();
    }
}


void uiStratLayerModelDisp::doDraw()
{
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlayseqgendesc.cc,v 1.1 2010-10-19 08:52:03 cvsbert Exp $";

#include "uistratsinglayseqgendesc.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "stratlayermodel.h"


mImplFactory2Param(uiLayerSequenceGenDesc,uiParent*,
	Strat::LayerSequenceGenDesc&,uiLayerSequenceGenDesc::factory)


uiLayerSequenceGenDesc::uiLayerSequenceGenDesc( uiParent* p,
					Strat::LayerSequenceGenDesc& desc )
    : uiGraphicsView(p,"LayerSequence Gen Desc editor")
    , desc_(desc)
    , border_(10)
{
    setPrefWidth( 200 );
    setPrefHeight( 600 );
    reDrawNeeded.notify( mCB(this,uiLayerSequenceGenDesc,reDraw) );
}


void uiLayerSequenceGenDesc::reDraw( CallBacker* )
{
    uiRect& wr = const_cast<uiRect&>( workrect_ );
    wr.setLeft( border_.left() );
    wr.setRight( (int)(width() - border_.right() + .5) );
    wr.setTop( border_.top() );
    wr.setBottom( (int)(height() - border_.bottom() + .5) );

    if ( !outeritm_ )
	{ outeritm_ = new uiRectItem; scene().addItem( outeritm_ ); }
    outeritm_->setRect( workrect_.left(), workrect_.top(),
	    		workrect_.width(), workrect_.height() );

    doDraw();
}


uiSingleLayerSequenceGenDesc::uiSingleLayerSequenceGenDesc( uiParent* p,
	Strat::LayerSequenceGenDesc& d )
    : uiLayerSequenceGenDesc(p,d)
{
}


uiSingleLayerSequenceGenDesc::UnitDisp::~UnitDisp()
{
    delete nm_;
}


void uiSingleLayerSequenceGenDesc::doDraw()
{
}


bool uiSingleLayerSequenceGenDesc::newDescReq()
{
    return false;
}


bool uiSingleLayerSequenceGenDesc::descEditReq()
{
    return false;
}


bool uiSingleLayerSequenceGenDesc::descRemoveReq()
{
    return false;
}

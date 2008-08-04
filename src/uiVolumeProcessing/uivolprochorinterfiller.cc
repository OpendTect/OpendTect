/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: uivolprochorinterfiller.cc,v 1.3 2008-08-04 22:31:16 cvskris Exp $";

#include "uivolprochorinterfiller.h"
#include "uimsg.h"
#include "volprochorinterfiller.h"

#include "ctxtioobj.h"
#include "emsurfacetr.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uivolprocchain.h"
#include "volprocchain.h"


namespace VolProc
{


void uiHorInterFiller::initClass()
{
    VolProc::uiChain::factory().addCreator( create, HorInterFiller::sKeyType() );
}    


uiHorInterFiller::uiHorInterFiller( uiParent* p, HorInterFiller* hf )
    : uiStepDialog( p, uiDialog::Setup( "Horizon Inter-Filler", 0, "helpid" ),
	            hf )
    , horinterfiller_( hf )
    , topctxt_( 0 )
    , bottomctxt_( 0 )
{
    usetopvalfld_ = new uiGenInput( this, "Use top value",
	    			    BoolInpSpec(hf->getTopHorizonID()) );
    usetopvalfld_->valuechanged.notify(mCB(this, uiHorInterFiller,updateFlds));
    usetopvalfld_->attach( alignedBelow, namefld_ );
    topctxt_ = mGetCtxtIOObj(EMHorizon3D,Surf);
    topctxt_->ctxt.forread = true;
    if ( hf->getTopHorizonID() )
	topctxt_->setObj( *hf->getTopHorizonID() );
    tophorfld_ = new uiIOObjSel( this, *topctxt_, "Top Horizon" );
    tophorfld_->attach( alignedBelow, usetopvalfld_ );
    topvalfld_ = new uiGenInput( this, "Top Value", 
	    			 FloatInpSpec( hf->getTopValue() ) );
    topvalfld_->attach( alignedBelow, tophorfld_ );
    
    usebottomvalfld_ = new uiGenInput( this, "Use bottom value",
	    			    BoolInpSpec(hf->getBottomHorizonID()) );
    usebottomvalfld_->valuechanged.notify(
	    mCB(this, uiHorInterFiller,updateFlds) );
    usebottomvalfld_->attach( alignedBelow, topvalfld_ );

    bottomctxt_ = mGetCtxtIOObj(EMHorizon3D,Surf);
    bottomctxt_->ctxt.forread = true;
    if ( hf->getBottomHorizonID() )
	bottomctxt_->setObj( *hf->getBottomHorizonID() );
    bottomhorfld_ = new uiIOObjSel( this, *bottomctxt_, "Bottom Horizon" );
    bottomhorfld_->attach( alignedBelow, usebottomvalfld_ );
    bottomvalfld_ = new uiGenInput( this, "Bottom Value",
	                            FloatInpSpec( hf->getBottomValue() ) ); 
    bottomvalfld_->attach( alignedBelow, bottomhorfld_ );

    updateFlds( 0 );
}


uiHorInterFiller::~uiHorInterFiller()
{
    delete topctxt_->ioobj;
    delete topctxt_;

    delete bottomctxt_->ioobj;
    delete bottomctxt_;
}


void uiHorInterFiller::updateFlds( CallBacker* )
{
    tophorfld_->display( usetopvalfld_->getBoolValue() );
    topvalfld_->display( usetopvalfld_->getBoolValue() );

    bottomhorfld_->display( usebottomvalfld_->getBoolValue() );
    bottomvalfld_->display( usebottomvalfld_->getBoolValue() );
}


uiStepDialog* uiHorInterFiller::create( uiParent* parent, Step* ps )
{
    mDynamicCastGet( HorInterFiller*, hf, ps );
    if ( !hf ) return 0;

    return new uiHorInterFiller( parent, hf );
}


bool uiHorInterFiller::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    tophorfld_->processInput();
    bottomhorfld_->processInput();

    if ( !usetopvalfld_->getBoolValue() && !usebottomvalfld_->getBoolValue() )
    {
	uiMSG().error( "Either top or bottom horizon must be entered" );
	return false;
    }
    
    if ( (usetopvalfld_->getBoolValue() && !tophorfld_->existingTyped()) ||
	 (usebottomvalfld_->getBoolValue() && !bottomhorfld_->existingTyped()) )
    {
	uiMSG().error("Non-existing horizon selected");
	return false;
    }

    const bool istopudf = usetopvalfld_->getBoolValue() &&
			  mIsUdf(topvalfld_->getfValue());
    const bool isbotudf = usebottomvalfld_->getBoolValue() &&
			  mIsUdf(bottomvalfld_->getfValue());
    if ( istopudf || isbotudf )
    {
	uiMSG().error("No value specified");
	return false;
    }
   
    if ( usetopvalfld_->getBoolValue() && usebottomvalfld_->getBoolValue() &&
	 tophorfld_->ctxtIOObj().ioobj->key()==
	 bottomhorfld_->ctxtIOObj().ioobj->key() )
    {
	uiMSG().error("Top and bottom horizons cannot be the same" );
	return false;
    }

    if ( usetopvalfld_->getBoolValue() )
    {
	const MultiID mid = tophorfld_->ctxtIOObj().ioobj->key();
	if ( !horinterfiller_->setTopHorizon( &mid, topvalfld_->getfValue() ) )
	{
	    uiMSG().error( "Could not set top horizon" );
	    return false;
	}
    }
    else
	horinterfiller_->setTopHorizon( 0, mUdf(float) );

    if ( usebottomvalfld_->getBoolValue() )
    {
	const MultiID mid = bottomhorfld_->ctxtIOObj().ioobj->key();
	if ( !horinterfiller_->setBottomHorizon( &mid,
		bottomvalfld_->getfValue()  ) )
	{
	    uiMSG().error( "Could not set bottom horizon" );
	    return false;
	}
    }
    else
	horinterfiller_->setBottomHorizon( 0, mUdf(float) );

    return true;
}


};//namespace


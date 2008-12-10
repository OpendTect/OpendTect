/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: uivolprochorinterfiller.cc,v 1.9 2008-12-10 18:24:14 cvskris Exp $";

#include "uivolprochorinterfiller.h"
#include "uimsg.h"
#include "volprochorinterfiller.h"

#include "ctxtioobj.h"
#include "emsurfacetr.h"
#include "mousecursor.h"
#include "survinfo.h"
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
    : uiStepDialog( p, uiDialog::Setup( HorInterFiller::sUserName(),
		HorInterFiller::sUserName(), mTODOHelpID ),
		    hf )
    , horinterfiller_( hf )
    , topctxt_( 0 )
    , bottomctxt_( 0 )
{
    const char* hortxt = "Horizon";
    usetophorfld_ = new uiGenInput( this, "Top boundary",
		    BoolInpSpec(hf->getTopHorizonID(),hortxt, "Survey top") );
    usetophorfld_->valuechanged.notify(mCB(this, uiHorInterFiller,updateFlds));
    usetophorfld_->attach( alignedBelow, namefld_ );
    topctxt_ = mGetCtxtIOObj(EMHorizon3D,Surf);
    topctxt_->ctxt.forread = true;
    if ( hf->getTopHorizonID() )
	topctxt_->setObj( *hf->getTopHorizonID() );
    tophorfld_ = new uiIOObjSel( this, *topctxt_, "Top Horizon" );
    tophorfld_->attach( alignedBelow, usetophorfld_ );
    topvalfld_ = new uiGenInput( this, "Top Value", 
	    			 FloatInpSpec( hf->getTopValue() ) );
    topvalfld_->attach( alignedBelow, tophorfld_ );
    
    usebottomhorfld_ = new uiGenInput( this, "Bottom boundary",
		BoolInpSpec(hf->getBottomHorizonID(),hortxt,"Survey bottom") );
    usebottomhorfld_->valuechanged.notify(
	    mCB(this, uiHorInterFiller,updateFlds) );
    usebottomhorfld_->attach( alignedBelow, topvalfld_ );

    bottomctxt_ = mGetCtxtIOObj(EMHorizon3D,Surf);
    bottomctxt_->ctxt.forread = true;
    if ( hf->getBottomHorizonID() )
	bottomctxt_->setObj( *hf->getBottomHorizonID() );
    bottomhorfld_ = new uiIOObjSel( this, *bottomctxt_, "Bottom Horizon" );
    bottomhorfld_->attach( alignedBelow, usebottomhorfld_ );

    usegradientfld_ = new uiGenInput( this, "Slope type",
	    BoolInpSpec(hf->usesGradient(), "Gradient" , "Bottom value" ));
    usegradientfld_->attach( alignedBelow, bottomhorfld_ );
    usegradientfld_->valuechanged.notify(mCB(this,uiHorInterFiller,updateFlds));

    BufferString gradientlabel = "Gradient [/";
    gradientlabel += SI().getZUnitString( false );
    gradientlabel += "]";
    gradientfld_ = new uiGenInput( this, gradientlabel.buf(),
	    			   FloatInpSpec( hf->getGradient() ) );
    gradientfld_->attach( alignedBelow, usegradientfld_ );

    bottomvalfld_ = new uiGenInput( this, "Bottom Value",
	                            FloatInpSpec( hf->getBottomValue() ) ); 
    bottomvalfld_->attach( alignedBelow, usegradientfld_ );

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
    tophorfld_->display( usetophorfld_->getBoolValue() );
    bottomhorfld_->display( usebottomhorfld_->getBoolValue() );
    bottomvalfld_->display( !usegradientfld_->getBoolValue() );
    gradientfld_->display( usegradientfld_->getBoolValue() );
}


uiStepDialog* uiHorInterFiller::create( uiParent* parent, Step* ps )
{
    mDynamicCastGet( HorInterFiller*, hf, ps );
    if ( !hf ) return 0;

    return new uiHorInterFiller( parent, hf );
}


bool uiHorInterFiller::acceptOK( CallBacker* cb )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    if ( mIsUdf( topvalfld_->getfValue() ) )
    {
	uiMSG().error("Top value must be set");
	return false;
    }

    if ( usegradientfld_->getBoolValue() && mIsUdf(gradientfld_->getfValue() ) )
    {
	uiMSG().error("Gradient must be set");
	return false;
    }

    if ( !usegradientfld_->getBoolValue() && mIsUdf(bottomvalfld_->getfValue()))
    {
	uiMSG().error("Bottom value must be set");
	return false;
    }

    if ( (usetophorfld_->getBoolValue() && !tophorfld_->existingTyped()) ||
	 (usebottomhorfld_->getBoolValue() && !bottomhorfld_->existingTyped()) )
    {
	uiMSG().error("Non-existing horizon selected");
	return false;
    }

    if ( usetophorfld_->getBoolValue() && usebottomhorfld_->getBoolValue() &&
	 tophorfld_->ctxtIOObj().ioobj->key()==
	 bottomhorfld_->ctxtIOObj().ioobj->key() )
    {
	uiMSG().error("Top and bottom horizons cannot be the same" );
	return false;
    }

    if ( usetophorfld_->getBoolValue() )
    {
	const MultiID mid = tophorfld_->ctxtIOObj().ioobj->key();
	if ( !horinterfiller_->setTopHorizon( &mid ) )
	{
	    uiMSG().error( "Could not set top horizon" );
	    return false;
	}
    }
    else
	horinterfiller_->setTopHorizon( 0 );

    if ( usebottomhorfld_->getBoolValue() )
    {
	const MultiID mid = bottomhorfld_->ctxtIOObj().ioobj->key();
	if ( !horinterfiller_->setBottomHorizon( &mid ) )
	{
	    uiMSG().error( "Could not set bottom horizon" );
	    return false;
	}
    }
    else
	horinterfiller_->setBottomHorizon( 0 );

    horinterfiller_->setTopValue( topvalfld_->getfValue() );
    horinterfiller_->setBottomValue( bottomvalfld_->getfValue() );
    horinterfiller_->setGradient( gradientfld_->getfValue() );
    horinterfiller_->useGradient( usegradientfld_->getBoolValue() );

    return true;
}


};//namespace


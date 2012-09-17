/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: uivolprochorinterfiller.cc,v 1.18 2011/08/24 13:19:43 cvskris Exp $";

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


uiHorInterFiller::uiHorInterFiller( uiParent* p, HorInterFiller* hf )
    : uiStepDialog( p, HorInterFiller::sFactoryDisplayName(), hf )
    , horinterfiller_( hf )
    , topctio_(mMkCtxtIOObj(EMHorizon3D))
    , bottomctio_(mMkCtxtIOObj(EMHorizon3D))
{
    setHelpID( "103.6.1" );

    const char* hortxt = "Horizon";
    topctio_->ctxt.forread = bottomctio_->ctxt.forread = true;

    usetophorfld_ = new uiGenInput( this, "Top boundary",
		    BoolInpSpec(hf->getTopHorizonID(),hortxt, "Survey top") );
    usetophorfld_->valuechanged.notify(mCB(this, uiHorInterFiller,updateFlds));
    if ( hf->getTopHorizonID() )
	topctio_->setObj( *hf->getTopHorizonID() );
    tophorfld_ = new uiIOObjSel( this, *topctio_, "Top Horizon" );
    tophorfld_->attach( alignedBelow, usetophorfld_ );
    topvalfld_ = new uiGenInput( this, "Top Value", 
	    			 FloatInpSpec( hf->getTopValue() ) );
    topvalfld_->attach( alignedBelow, tophorfld_ );
    
    usebottomhorfld_ = new uiGenInput( this, "Bottom boundary",
		BoolInpSpec(hf->getBottomHorizonID(),hortxt,"Survey bottom") );
    usebottomhorfld_->valuechanged.notify(
	    mCB(this, uiHorInterFiller,updateFlds) );
    usebottomhorfld_->attach( alignedBelow, topvalfld_ );

    if ( hf->getBottomHorizonID() )
	bottomctio_->setObj( *hf->getBottomHorizonID() );
    bottomhorfld_ = new uiIOObjSel( this, *bottomctio_, "Bottom Horizon" );
    bottomhorfld_->attach( alignedBelow, usebottomhorfld_ );

    usegradientfld_ = new uiGenInput( this, "Slope type",
	    BoolInpSpec(hf->usesGradient(), "Gradient" , "Bottom value" ));
    usegradientfld_->attach( alignedBelow, bottomhorfld_ );
    usegradientfld_->valuechanged.notify(mCB(this,uiHorInterFiller,updateFlds));

    BufferString gradientlabel = "Gradient [/";
    gradientlabel += SI().getZUnitString( false );
    gradientlabel += "]";
    gradientfld_ = new uiGenInput( this, gradientlabel.buf(), FloatInpSpec() );
    const float gradient = hf->getGradient();
    if ( !mIsUdf(gradient) )
	gradientfld_->setValue( gradient/SI().zFactor() );
    gradientfld_->attach( alignedBelow, usegradientfld_ );

    bottomvalfld_ = new uiGenInput( this, "Bottom Value",
	                            FloatInpSpec( hf->getBottomValue() ) ); 
    bottomvalfld_->attach( alignedBelow, usegradientfld_ );

    addNameFld( bottomvalfld_ );
    updateFlds( 0 );
}


uiHorInterFiller::~uiHorInterFiller()
{
    delete topctio_->ioobj; delete topctio_;
    delete bottomctio_->ioobj; delete bottomctio_;
}


void uiHorInterFiller::updateFlds( CallBacker* )
{
    tophorfld_->display( usetophorfld_->getBoolValue() );
    bottomhorfld_->display( usebottomhorfld_->getBoolValue() );
    bottomvalfld_->display( !usegradientfld_->getBoolValue() );
    gradientfld_->display( usegradientfld_->getBoolValue() );
}


uiStepDialog* uiHorInterFiller::createInstance( uiParent* parent, Step* ps )
{
    mDynamicCastGet( HorInterFiller*, hf, ps );
    if ( !hf ) return 0;

    return new uiHorInterFiller( parent, hf );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiHorInterFiller::acceptOK( CallBacker* cb )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    if ( mIsUdf( topvalfld_->getfValue() ) )
	mErrRet("Please provide the Top value")

    const bool usegradient = usegradientfld_->getBoolValue();
    const bool usetophor = usetophorfld_->getBoolValue();
    const bool usebothor = usebottomhorfld_->getBoolValue();

    if ( usegradient && mIsUdf(gradientfld_->getfValue() ) )
	mErrRet("Please provide the Gradient")
    else if ( !usegradient && mIsUdf(bottomvalfld_->getfValue()))
	mErrRet("Please provide the Bottom value")

    if ( (usetophor && !tophorfld_->commitInput()) )
	mErrRet("Please select the top horizon")
    if ( (usebothor && !bottomhorfld_->commitInput()) )
	mErrRet("Please select the bottom horizon")

    if ( usetophor && usebothor && tophorfld_->ctxtIOObj().ioobj->key()
			        == bottomhorfld_->ctxtIOObj().ioobj->key() )
	mErrRet("Top and bottom horizons cannot be the same")

    if ( !usetophor )
	horinterfiller_->setTopHorizon( 0 );
    else
    {
	const MultiID mid = tophorfld_->ctxtIOObj().ioobj->key();
	if ( !horinterfiller_->setTopHorizon( &mid ) )
	    mErrRet("Cannot use top horizon")
    }

    if ( !usebothor )
	horinterfiller_->setBottomHorizon( 0 );
    else
    {
	const MultiID mid = bottomhorfld_->ctxtIOObj().ioobj->key();
	if ( !horinterfiller_->setBottomHorizon( &mid ) )
	    mErrRet("Cannot use bottom horizon")
    }

    horinterfiller_->setTopValue( topvalfld_->getfValue() );
    horinterfiller_->setBottomValue( bottomvalfld_->getfValue() );
    horinterfiller_->setGradient( gradientfld_->getfValue()*SI().zFactor() );
    horinterfiller_->useGradient( usegradientfld_->getBoolValue() );

    return true;
}


};//namespace


/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolprochorinterfiller.h"
#include "volprochorinterfiller.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uivolprocchain.h"

#include "emsurfacetr.h"
#include "mousecursor.h"
#include "od_helpids.h"
#include "survinfo.h"
#include "volprocchain.h"


namespace VolProc
{

uiHorInterFiller::uiHorInterFiller( uiParent* p, HorInterFiller* hf, bool is2d )
    : uiStepDialog( p, HorInterFiller::sFactoryDisplayName(), hf, is2d )
    , horinterfiller_( hf )
{
    setHelpKey( mODHelpKey(mHorInterFillerHelpID) );

    usetophorfld_ = new uiGenInput( this, tr("Top boundary"),
	    BoolInpSpec(hf->getTopHorizonID(),uiStrings::sHorizon(),
		tr("Survey top")) );
    usetophorfld_->valueChanged.notify(mCB(this, uiHorInterFiller,updateFlds));

    IOObjContext ctxt = is2d ? mIOObjContext(EMHorizon2D)
			     : mIOObjContext(EMHorizon3D);
    tophorfld_ = new uiIOObjSel( this, ctxt, uiStrings::sTopHor() );
    tophorfld_->attach( alignedBelow, usetophorfld_ );
    topvalfld_ = new uiGenInput( this, tr("Top Value"),
				 FloatInpSpec( hf->getTopValue() ) );
    topvalfld_->attach( alignedBelow, tophorfld_ );

    usebottomhorfld_ = new uiGenInput( this, tr("Bottom boundary"),
	BoolInpSpec(hf->getBottomHorizonID(),uiStrings::sHorizon(),
			    tr("Survey bottom")) );
    usebottomhorfld_->valueChanged.notify(
	    mCB(this, uiHorInterFiller,updateFlds) );
    usebottomhorfld_->attach( alignedBelow, topvalfld_ );

    bottomhorfld_ = new uiIOObjSel( this, ctxt, uiStrings::sBottomHor() );
    bottomhorfld_->attach( alignedBelow, usebottomhorfld_ );

    usegradientfld_ = new uiGenInput( this, tr("Slope type"),
	    BoolInpSpec(hf->usesGradient(), tr("Gradient") ,
			tr("Bottom value") ));
    usegradientfld_->attach( alignedBelow, bottomhorfld_ );
    usegradientfld_->valueChanged.notify(mCB(this,uiHorInterFiller,updateFlds));

    const uiString gradientlabel = tr( "Gradient [/%1]")
	    .arg( SI().getUiZUnitString( false ) );
    gradientfld_ = new uiGenInput( this, gradientlabel, FloatInpSpec() );
    const float gradient = hf->getGradient();
    if ( !mIsUdf(gradient) )
	gradientfld_->setValue( gradient/SI().zDomain().userFactor() );
    gradientfld_->attach( alignedBelow, usegradientfld_ );

    bottomvalfld_ = new uiGenInput( this, tr("Bottom Value"),
				    FloatInpSpec( hf->getBottomValue() ) );
    bottomvalfld_->attach( alignedBelow, usegradientfld_ );

    addNameFld( bottomvalfld_ );

    if ( hf )
    {
	if ( hf->getTopHorizonID() )
	    tophorfld_->setInput( *hf->getTopHorizonID() );
	if ( hf->getBottomHorizonID() )
	    bottomhorfld_->setInput( *hf->getBottomHorizonID() );
    }

    updateFlds( 0 );
}


uiHorInterFiller::~uiHorInterFiller()
{
}


void uiHorInterFiller::updateFlds( CallBacker* )
{
    tophorfld_->display( usetophorfld_->getBoolValue() );
    bottomhorfld_->display( usebottomhorfld_->getBoolValue() );
    bottomvalfld_->display( !usegradientfld_->getBoolValue() );
    gradientfld_->display( usegradientfld_->getBoolValue() );
}


uiStepDialog* uiHorInterFiller::createInstance( uiParent* parent, Step* ps,
						bool is2d )
{
    mDynamicCastGet( HorInterFiller*, hf, ps );
    if ( !hf ) return 0;

    return new uiHorInterFiller( parent, hf, is2d );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiHorInterFiller::acceptOK( CallBacker* cb )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    if ( mIsUdf( topvalfld_->getFValue() ) )
	mErrRet(tr("Please provide the Top value"))

    const bool usegradient = usegradientfld_->getBoolValue();
    const bool usetophor = usetophorfld_->getBoolValue();
    const bool usebothor = usebottomhorfld_->getBoolValue();

    if ( usegradient && mIsUdf(gradientfld_->getFValue() ) )
	mErrRet(tr("Please provide the Gradient"))
    else if ( !usegradient && mIsUdf(bottomvalfld_->getFValue()))
	mErrRet(tr("Please provide the Bottom value"))

    const IOObj* ioobjtop = tophorfld_->ioobj( true );
    const IOObj* ioobjbot = bottomhorfld_->ioobj( true );
    if ( usetophor && !ioobjtop )
	mErrRet(tr("Please select the top horizon"))
    if ( usebothor && !ioobjbot )
	mErrRet(tr("Please select the bottom horizon"))

    if ( usetophor && usebothor && ioobjtop->key() == ioobjbot->key() )
	mErrRet(tr("Top and bottom horizons cannot be the same"))

    if ( !usetophor )
	horinterfiller_->setTopHorizon( nullptr );
    else
    {
	const MultiID mid = ioobjtop->key();
	if ( !horinterfiller_->setTopHorizon(&mid) )
	    mErrRet(tr("Cannot use top horizon"))
    }

    if ( !usebothor )
	horinterfiller_->setBottomHorizon( nullptr );
    else
    {
	const MultiID mid = ioobjbot->key();
	if ( !horinterfiller_->setBottomHorizon(&mid) )
	    mErrRet(tr("Cannot use bottom horizon"))
    }

    horinterfiller_->setTopValue( topvalfld_->getFValue() );
    horinterfiller_->setBottomValue( bottomvalfld_->getFValue() );
    horinterfiller_->setGradient(
			gradientfld_->getFValue()*SI().zDomain().userFactor() );
    horinterfiller_->useGradient( usegradientfld_->getBoolValue() );

    return true;
}

} // namespace VolProc

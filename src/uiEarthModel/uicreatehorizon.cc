/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicreatehorizon.h"

#include "uigeninput.h"
#include "uiiosurface.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uitaskrunner.h"

#include "emhorizon3d.h"
#include "ioobj.h"
#include "survinfo.h"


uiCreateHorizon::uiCreateHorizon( uiParent* p, bool is2d )
    : uiDialog(p,Setup(uiStrings::phrCreate(uiStrings::sHorizon(1)),mNoDlgTitle,
		       mODHelpKey(mCreateHorizonHelpID)).modal(false))
    , zfld_(0)
    , ready(this)
{
    setOkText( uiStrings::sCreate() );

    if ( is2d )
    {
	new uiLabel( this, tr("2D not implemented yet") );
	return;
    }

    uiString lbl = tr("Z Value ");
    lbl.append( SI().getUiZUnitString() );
    zfld_ = new uiGenInput( this, lbl, FloatInpSpec(SI().zRange(true).start
					*SI().zDomain().userFactor()) );

    uiSurfaceWrite::Setup swsu( EM::Horizon3D::typeStr(),
				EM::Horizon3D::userTypeStr() );
    swsu.withsubsel(true);
    outfld_ = new uiSurfaceWrite( this, swsu );
    outfld_->attach( alignedBelow, zfld_ );

    enableSaveButton( tr("Display after create") );
}


uiCreateHorizon::~uiCreateHorizon()
{
}


MultiID uiCreateHorizon::getSelID() const
{ return outfld_->selIOObj() ? outfld_->selIOObj()->key() : MultiID::udf(); }


bool uiCreateHorizon::acceptOK( CallBacker* )
{
    if ( !zfld_ ) return true;

    if ( !outfld_->processInput() )
	return false;

    float zval = zfld_->getFValue();
    if ( mIsUdf(zval) )
    {
	uiMSG().error( tr("Z value is undefined. Please enter a valid value") );
	return false;
    }

    zval /= SI().zDomain().userFactor();
    if ( !SI().zRange(false).includes(zval,false) )
    {
	const bool res =
		uiMSG().askContinue( tr("Z Value is outside survey Z range") );
	if ( !res ) return false;
    }

    uiPosSubSel* subsel = outfld_->getPosSubSel();
    TrcKeySampling hrg( true );
    if ( subsel && !subsel->isAll() )
	hrg = subsel->envelope().hsamp_;

    RefMan<EM::Horizon3D> hor3d = EM::Horizon3D::createWithConstZ( zval, hrg );
    if ( !hor3d ) return false;

    uiTaskRunner uitr( this );
    hor3d->setMultiID( outfld_->selIOObj()->key() );
    PtrMan<Executor> saver = hor3d->saver();
    if ( !saver || !uitr.execute(*saver) )
    {
	uiMSG().error( tr("Cannot save horizon") );
	return false;
    }

    ready.trigger();
    const bool res = uiMSG().askGoOn(tr("Horizon successfully created.\n\n"
				        "Do you want to create another one?"));
    raise();
    return !res;
}

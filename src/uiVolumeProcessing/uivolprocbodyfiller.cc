/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uivolprocbodyfiller.h"
#include "volprocbodyfiller.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uivolprocchain.h"

#include "embodytr.h"
#include "separstr.h"
#include "od_helpids.h"


namespace VolProc
{

void uiBodyFiller::initClass()
{
    SeparString str( sFactoryKeyword(), uiStepDialog::factory().cSeparator() );
    str += BodyFiller::sKeyOldType();

    uiStepDialog::factory().addCreator( createInstance, str,
				        sFactoryDisplayName() );
}


uiBodyFiller::uiBodyFiller( uiParent* p, BodyFiller* mp )
    : uiStepDialog( p, BodyFiller::sFactoryDisplayName(), mp )
    , bodyfiller_( mp )
{
    setHelpKey( mODHelpKey(mBodyFillerHelpID) );

    IOObjContext ctxt = mIOObjContext( EMBody );
    ctxt.forread = true;
    uinputselfld_ = new uiIOObjSel( this, ctxt, tr("Input body") );
    uinputselfld_->selectionDone.notify( mCB(this,uiBodyFiller,bodySel) );
    if ( mp )
	uinputselfld_->setInput( mp->getSurfaceID() );

    insidevaluefld_ = new uiGenInput( this, tr("Inside value"),
	    FloatInpSpec(mp ? mp->getInsideValue() : 1e30) );
    insidevaluefld_->attach( alignedBelow, uinputselfld_ );

    outsidevaluefld_ = new uiGenInput( this, tr("Outside value"),
	    FloatInpSpec(mp ? mp->getOutsideValue() : 1e30) );
    outsidevaluefld_->attach( alignedBelow, insidevaluefld_ );

    addNameFld( outsidevaluefld_ );
}


void uiBodyFiller::bodySel( CallBacker* )
{
    const IOObj* ioobj = uinputselfld_->ioobj();
    if ( ioobj )
	namefld_->setText( ioobj->name() );
}


uiStepDialog* uiBodyFiller::createInstance(uiParent* parent,Step* ps)
{
    mDynamicCastGet( BodyFiller*, mp, ps );
    if ( !mp ) return 0;

    return new uiBodyFiller( parent, mp );
}


bool uiBodyFiller::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    if ( mIsUdf(insidevaluefld_->getfValue()) &&
	 mIsUdf(outsidevaluefld_->getfValue()) )
    {
	uiMSG().error(tr("Set at lease one defined value"));
	return false;
    }

    const IOObj* ioobj = uinputselfld_->ioobj();
    if ( !ioobj )
    {
	uiMSG().error(tr("Invalid empty input body"));
	return false;
    }

    bodyfiller_->setSurface( ioobj->key() );
    bodyfiller_->setInsideOutsideValue( insidevaluefld_->getfValue(),
					outsidevaluefld_->getfValue() );

    return true;
}

} // namespace VolProc


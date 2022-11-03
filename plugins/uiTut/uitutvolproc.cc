/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutvolproc.h"

#include "uigeninput.h"
#include "uistepoutsel.h"


static const char* possibleoperations[] =
{
        "Square",
        "Shift",
	nullptr
};

namespace VolProc
{

uiTutOpCalculator::uiTutOpCalculator( uiParent* p, TutOpCalculator* opcalc )
    : uiStepDialog( p, TutOpCalculator::sFactoryDisplayName(), opcalc )
    , opcalc_(opcalc)
{
    typefld_ = new uiGenInput( this,
				tr("Choose action to execute"),
				StringListInpSpec( possibleoperations ) );
    mAttachCB( typefld_->valuechanged, uiTutOpCalculator::typeSel );

    shiftfld_ = new uiStepOutSel( this, false  );
    shiftfld_->attach( alignedBelow, typefld_ );

    addNameFld( shiftfld_ );

    mAttachCB( postFinalize(), uiTutOpCalculator::typeSel );
}


uiTutOpCalculator::~uiTutOpCalculator()
{
    detachAllNotifiers();
}


uiStepDialog* uiTutOpCalculator::createInstance( uiParent* parent, Step* ps,
						 bool is2d )
{
    mDynamicCastGet( TutOpCalculator*, opcalc, ps );
    if ( !opcalc )
	return nullptr;

    return new uiTutOpCalculator( parent, opcalc );
}


bool uiTutOpCalculator::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK(cb) )
	return false;

    const BufferString type( typefld_->text() );
    const BufferStringSet strs( possibleoperations );
    const int opidx = strs.indexOf( type );
    IOPar steppar;
    steppar.set( TutOpCalculator::sKeyTypeIndex(), opidx>0 ? opidx : 0 );
    if ( opidx == 1 )
    {
	steppar.set( sKey::StepInl(), shiftfld_->getBinID().inl() );
	steppar.set( sKey::StepCrl(), shiftfld_->getBinID().crl() );
    }

    return opcalc_->usePar( steppar );
}


void uiTutOpCalculator::typeSel( CallBacker* )
{
    const BufferString type( typefld_->text() );
    const BufferStringSet strs( possibleoperations );
    const int opidx = strs.indexOf(type);
    shiftfld_->display( opidx == 1 );
}

} // namespace VolProc

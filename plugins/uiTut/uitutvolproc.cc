/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : H. Huck
 * DATE     : March 2016
-*/

#include "uitutvolproc.h"

#include "keystrs.h"
#include "uigeninput.h"
#include "uistepoutsel.h"


static const char* possibleoperations[] =
{
	"Square",
	"Shift",
	0
};

namespace VolProc
{

uiTutOpCalculator::uiTutOpCalculator( uiParent* p, TutOpCalculator* opcalc,
				      bool is2d )
    : uiStepDialog( p, TutOpCalculator::sFactoryDisplayName(), opcalc, is2d )
    , opcalc_( opcalc )
{
    typefld_ = new uiGenInput( this,
				tr("Choose action to execute"),
				StringListInpSpec( possibleoperations ) );
    mAttachCB( typefld_->valuechanged, uiTutOpCalculator::typeSel );

    shiftfld_ = new uiStepOutSel( this, false  );
    shiftfld_->attach( alignedBelow, typefld_ );

    addNameFld( shiftfld_ );

    mAttachCB( postFinalise(), uiTutOpCalculator::typeSel );
}


uiTutOpCalculator::~uiTutOpCalculator()
{
    detachAllNotifiers();
}


uiStepDialog* uiTutOpCalculator::createInstance( uiParent* parent, Step* ps,
						 bool is2d )
{
    mDynamicCastGet( TutOpCalculator*, opcalc, ps );
    if ( !opcalc ) return 0;

    return new uiTutOpCalculator( parent, opcalc, is2d );
}


bool uiTutOpCalculator::acceptOK()
{
    if ( !uiStepDialog::acceptOK() )
	return false;

    const BufferString type( typefld_->text() );
    const BufferStringSet strs( possibleoperations );
    const int opidx = strs.indexOf(type);
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

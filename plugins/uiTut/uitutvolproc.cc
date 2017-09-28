/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : H. Huck
 * DATE     : March 2016
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uitutvolproc.h"

#include "survinfo.h"
#include "uimsg.h"
#include "tutvolproc.h"
#include "uigeninput.h"
#include "uistepoutsel.h"
#include "uivolprocchain.h"
#include "od_helpids.h"


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
    typefld_->valuechanged.notify( mCB(this,uiTutOpCalculator,typeSel) );

    shiftfld_ = new uiStepOutSel( this, false  );
    shiftfld_->attach( alignedBelow, typefld_ );

    addNameFld( shiftfld_ );

    typeSel(0);
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

    const FixedString type = typefld_->text();
    BufferStringSet strs( possibleoperations );
    const int opidx = strs.indexOf(type);
    opcalc_->setOpType( opidx>0 ? opidx : 0 );

    if ( opidx == 1 )
	opcalc_->setShift( shiftfld_->getBinID() );

    return true;
}


void uiTutOpCalculator::typeSel( CallBacker* )
{
    const FixedString type = typefld_->text();
    BufferStringSet strs( possibleoperations );
    const int opidx = strs.indexOf(type);
    shiftfld_->display( opidx == 1 );
}

} // namespace VolProc

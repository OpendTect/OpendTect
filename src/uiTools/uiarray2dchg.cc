/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jul 2006
________________________________________________________________________

-*/

#include "uiarray2dchg.h"
#include "uigeninput.h"
#include "uistepoutsel.h"
#include "od_helpids.h"

uiArr2DFilterPars::uiArr2DFilterPars( uiParent* p,
				      const Array2DFilterPars* prs )
    : uiGroup(p,"Array2D filter parameters")
{
    Array2DFilterPars pars; if ( prs ) pars = *prs;

    medianfld_ = new uiGenInput( this, tr("Filter type"),
				 BoolInpSpec(true,tr("Median"),tr("Average")) );
    medianfld_->setValue( pars.type_ == Stats::Median );

    stepoutfld_ = new uiStepOutSel( this, false, tr("Filter stepout") );
    stepoutfld_->setVal( true, pars.stepout_.row() );
    stepoutfld_->setVal( false, pars.stepout_.col() );
    stepoutfld_->attach( alignedBelow, medianfld_ );

    setHAlignObj( medianfld_ );
}


Array2DFilterPars uiArr2DFilterPars::getInput() const
{
    Array2DFilterPars pars;
    pars.type_ = medianfld_->getBoolValue() ? Stats::Median : Stats::Average;
    pars.stepout_ = stepoutfld_->getRowCol();
    return pars;
}


uiArr2DFilterParsDlg::uiArr2DFilterParsDlg( uiParent* p,
					    const Array2DFilterPars* prs )
    : uiDialog(p,uiDialog::Setup(tr("Filter parameters"),mNoDlgTitle,
				 mODHelpKey(mArr2DFilterParsDlgHelpID)))
{
    fld = new uiArr2DFilterPars( this, prs );
}

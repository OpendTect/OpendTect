/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jul 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiarray2dchg.h"
#include "uigeninput.h"
#include "uistepoutsel.h"

static const char* extrapolstrs[] =
{ "To outer border", "To convex hull", "No", 0 };

uiArr2DFilterPars::uiArr2DFilterPars( uiParent* p,
				      const Array2DFilterPars* prs )
    : uiGroup(p,"Array2D filter parameters")
{
    Array2DFilterPars pars; if ( prs ) pars = *prs;

    medianfld_ = new uiGenInput( this, "Filter type",
				 BoolInpSpec(true,"Median","Average") );
    medianfld_->setValue( pars.type_ == Stats::Median );

    stepoutfld_ = new uiStepOutSel( this, false, "Filter stepout" );
    stepoutfld_->setVal( true, pars.stepout_.row );
    stepoutfld_->setVal( false, pars.stepout_.col );
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
    : uiDialog(p,uiDialog::Setup("Filter parameters",
				 "Specify parameters for fill","104.0.7"))
{
    fld = new uiArr2DFilterPars( this, prs );
}

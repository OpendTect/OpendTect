/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jul 2006
 RCS:		$Id: uiarray2dchg.cc,v 1.1 2007-01-31 11:48:48 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiarray2dchg.h"
#include "uigeninput.h"
#include "uistepoutsel.h"


uiArr2DInterpolPars::uiArr2DInterpolPars( uiParent* p,
					  const Array2DInterpolatorPars* prs )
    : uiGroup(p,"Array2D interpolator parameters")
{
    Array2DInterpolatorPars pars; if ( prs ) pars = *prs;

    extrapolatefld_ = new uiGenInput( this, "Extrapolate outward",
	    			      BoolInpSpec() );
    extrapolatefld_->setValue( pars.extrapolate_ );

    bool isdef = pars.maxholesize_ > 0;
    maxholefld_ = new uiGenInput( this, "Maximum fill size (nodes)",
	    			  IntInpSpec(isdef ? pars.maxholesize_ : 20) );
    maxholefld_->setWithCheck( true ); maxholefld_->setChecked( isdef );
    maxholefld_->attach( alignedBelow, extrapolatefld_ );

    isdef = pars.maxnrsteps_ > 0;
    maxstepsfld_ = new uiGenInput( this, "Maximum interpolation steps",
	    			  IntInpSpec(isdef ? pars.maxnrsteps_ : 1) );
    maxstepsfld_->setWithCheck( true ); maxstepsfld_->setChecked( isdef );
    maxstepsfld_->attach( alignedBelow, maxholefld_ );

    setHAlignObj( extrapolatefld_ );
}


Array2DInterpolatorPars uiArr2DInterpolPars::getInput() const
{
    Array2DInterpolatorPars pars;
    pars.extrapolate_ = extrapolatefld_->getBoolValue();
    pars.maxholesize_ = maxholefld_->isChecked()
		       ? maxholefld_->getIntValue() : -1;
    if ( pars.maxholesize_ < 1 || mIsUdf(pars.maxholesize_) )
	pars.maxholesize_ = -1;
    pars.maxnrsteps_ = maxstepsfld_->isChecked()
		       ? maxstepsfld_->getIntValue() : -1;
    if ( pars.maxnrsteps_ < 1 || mIsUdf(pars.maxnrsteps_) )
	pars.maxnrsteps_ = -1;
    return pars;
}


uiArr2DInterpolParsDlg::uiArr2DInterpolParsDlg( uiParent* p,
				const Array2DInterpolatorPars* prs )
    : uiDialog(p,uiDialog::Setup("Interpolation parameters",
				 "Specify parameters for fill","0.0.0"))
{
    fld = new uiArr2DInterpolPars( this, prs );
}


uiArr2DFilterPars::uiArr2DFilterPars( uiParent* p,
				      const Array2DFilterPars* prs )
    : uiGroup(p,"Array2D filter parameters")
{
    Array2DFilterPars pars; if ( prs ) pars = *prs;

    medianfld_ = new uiGenInput( this, "Filter type",
				 BoolInpSpec("Median","Average") );
    medianfld_->setValue( pars.type_ == Stats::Median );

    stepoutfld_ = new uiStepOutSel( this, false, "Filter stepout" );
    stepoutfld_->setVal( true, pars.stepout_.r() );
    stepoutfld_->setVal( false, pars.stepout_.c() );
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
				 "Specify parameters for fill","0.0.0"))
{
    fld = new uiArr2DFilterPars( this, prs );
}

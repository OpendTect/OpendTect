/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jul 2006
 RCS:		$Id: uiarray2dchg.cc,v 1.6 2008-09-23 15:46:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiarray2dchg.h"
#include "uigeninput.h"
#include "uistepoutsel.h"

static const char* extrapolstrs[] =
{ "To outer border", "To convex hull", "No", 0 };

uiArr2DInterpolPars::uiArr2DInterpolPars( uiParent* p,
					  const Array2DInterpolatorPars* prs )
    : uiGroup(p,"Array2D interpolator parameters")
{
    Array2DInterpolatorPars pars; if ( prs ) pars = *prs;

    extrapolatefld_ = new uiGenInput( this, "Fill outward",
	    			      StringListInpSpec(extrapolstrs) );
    extrapolatefld_->setValue( (int)pars.filltype_ );

    bool isdef = pars.maxholesize_ > 0;
    maxholefld_ = new uiGenInput( this, "Maximum fill size (nodes)",
	    			  IntInpSpec(isdef ? pars.maxholesize_ : 20) );
    maxholefld_->setWithCheck( true ); maxholefld_->setChecked( isdef );
    maxholefld_->attach( alignedBelow, extrapolatefld_ );

    doextendfld_ = new uiGenInput( this, "Fill type",
		       BoolInpSpec(pars.useextension_,"Extension","Gridding") );
    doextendfld_->attach( alignedBelow, maxholefld_ );
    const CallBack cb( mCB(this,uiArr2DInterpolPars,doExtChg) );
    doextendfld_->valuechanged.notify( cb );

    isdef = pars.maxnrsteps_ > 0;
    maxstepsfld_ = new uiGenInput( this, "Maximum extension steps",
	    			  IntInpSpec(isdef ? pars.maxnrsteps_ : 1) );
    maxstepsfld_->setWithCheck( true ); maxstepsfld_->setChecked( isdef );
    maxstepsfld_->attach( alignedBelow, doextendfld_ );

    isdef = pars.srchrad_ > 0;
    srchradfld_ = new uiGenInput( this, "Search radius",
	    			  FloatInpSpec(isdef ? pars.srchrad_ : 1000) );
    srchradfld_->setWithCheck( true ); srchradfld_->setChecked( true );
    srchradfld_->attach( alignedBelow, doextendfld_ );

    setHAlignObj( extrapolatefld_ );
    mainwin()->finaliseDone.notify( cb );
}


void uiArr2DInterpolPars::doExtChg( CallBacker* )
{
    const bool isext = doextendfld_->getBoolValue();
    maxstepsfld_->display( isext );
    srchradfld_->display( !isext );
}


Array2DInterpolatorPars uiArr2DInterpolPars::getInput() const
{
    Array2DInterpolatorPars pars;

    pars.filltype_ = (Array2DInterpolatorPars::FillType)
	    		extrapolatefld_->getIntValue();
    pars.useextension_ = doextendfld_->getBoolValue();

    pars.maxholesize_ = maxholefld_->isChecked()
		       ? maxholefld_->getIntValue() : -1;
    if ( pars.maxholesize_ < 1 || mIsUdf(pars.maxholesize_) )
	pars.maxholesize_ = -1;

    if ( pars.useextension_ )
    {
	pars.maxnrsteps_ = maxstepsfld_->isChecked()
			   ? maxstepsfld_->getIntValue() : -1;
	if ( pars.maxnrsteps_ < 1 || mIsUdf(pars.maxnrsteps_) )
	    pars.maxnrsteps_ = -1;
    }
    else
    {
	pars.srchrad_ = srchradfld_->isChecked()
			   ? srchradfld_->getfValue() : -1;
	if ( pars.srchrad_ < 0 || mIsUdf(pars.srchrad_) )
	    pars.srchrad_ = -1;
    }

    return pars;
}


uiArr2DInterpolParsDlg::uiArr2DInterpolParsDlg( uiParent* p,
				const Array2DInterpolatorPars* prs )
    : uiDialog(p,uiDialog::Setup("Interpolation parameters",
				 "Specify parameters for fill","104.0.6"))
{
    fld = new uiArr2DInterpolPars( this, prs );
}


uiArr2DFilterPars::uiArr2DFilterPars( uiParent* p,
				      const Array2DFilterPars* prs )
    : uiGroup(p,"Array2D filter parameters")
{
    Array2DFilterPars pars; if ( prs ) pars = *prs;

    medianfld_ = new uiGenInput( this, "Filter type",
				 BoolInpSpec(true,"Median","Average") );
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
				 "Specify parameters for fill","104.0.7"))
{
    fld = new uiArr2DFilterPars( this, prs );
}

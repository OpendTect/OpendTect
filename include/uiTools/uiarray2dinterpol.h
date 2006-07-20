#ifndef uiarray2dinterpol_h
#define uiarray2dinterpol_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jul 2006
 RCS:		$Id: uiarray2dinterpol.h,v 1.1 2006-07-20 16:30:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigeninput.h"
#include "array2dinterpol.h"
#include "uidialog.h"


class uiArr2DInterpolPars : public uiGroup
{
public:

uiArr2DInterpolPars( uiParent* p, const Array2DInterpolatorPars* prs=0 )
    : uiGroup(p,"Array2D interpolator parameters")
{
    Array2DInterpolatorPars pars; if ( prs ) pars = *prs;

    extrapolatefld = new uiGenInput( this, "Extrapolate outward",
	    			      BoolInpSpec() );
    extrapolatefld->setValue( pars.extrapolate_ );

    bool isdef = pars.maxholesize_ > 0;
    maxholefld = new uiGenInput( this, "Maximum fill size (nodes)",
	    			  IntInpSpec(isdef ? pars.maxholesize_ : 20) );
    maxholefld->setWithCheck( true ); maxholefld->setChecked( isdef );
    maxholefld->attach( alignedBelow, extrapolatefld );

    isdef = pars.maxnrsteps_ > 0;
    maxstepsfld = new uiGenInput( this, "Maximum interpolation steps",
	    			  IntInpSpec(isdef ? pars.maxnrsteps_ : 1) );
    maxstepsfld->setWithCheck( true ); maxstepsfld->setChecked( isdef );
    maxstepsfld->attach( alignedBelow, maxholefld );

    setHAlignObj( extrapolatefld );
}


Array2DInterpolatorPars getInput()
{
    Array2DInterpolatorPars pars;
    pars.extrapolate_ = extrapolatefld->getBoolValue();
    pars.maxholesize_ = maxholefld->isChecked()
		       ? maxholefld->getIntValue() : -1;
    if ( pars.maxholesize_ < 1 || mIsUdf(pars.maxholesize_) )
	pars.maxholesize_ = -1;
    pars.maxnrsteps_ = maxstepsfld->isChecked()
		       ? maxstepsfld->getIntValue() : -1;
    if ( pars.maxnrsteps_ < 1 || mIsUdf(pars.maxnrsteps_) )
	pars.maxnrsteps_ = -1;
    return pars;
}

    uiGenInput*	extrapolatefld;
    uiGenInput*	maxholefld;
    uiGenInput*	maxstepsfld;

};


class uiArr2DInterpolParsDlg : public uiDialog
{
public:

uiArr2DInterpolParsDlg( uiParent* p, const Array2DInterpolatorPars* prs=0 )
    : uiDialog(p,uiDialog::Setup("Interpolation parameters",
				 "Specify parameters for fill","0.0.0"))
{
    fld = new uiArr2DInterpolPars( this, prs );
}

Array2DInterpolatorPars getInput()
{
    return fld->getInput();
}

    uiArr2DInterpolPars*	fld;

};


#endif

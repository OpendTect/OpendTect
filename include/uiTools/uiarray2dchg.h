#ifndef uiarray2dchg_h
#define uiarray2dchg_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jul 2006
 RCS:		$Id: uiarray2dchg.h,v 1.4 2009-01-08 07:07:01 cvsranojay Exp $
________________________________________________________________________

-*/

#include "array2dinterpol.h"
#include "array2dfilter.h"
#include "uigroup.h"
#include "uidialog.h"

class uiGenInput;
class uiStepOutSel;


mClass uiArr2DInterpolPars : public uiGroup
{
public:

				uiArr2DInterpolPars(uiParent*,
					const Array2DInterpolatorPars* p=0);

    Array2DInterpolatorPars	getInput() const;

protected:

    uiGenInput*	extrapolatefld_;
    uiGenInput*	maxholefld_;
    uiGenInput*	doextendfld_;
    uiGenInput*	maxstepsfld_;
    uiGenInput*	srchradfld_;

    void	doExtChg(CallBacker*);
};


mClass uiArr2DInterpolParsDlg : public uiDialog
{
public:

				uiArr2DInterpolParsDlg(uiParent*,
					const Array2DInterpolatorPars* p=0);

    Array2DInterpolatorPars	getInput() const
				{ return fld->getInput(); }

    uiArr2DInterpolPars*	fld;

};


mClass uiArr2DFilterPars : public uiGroup
{
public:

				uiArr2DFilterPars(uiParent*,
					const Array2DFilterPars* p=0);

    Array2DFilterPars	getInput() const;

protected:

    uiGenInput*		medianfld_;
    uiStepOutSel*	stepoutfld_;

};


mClass uiArr2DFilterParsDlg : public uiDialog
{
public:

			uiArr2DFilterParsDlg(uiParent*,
					     const Array2DFilterPars* p=0);

    Array2DFilterPars	getInput() const
			{ return fld->getInput(); }

    uiArr2DFilterPars*	fld;

};



#endif

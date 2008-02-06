#ifndef uiarray2dchg_h
#define uiarray2dchg_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jul 2006
 RCS:		$Id: uiarray2dchg.h,v 1.3 2008-02-06 14:33:52 cvsbert Exp $
________________________________________________________________________

-*/

#include "array2dinterpol.h"
#include "array2dfilter.h"
#include "uigroup.h"
#include "uidialog.h"

class uiGenInput;
class uiStepOutSel;


class uiArr2DInterpolPars : public uiGroup
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


class uiArr2DInterpolParsDlg : public uiDialog
{
public:

				uiArr2DInterpolParsDlg(uiParent*,
					const Array2DInterpolatorPars* p=0);

    Array2DInterpolatorPars	getInput() const
				{ return fld->getInput(); }

    uiArr2DInterpolPars*	fld;

};


class uiArr2DFilterPars : public uiGroup
{
public:

				uiArr2DFilterPars(uiParent*,
					const Array2DFilterPars* p=0);

    Array2DFilterPars	getInput() const;

protected:

    uiGenInput*		medianfld_;
    uiStepOutSel*	stepoutfld_;

};


class uiArr2DFilterParsDlg : public uiDialog
{
public:

			uiArr2DFilterParsDlg(uiParent*,
					     const Array2DFilterPars* p=0);

    Array2DFilterPars	getInput() const
			{ return fld->getInput(); }

    uiArr2DFilterPars*	fld;

};



#endif

#ifndef uiarray2dchg_h
#define uiarray2dchg_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jul 2006
 RCS:		$Id: uiarray2dchg.h,v 1.5 2009-04-23 18:08:50 cvskris Exp $
________________________________________________________________________

-*/

#include "array2dfilter.h"
#include "uigroup.h"
#include "uidialog.h"

class uiGenInput;
class uiStepOutSel;


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

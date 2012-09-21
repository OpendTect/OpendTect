#ifndef uiarray2dchg_h
#define uiarray2dchg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jul 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "array2dfilter.h"
#include "uigroup.h"
#include "uidialog.h"

class uiGenInput;
class uiStepOutSel;


mClass(uiTools) uiArr2DFilterPars : public uiGroup
{
public:

				uiArr2DFilterPars(uiParent*,
					const Array2DFilterPars* p=0);

    Array2DFilterPars	getInput() const;

protected:

    uiGenInput*		medianfld_;
    uiStepOutSel*	stepoutfld_;

};


mClass(uiTools) uiArr2DFilterParsDlg : public uiDialog
{
public:

			uiArr2DFilterParsDlg(uiParent*,
					     const Array2DFilterPars* p=0);

    Array2DFilterPars	getInput() const
			{ return fld->getInput(); }

    uiArr2DFilterPars*	fld;

};



#endif


#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2014
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"
#include "dbkey.h"

class uiCheckList;
class uiSpinBox;


/*!\brief creates probability density functions (Gaussian and Sampled) */

mExpClass(uiIo) uiProbDenFuncGen : public uiDialog
{ mODTextTranslationClass(uiProbDenFuncGen);
public:
			uiProbDenFuncGen(uiParent*);

    DBKey		newObjKey() const	{ return ioobjky_; }

protected:

    uiCheckList*	choicefld_;
    uiSpinBox*		nrdimfld_;

    DBKey		ioobjky_;

    void		choiceSel(CallBacker*);
    bool		acceptOK();

};

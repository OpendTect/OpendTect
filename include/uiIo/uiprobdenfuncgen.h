#ifndef uiprobdenfuncgen_h
#define uiprobdenfuncgen_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2014
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "multiid.h"

class uiCheckList;
class uiSpinBox;


/*!\brief creates probability density functions (Gaussian and Sampled) */

mExpClass(uiIo) uiProbDenFuncGen : public uiDialog
{
public:
			uiProbDenFuncGen(uiParent*);

    MultiID		newObjKey() const	{ return ioobjky_; }

protected:

    uiCheckList*	choicefld_;
    uiSpinBox*		nrdimfld_;

    MultiID		ioobjky_;

    void		choiceSel(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif

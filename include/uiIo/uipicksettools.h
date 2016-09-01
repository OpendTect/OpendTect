#ifndef uipicksettools_h
#define uipicksettools_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2016
________________________________________________________________________

-*/

#include "uipicksetsel.h"
#include "uidialog.h"
class uiIOObjSelGrp;


/*!\brief merges sets selected by the user. */

mExpClass(uiIo) uiMergePickSets : public uiDialog
{ mODTextTranslationClass(uiMergePickSets);
public:

			uiMergePickSets(uiParent*,MultiID&);

protected:

    uiIOObjSelGrp*	selfld_;
    uiIOObjSel*		outfld_;
    MultiID&		setid_;

    bool		acceptOK();
    RefMan<Pick::Set>	getMerged(IOPar&) const;

};


#endif

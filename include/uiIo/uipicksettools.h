#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2016
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uipicksetsel.h"
#include "uidialog.h"
class uiIOObjSelGrp;


/*!\brief merges sets selected by the user. */

mExpClass(uiIo) uiMergePickSets : public uiDialog
{ mODTextTranslationClass(uiMergePickSets);
public:

			uiMergePickSets(uiParent*,DBKey&);

protected:

    uiIOObjSelGrp*	selfld_;
    uiIOObjSel*		outfld_;
    DBKey&		setid_;

    bool		acceptOK();
    RefMan<Pick::Set>	getMerged(IOPar&) const;

};

#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"

class uiDataRootSel;

mExpClass(uiIo) uiFixInvalidDataRoot : public uiDialog
{ mODTextTranslationClass(uiFixInvalidDataRoot);
public:

			uiFixInvalidDataRoot(uiParent*);

protected:

    uiDataRootSel*	dirfld_;

    bool		acceptOK();
    bool		rejectOK();

    void		offerCreateSomeSurveys(const char*);

};

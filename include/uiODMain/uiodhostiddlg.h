#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		March 2020
 RCS:		$Id$
________________________________________________________________________
-*/

#include "uiodmainmod.h"
#include "uidialog.h"

class uiGenInput;

mExpClass(uiODMain) uiHostIDDlg : public uiDialog
{ mODTextTranslationClass(uiHostIDDlg)
public:
			uiHostIDDlg(uiParent*);
protected:
    void		copyCB(CallBacker*);

    uiGenInput*		hostidfld_;
    uiGenInput*		hostnmfld_;
    uiGenInput*		osfld_;
    uiGenInput*		usernmfld_;
};


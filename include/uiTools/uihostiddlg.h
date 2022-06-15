#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		April 2021
________________________________________________________________________
-*/

#include "uitoolsmod.h"
#include "uidialog.h"

class uiGenInput;
class uiLocalHostGrp;

mExpClass(uiTools) uiHostIDDlg : public uiDialog
{ mODTextTranslationClass(uiHostIDDlg)
public:
			uiHostIDDlg(uiParent*);
			~uiHostIDDlg();
protected:
    void		copyCB(CallBacker*);

    uiGenInput*		hostidfld_;
    uiLocalHostGrp*	localhostgrp_;
    uiGenInput*		timezonefld_;
    uiGenInput*		osfld_;
    uiGenInput*		productnmfld_;
    uiGenInput*		usernmfld_;
};


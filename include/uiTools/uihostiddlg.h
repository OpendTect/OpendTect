#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		April 2021
 RCS:		$Id$
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
protected:
    void		copyCB(CallBacker*);

    uiGenInput*		hostidfld_;
    uiLocalHostGrp*	localhostgrp_;
    uiGenInput*		osfld_;
    uiGenInput*		productnmfld_;
    uiGenInput*		usernmfld_;
};


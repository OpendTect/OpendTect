#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

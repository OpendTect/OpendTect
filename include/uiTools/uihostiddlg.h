#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidlggroup.h"

class uiInfoGroup;

mExpClass(uiTools) uiInformationDlg : public uiTabStackDlg
{ mODTextTranslationClass(uiInformationDlg)
public:
			uiInformationDlg(uiParent*);
			~uiInformationDlg();

private:
    void		finalizeCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    void		copyToClipboard();

    ObjectSet<uiInfoGroup> infodlggrp_;

};

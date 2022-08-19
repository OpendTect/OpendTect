#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisysadmmod.h"
#include "uidialog.h"
#include "sets.h"

class uiListBox;

/*!
\brief User interface for OpendTect System Administration color tabs.
*/

mExpClass(uiSysAdm) uiODSysAdmColorTabs : public uiDialog
{ mODTextTranslationClass(uiODSysAdmColorTabs);
public:

			uiODSysAdmColorTabs(uiParent*);
			~uiODSysAdmColorTabs();

protected:

    void		fillList(bool);
    void		rebuildList(int);
    void		addPush(CallBacker*);
    void		rmPush(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    uiListBox*		listfld;
};

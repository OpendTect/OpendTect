#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Jul 2006
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



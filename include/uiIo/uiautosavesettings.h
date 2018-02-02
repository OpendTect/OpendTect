#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2016
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uisettings.h"
class uiGenInput;


mExpClass(uiIo) uiAutoSaverSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiAutoSaverSettingsGroup);

public:

    mDecluiSettingsGroupPublicFns( uiAutoSaverSettingsGroup,
				   General, "AutoSave", "autosave",
				   tr("Auto-Save"), mTODOHelpKey )

			uiAutoSaverSettingsGroup(uiParent*,Settings&);


    static bool		autoAskRestore();

private:

    uiGenInput*		isactivefld_;
    uiGenInput*		usehiddenfld_;
    uiGenInput*		nrsecondsfld_;
    uiGenInput*		autoaskfld_;

    const bool		wasactive_;
    const bool		washidden_;
    const int		oldnrsecs_;

    void		isActiveCB(CallBacker*);

    virtual void	doCommit(uiRetVal&);

};

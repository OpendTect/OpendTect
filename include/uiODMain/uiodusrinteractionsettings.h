#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2003
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uisettings.h"
class uiComboBox;
class uiLabeledSpinBox;
class uiShortcutsList;


/*! Settings Group for keyboard interaction. */

mExpClass(uiODMain) uiKeyboardInteractionSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiKeyboardInteractionSettingsGroup);
public:

    mDecluiSettingsGroupPublicFns( uiKeyboardInteractionSettingsGroup,
				   Interaction, "Keyboard interaction",
				   "keyboard", tr("Keyboard Interaction"),
				   mTODOHelpKey )

			uiKeyboardInteractionSettingsGroup(uiParent*,Settings&);

protected:

    bool			initialenabvirtualkeyboard_;

    uiGenInput*			virtualkeyboardfld_;
    ObjectSet<uiComboBox>	stateboxes_;
    ObjectSet<uiComboBox>	keyboxes_;
    ObjectSet<uiLabeledSpinBox>	eikdboxes_;

    virtual void		doCommit(uiRetVal&);

};


/*! Settings Group for mouse interaction. */

mExpClass(uiODMain) uiMouseInteractionSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiMouseInteractionSettingsGroup);
public:

    mDecluiSettingsGroupPublicFns( uiMouseInteractionSettingsGroup,
				   Interaction, "Mouse interaction", "mouse",
				   tr("Mouse Interaction"),
				   mTODOHelpKey )

			uiMouseInteractionSettingsGroup(uiParent*,Settings&);

protected:

    uiGenInput*		keybindingfld_;
    uiGenInput*		wheeldirectionfld_;
    uiGenInput*		trackpadzoomspeedfld_;

    BufferString	initialkeybinding_;
    float		initialzoomfactor_;
    bool		initialmousewheelreversal_;

    virtual void	doCommit(uiRetVal&);

};

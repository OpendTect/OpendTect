#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          25/9/2000
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uisettings.h"
#include "bufstringset.h"
#include "fontdata.h"

class uiButton;
class uiButtonGroup;
class uiFont;
class uiLabel;
class uiLabeledComboBox;


mExpClass(uiTools) uiFontSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiFontSettingsGroup);
public:

    mDecluiSettingsGroupPublicFns( uiFontSettingsGroup,
				   LooknFeel, "Fonts", "font",
				   uiStrings::sFont(mPlural),
				   mODHelpKey(mSetFontsHelpID) )

			uiFontSettingsGroup(uiParent*,Settings&);

protected:

    uiButtonGroup*	butgrp_;
    ObjectSet<uiButton> buttons_;
    ObjectSet<uiLabel>	lbls_;
    TypeSet<FontData::StdSz> types_;

    void		addButton(FontData::StdSz,uiString infotxt);
    void		addResetButton();
    void		butPushed(CallBacker*);
    void		resetCB(CallBacker*);

    virtual void	doCommit(uiRetVal&)		{}
    virtual void	doRollBack();

};


mExpClass(uiTools) uiSelFonts : public uiDialog
{ mODTextTranslationClass(uiSelFonts)
public:

			uiSelFonts(uiParent*,const uiString& title,
				   const HelpKey&);
			~uiSelFonts();

    void		add(const char* str,const char* stdfontkey);

    const char*		resultFor(const char* str);

protected:

    ObjectSet<uiLabeledComboBox>	sels_;
    BufferStringSet			ids_;

};

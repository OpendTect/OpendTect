#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
	    mDefaultFactoryInstantiation2Param(
		uiSettingsGroup,
		uiFontSettingsGroup,
		uiParent*,Settings&,
		"Fonts",
		mToUiStringTodo(sFactoryKeyword()))

			uiFontSettingsGroup(uiParent*,Settings&);

    bool		acceptOK() override;
    HelpKey		helpKey() const override;

protected:

    uiButtonGroup*	butgrp_;
    ObjectSet<uiButton> buttons_;
    ObjectSet<uiLabel>	lbls_;
    TypeSet<FontData::StdSz> types_;

    void		addButton(FontData::StdSz,uiString infotxt);
    void		addResetButton();
    void		butPushed(CallBacker*);
    void		resetCB(CallBacker*);
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

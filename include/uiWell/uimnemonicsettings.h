#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uisettings.h"


class uiPushButton;
class uiListBox;
class uiMnemonicProperties;


mExpClass(uiWell) uiMnemonicSettings : public uiSettingsGroup
{
mODTextTranslationClass(uiMnemonicSettings)
public:

    mDefaultFactoryInstantiation2Param( uiSettingsGroup,
					uiMnemonicSettings,
					uiParent*,Settings&,
					"Mnemonics",
					mToUiStringTodo(sFactoryKeyword()));

protected:
			uiMnemonicSettings(uiParent*, Settings&);
			~uiMnemonicSettings();

private:
    bool		acceptOK() override;
    HelpKey		helpKey() const override;
    void		mnemSelCB(CallBacker*);
    void		updateCB(CallBacker*);
    void		applyCB(CallBacker*);
    void		removeCB(CallBacker*);
    void		importCB(CallBacker*);
    void		exportCB(CallBacker*);
    void		fillMnemonicList();

    uiListBox*			mnemonicsfld_;
    uiMnemonicProperties*	mnempropsfld_;
    uiPushButton*		applybut_;
    uiPushButton*		removebut_;
    uiPushButton*		importbut_;
    uiPushButton*		exportbut_;
    IOPar			mnemsetts_;

};

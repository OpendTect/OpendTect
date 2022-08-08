#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra/Bert
 Date:          Jan 2002/Mar 2014
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "uisettings.h"
#include "bufstringset.h"

class uiBatchJobDispatcherSel;
class uiGenInput;
class uiLabel;
class uiListBox;
class Settings;

mExpClass(uiIo) uiProcSettings : public uiSettingsGroup
{ mODTextTranslationClass(uiProcSettings);
public:
			mDefaultFactoryInstantiation2Param(
		uiSettingsGroup,
		uiProcSettings,
		uiParent*,Settings&,
		"Processing",
		mToUiStringTodo(sFactoryKeyword()))

			uiProcSettings(uiParent*,Settings&);
    bool		acceptOK() override;
    HelpKey		helpKey() const override;

protected:

    uiGenInput*		nrinlfld_;
    uiGenInput*		clusterfld_;

    int			nrinl_;
    bool		enabclusterproc_;

};


mExpClass(uiIo) uiStartBatchJobDialog : public uiDialog
{ mODTextTranslationClass(uiStartBatchJobDialog);
public:

				uiStartBatchJobDialog(uiParent*);
				~uiStartBatchJobDialog();

protected:

    BufferStringSet		filenames_;
    bool			canresume_;

    uiListBox*			jobsfld_;
    uiBatchJobDispatcherSel*	batchfld_;
    uiGenInput*			resumefld_;
    uiLabel*			invalidsellbl_;
    uiButton*			vwfilebut_;
    uiButton*			rmfilebut_;

    void			fillList(CallBacker*);
    void			itmSel(CallBacker*);
    void			launcherSel(CallBacker*);
    void			viewFile(CallBacker*);
    void			rmFile(CallBacker*);
    bool			acceptOK(CallBacker*) override;

    bool			canRun() const;
    void			setButSens();

private:

    static const uiString	sKeyNoParFiles();
};



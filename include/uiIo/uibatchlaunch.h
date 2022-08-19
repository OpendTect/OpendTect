#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

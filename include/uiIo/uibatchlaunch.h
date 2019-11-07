#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra/Bert
 Date:          Jan 2002/Mar 2014
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"
#include "uisettings.h"
#include "bufstringset.h"
#include "uistrings.h"

class uiBatchJobDispatcherSel;
class uiGenInput;
class uiLabel;
class uiListBox;
class Settings;


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
    bool			acceptOK();

    bool			canRun() const;
    void			setButSens();

private:

    static const uiString	sKeyNoParFiles();

};


mExpClass(uiIo) uiProcSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiProcSettingsGroup);
public:

    mDecluiSettingsGroupPublicFns( uiProcSettingsGroup,
				   General, "Processing", "batchprogs",
				   uiStrings::sProcessing(),
				   mODHelpKey(mProcSettingsHelpID) )

			uiProcSettingsGroup(uiParent*,Settings&);

protected:

    const int		initialnrinl_;
    const bool		initialcpenabled_;

    uiGenInput*		nrinlfld_;
    uiGenInput*		clusterfld_;

    bool		clusterProcEnabled() const;

    virtual void	doCommit(uiRetVal&);

};

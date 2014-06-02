#ifndef uibatchlaunch_h
#define uibatchlaunch_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra/Bert
 Date:          Jan 2002/Mar 2014
 RCS:           $Id$
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
				sFactoryKeyword())

			uiProcSettings(uiParent*,Settings&);
    bool		acceptOK();
    HelpKey		helpKey() const;

protected:

    uiGenInput*		nrinlfld_;
    uiGenInput*		clusterfld_;

    int			nrinl_;
    bool		enabclusterproc_;
};


mExpClass(uiIo) uiStartBatchJobDialog : public uiDialog
{
public:

				uiStartBatchJobDialog(uiParent*);

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
    bool			acceptOK(CallBacker*);

    bool			canRun() const;
    void			setButSens();

};


#endif

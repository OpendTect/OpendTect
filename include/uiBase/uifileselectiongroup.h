#pragma once

/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2017
 ________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uifileselector.h"
#include "uigroup.h"

class uiListBox;
class uiComboBox;
class uiLineEdit;
class uiToolButton;


mExpClass(uiBase) uiFileSelectionGroup : public uiGroup
{ mODTextTranslationClass(uiFileSelectionGroup);
public:

    typedef uiFileSelectorSetup	Setup;

			uiFileSelectionGroup(uiParent*,const Setup&);

    BufferString	fileName() const;
    void		getSelected(BufferStringSet&) const;
    const char*		protocol() const;

    void		setFileName(const char*);
    void		setFileNames(const BufferStringSet&);

    Notifier<uiFileSelectionGroup>	selChange;


protected:

    uiListBox*		dirselfld_;
    uiListBox*		leafselfld_;
    uiComboBox*		fsaselfld_;
    uiComboBox*		filtfld_;
    uiComboBox*		sortfld_ = nullptr;
    uiLineEdit*		fnmfld_;
    uiLineEdit*		newfoldernamefld_;
    uiToolButton*	newfolderbut_;

    Setup		setup_;
    BufferStringSet	fsakeys_;

    void		dirSelCB(CallBacker*);
    void		fnmSelCB(CallBacker*);
    void		filtChgCB(CallBacker*);
    void		sortChgCB(CallBacker*);
    void		fnmChgCB(CallBacker*);
    void		newFolderCrReqCB(CallBacker*);

private:

    void		createFSAStuff(uiGroup*);
    uiGroup*		createMainGroup();

};

#pragma once

/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2010
 ________________________________________________________________________

-*/

#include "uibasemod.h"
#include "fileselector.h"

class uiListBox;
class uiComboBox;
class uiLineEdit;
class uiButtonGroup;


mExpClass(uiBase) uiFileSelectionGroup : public uiGroup
{ mODTextTranslationClass(uiFileSelectionGroup);
public:

    typedef uiFileSelectorSetup	Setup;

			uiFileSelectionGroup(uiParent*,const Setup&);

    BufferString	fileName() const;
    void		getSelected(BufferStringSet&) const;

    Notifier<uiFileSelectionGroup>	selChange;


protected:

    uiListBox*		dirselfld_;
    uiListBox*		leafselfld_;
    uiComboBox*		filtfld_;
    uiComboBox*		sortfld_;
    uiLineEdit*		fnmfld_;

    Setup		setup_;

    void		dirSelCB(CallBacker*);
    void		fnmSelCB(CallBacker*);
    void		filtChgCB(CallBacker*);
    void		sortChgCB(CallBacker*);
    void		fnmChgCB(CallBacker*);

};

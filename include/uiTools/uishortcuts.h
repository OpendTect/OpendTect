#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          06/12/2005
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "iopar.h"

class uiComboBox;
class uiShortcutsList;
class uiLabeledSpinBox;

/*! \brief: setup a dialog where the user can select which key will be used as 
  shortcuts.
*/

mExpClass(uiTools) uiShortcutsDlg : public uiDialog
{ mODTextTranslationClass(uiShortcutsDlg);
public:
		    	uiShortcutsDlg(uiParent*,const char* selkey);
		    	~uiShortcutsDlg();

protected:

    bool		acceptOK(CallBacker*);

    ObjectSet<uiComboBox> stateboxes_;
    ObjectSet<uiComboBox> keyboxes_;
    ObjectSet<uiLabeledSpinBox> lblspinboxes_;
    uiShortcutsList&	scl_;
};


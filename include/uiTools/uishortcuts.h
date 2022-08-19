#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    bool		acceptOK(CallBacker*) override;

    ObjectSet<uiComboBox> stateboxes_;
    ObjectSet<uiComboBox> keyboxes_;
    ObjectSet<uiLabeledSpinBox> lblspinboxes_;
    uiShortcutsList&	scl_;
};

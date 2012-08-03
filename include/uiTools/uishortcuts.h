#ifndef uishortcuts_h
#define uishortcuts_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          06/12/2005
 RCS:           $Id: uishortcuts.h,v 1.9 2012-08-03 13:01:15 cvskris Exp $
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

mClass(uiTools) uiShortcutsDlg : public uiDialog
{
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

#endif


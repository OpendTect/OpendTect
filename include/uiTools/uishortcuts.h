#ifndef uishortcuts_h
#define uishortcuts_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          06/12/2005
 RCS:           $Id: uishortcuts.h,v 1.8 2011/08/04 16:36:02 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "iopar.h"

class uiComboBox;
class uiShortcutsList;
class uiLabeledSpinBox;

/*! \brief: setup a dialog where the user can select which key will be used as 
  shortcuts.
*/

mClass uiShortcutsDlg : public uiDialog
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

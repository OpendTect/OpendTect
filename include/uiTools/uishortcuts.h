#ifndef uishortcuts_h
#define uishortcuts_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          06/12/2005
 RCS:           $Id: uishortcuts.h,v 1.7 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "iopar.h"

class uiComboBox;
class uiShortcutsList;

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
    uiShortcutsList&	scl_;
};

#endif

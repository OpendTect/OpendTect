#ifndef uishortcuts_h
#define uishortcuts_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          06/12/2005
 RCS:           $Id: uishortcuts.h,v 1.6 2009-01-08 07:07:01 cvsranojay Exp $
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

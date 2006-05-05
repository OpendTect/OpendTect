#ifndef uishortcuts_h
#define uishortcuts_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          06/12/2005
 RCS:           $Id: uishortcuts.h,v 1.4 2006-05-05 14:43:14 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "iopar.h"

class uiTable;
class uiListBox;
class CallBacker;

/*! \brief: setup a dialog where the user can select which key will be used as 
  shortcuts.
*/

class uiShortcutsDlg : public uiDialog
{
public:
		    	uiShortcutsDlg(uiParent*);

protected:
    void		fillTable();
    void		getKeyValues(int,int&,int&,IOPar*) const;
    int			getUIValue(int,int) const;
    void		writeToSettings();
    void		shortcutsDlgClosed(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		removeShortcutsOldStyle();//compat with old files...

    uiTable*            shortcutskeys_;
};

#endif

#ifndef uishortcuts_h
#define uishortcuts_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          06/12/2005
 RCS:           $Id: uishortcuts.h,v 1.2 2006-01-31 16:56:54 cvshelene Exp $
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
    void		readFileValues();
    void		getKeyValues(int,int&,int&) const;
    int			getUIValue(int,int) const;
    void		fillPar(IOPar&) const;
    void		shortcutsDlgClosed(CallBacker*);

    uiTable*            shortcutskeys_;

    IOPar		pars_;
};

#endif

#ifndef uishortcuts_h
#define uishortcuts_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          06/12/2005
 RCS:           $Id: uishortcuts.h,v 1.3 2006-02-17 17:47:32 cvshelene Exp $
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
    bool		readFileValues();
    void		getKeyValues(int,int&,int&,bool) const;
    int			getUIValue(int,int) const;
    void		fillPar() const;
    void		shortcutsDlgClosed(CallBacker*);

    uiTable*            shortcutskeys_;

    IOPar		pars_;
};

#endif

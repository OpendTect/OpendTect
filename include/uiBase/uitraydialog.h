#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		March 2022
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "uidialog.h"

/*!\brief Stand-alone dialog window associated with a system tray icon.

It is meant to be the base class for dialog windows associated with long running
applications.

*/

class uiSystemTrayIcon;
class uiMenu;

mExpClass(uiBase) uiTrayDialog : public uiDialog
{ mODTextTranslationClass(uiTrayDialog)
public:
			uiTrayDialog(uiParent*,const Setup&);
			~uiTrayDialog();

    void		setTrayIcon(const char*);
    void		setTrayToolTip(const uiString&);

protected:
    uiSystemTrayIcon*	trayicon_;

    void		makeTrayMenu();
    virtual void	addApplicationTrayMenuItems(uiMenu*)	{}

    void		initUI(CallBacker*);
    virtual void	showCB(CallBacker*);
    virtual void	exitCB(CallBacker*);

    void		setInTray(bool yn);
};

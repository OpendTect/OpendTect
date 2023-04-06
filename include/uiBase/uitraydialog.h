#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			mOD_DisableCopy(uiTrayDialog)

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

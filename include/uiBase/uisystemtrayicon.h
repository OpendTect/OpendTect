#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "callback.h"

mFDQtclass(QSystemTrayIcon)
mFDQtclass(QSystemTrayIconMessenger)

class uiIcon;
class uiMenu;
class uiString;

mExpClass(uiBase) uiSystemTrayIcon : public CallBacker
{
public:

			uiSystemTrayIcon(const uiIcon&);
			~uiSystemTrayIcon();

    void		setIcon(const uiIcon&);
    void		setToolTip(const uiString&);
    void		setMenu(uiMenu*); // becomes mine
    void		show();
    void		hide();

    Notifier<uiSystemTrayIcon>	messageClicked;
    Notifier<uiSystemTrayIcon>	clicked;
    Notifier<uiSystemTrayIcon>	rightClicked;
    Notifier<uiSystemTrayIcon>	middleClicked;
    Notifier<uiSystemTrayIcon>	doubleClicked;

    static bool		isSystemTrayAvailable();

protected:

    mQtclass(QSystemTrayIcon*)		qsystemtrayicon_;
    mQtclass(QSystemTrayIconMessenger*) messenger_;

    uiMenu*		menu_;
};


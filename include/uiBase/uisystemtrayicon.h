#ifndef uisystemtrayicon_h
#define uisystemtrayicon_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "callback.h"

mFDQtclass(QSystemTrayIcon)
mFDQtclass(QSystemTrayIconMessenger)
class ioPixmap;

mExpClass(uiBase) uiSystemTrayIcon : public CallBacker
{
public:

    			uiSystemTrayIcon(const ioPixmap&);
			~uiSystemTrayIcon();

    void		setPixmap(const ioPixmap&);
    void		setToolTip(const char*);
    void		show();
    void		hide();

    Notifier<uiSystemTrayIcon>	messageClicked;
    Notifier<uiSystemTrayIcon>	clicked;
    Notifier<uiSystemTrayIcon>	rightClicked;
    Notifier<uiSystemTrayIcon>	middleClicked;
    Notifier<uiSystemTrayIcon>	doubleClicked;

protected:

    mQtclass(QSystemTrayIcon*)		qsystemtrayicon_;
    mQtclass(QSystemTrayIconMessenger*) messenger_;

    int					action_;
};

#endif


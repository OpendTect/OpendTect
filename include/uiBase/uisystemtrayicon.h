#ifndef uisystemtrayicon_h
#define uisystemtrayicon_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: uisystemtrayicon.h,v 1.1 2010/09/20 06:07:45 cvsnanne Exp $
________________________________________________________________________

-*/

#include "callback.h"

class ioPixmap;
class QSystemTrayIcon;
class QSystemTrayIconMessenger;

mClass uiSystemTrayIcon : public CallBacker
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

    QSystemTrayIcon*	qsystemtrayicon_;
    QSystemTrayIconMessenger* messenger_;

    int			action_;
};

#endif

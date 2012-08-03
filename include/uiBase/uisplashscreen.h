#ifndef uisplashscreen_h
#define uisplashscreen_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: uisplashscreen.h,v 1.4 2012-08-03 13:00:53 cvskris Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "commondefs.h"

class ioPixmap;
class uiMainWin;
class QSplashScreen;

mClass(uiBase) uiSplashScreen
{
public:
    				uiSplashScreen(const ioPixmap&);
				~uiSplashScreen();

    void			show();
    void			finish(uiMainWin*);
    void			showMessage(const char*);

protected:
    QSplashScreen*		qsplashscreen_;
};

#endif


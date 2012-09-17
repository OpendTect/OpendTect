#ifndef uisplashscreen_h
#define uisplashscreen_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: uisplashscreen.h,v 1.3 2009/07/22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "commondefs.h"

class ioPixmap;
class uiMainWin;
class QSplashScreen;

mClass uiSplashScreen
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

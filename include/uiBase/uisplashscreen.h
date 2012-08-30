#ifndef uisplashscreen_h
#define uisplashscreen_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: uisplashscreen.h,v 1.5 2012-08-30 06:05:55 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "commondefs.h"

class ioPixmap;
class uiMainWin;
mFDQtclass(QSplashScreen)

mClass(uiBase) uiSplashScreen
{
public:
    				uiSplashScreen(const ioPixmap&);
				~uiSplashScreen();

    void			show();
    void			finish(uiMainWin*);
    void			showMessage(const char*);

protected:
    mQtclass(QSplashScreen*)	qsplashscreen_;
};

#endif


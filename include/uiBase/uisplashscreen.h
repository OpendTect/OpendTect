#ifndef uisplashscreen_h
#define uisplashscreen_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: uisplashscreen.h,v 1.2 2009-01-09 04:26:14 cvsnanne Exp $
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

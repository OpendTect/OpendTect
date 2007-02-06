#ifndef uisplashscreen_h
#define uisplashscreen_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: uisplashscreen.h,v 1.1 2007-02-06 21:27:05 cvsnanne Exp $
________________________________________________________________________

-*/

class ioPixmap;
class uiMainWin;
class QSplashScreen;

class uiSplashScreen
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

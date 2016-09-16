#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2006
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "gendefs.h"

class uiPixmap;
class uiMainWin;
mFDQtclass(QSplashScreen)

mExpClass(uiBase) uiSplashScreen
{
public:
				uiSplashScreen(const uiPixmap&);
				~uiSplashScreen();

    void			show();
    void			finish(uiMainWin*);
    void			showMessage(const char*);

protected:
    mQtclass(QSplashScreen*)	qsplashscreen_;
};

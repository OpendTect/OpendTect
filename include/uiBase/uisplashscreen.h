#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

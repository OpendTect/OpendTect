/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: uisplashscreen.cc,v 1.1 2007-02-06 21:26:50 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uisplashscreen.h"
#include "uimainwin.h"
#include "pixmap.h"

#include <QSplashScreen>


uiSplashScreen::uiSplashScreen( const ioPixmap& pm )
{
    qsplashscreen_ = new QSplashScreen( *pm.Pixmap() );
}


uiSplashScreen::~uiSplashScreen()
{ delete qsplashscreen_; }

void uiSplashScreen::show()
{ qsplashscreen_->show(); }

void uiSplashScreen::finish( uiMainWin* mw )
{ qsplashscreen_->finish( mw->qWidget() ); }

void uiSplashScreen::showMessage( const char* msg )
{ qsplashscreen_->showMessage( msg ); }

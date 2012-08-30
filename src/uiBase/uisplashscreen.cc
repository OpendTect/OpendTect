/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uisplashscreen.cc,v 1.9 2012-08-30 07:52:52 cvsnageswara Exp $";


#include "uisplashscreen.h"
#include "uimainwin.h"
#include "pixmap.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QSplashScreen>


uiSplashScreen::uiSplashScreen( const ioPixmap& pm )
{
    mQtclass(QDesktopWidget*) qdw = mQtclass(QApplication)::desktop();
    mQtclass(QWidget*) parent = qdw->screen( qdw->primaryScreen() );
    qsplashscreen_ = new mQtclass(QSplashScreen)( parent, *pm.qpixmap() );
}


uiSplashScreen::~uiSplashScreen()
{ delete qsplashscreen_; }

void uiSplashScreen::show()
{ qsplashscreen_->show(); }

void uiSplashScreen::finish( uiMainWin* mw )
{ qsplashscreen_->finish( mw->qWidget() ); }

void uiSplashScreen::showMessage( const char* msg )
{ qsplashscreen_->showMessage( msg ); }

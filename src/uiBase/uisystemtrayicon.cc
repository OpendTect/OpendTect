/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uisystemtrayicon.h"
#include "i_qsystemtrayicon.h"

#include "pixmap.h"


uiSystemTrayIcon::uiSystemTrayIcon( const ioPixmap& pm )
    : action_(-1)
    , messageClicked(this)
    , clicked(this)
    , rightClicked(this)
    , middleClicked(this)
    , doubleClicked(this)
{
    qsystemtrayicon_ = new mQtclass(QSystemTrayIcon)();
    messenger_ = new mQtclass(QSystemTrayIconMessenger)( qsystemtrayicon_,
	    						 this );
    setPixmap( pm );
}


uiSystemTrayIcon::~uiSystemTrayIcon()
{
    delete messenger_;
    delete qsystemtrayicon_;
}


void uiSystemTrayIcon::setPixmap( const ioPixmap& pm )
{
    mQtclass(QIcon) qicon; 
    if ( pm.qpixmap() ) qicon = mQtclass(QIcon)( *pm.qpixmap() );
    qsystemtrayicon_->setIcon( qicon );
}


void uiSystemTrayIcon::setToolTip( const char* tt )
{ qsystemtrayicon_->setToolTip( tt ); }

void uiSystemTrayIcon::show()
{ qsystemtrayicon_->show(); }

void uiSystemTrayIcon::hide()
{ qsystemtrayicon_->hide(); }

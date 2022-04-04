/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
________________________________________________________________________

-*/


#include "uisystemtrayicon.h"
#include "i_qsystemtrayicon.h"

#include "uiicon.h"
#include "uimenu.h"
#include "uistring.h"

#include "q_uiimpl.h"

mUseQtnamespace

uiSystemTrayIcon::uiSystemTrayIcon( const uiIcon& icon )
    : menu_(0)
    , messageClicked(this)
    , clicked(this)
    , rightClicked(this)
    , middleClicked(this)
    , doubleClicked(this)
{
    qsystemtrayicon_ = new QSystemTrayIcon( icon.qicon() );
    messenger_ = new QSystemTrayIconMessenger( qsystemtrayicon_, this );
}


uiSystemTrayIcon::~uiSystemTrayIcon()
{
    delete menu_;
    delete messenger_;
    delete qsystemtrayicon_;
}


void uiSystemTrayIcon::setMenu( uiMenu* mnu )
{
    delete menu_;
    menu_ = mnu;

    qsystemtrayicon_->setContextMenu( mnu->getQMenu() );
}


void uiSystemTrayIcon::setIcon( const uiIcon& icon )
{ qsystemtrayicon_->setIcon( icon.qicon() ); }

void uiSystemTrayIcon::setToolTip( const uiString& tt )
{ qsystemtrayicon_->setToolTip( toQString(tt) ); }

void uiSystemTrayIcon::show()
{ qsystemtrayicon_->show(); }

void uiSystemTrayIcon::hide()
{ qsystemtrayicon_->hide(); }

bool uiSystemTrayIcon::isSystemTrayAvailable()
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		March 2022
________________________________________________________________________

-*/

#include "applicationdata.h"
#include "uitraydialog.h"
#include "uiicon.h"
#include "uimenu.h"
#include "uisystemtrayicon.h"
#include "odwindow.h"

#include "uimsg.h"

#define mBody static_cast<uiDialogBody*>(body_)

uiTrayDialog::uiTrayDialog( uiParent* p, const uiDialog::Setup& s )
    : uiDialog(p, s)
{
    if ( uiSystemTrayIcon::isSystemTrayAvailable() )
    {
	setInTray( true );
	trayicon_ = new uiSystemTrayIcon( uiIcon() );
	trayicon_->show();
	mAttachCB(trayicon_->clicked,uiTrayDialog::showCB);
    }
    else
	setInTray( false );

    mAttachCB(postFinalize(), uiTrayDialog::initUI);
}


uiTrayDialog::~uiTrayDialog()
{
    detachAllNotifiers();
}


void uiTrayDialog::initUI( CallBacker* )
{
    makeTrayMenu();
}


void uiTrayDialog::showCB( CallBacker* )
{
    mBody->display( true );
}


void uiTrayDialog::exitCB( CallBacker* )
{
    showCB( nullptr );
    if ( uiMSG().askGoOn(tr("Confirm Exit of %1").arg(caption())) )
    {
	setInTray( false );
	close();
    }
}

void uiTrayDialog::setTrayIcon( const char* icon_identifier)
{
    if ( trayicon_ )
	trayicon_->setIcon( uiIcon(icon_identifier) );
}


void uiTrayDialog::setTrayToolTip( const uiString& uistr )
{
    if ( trayicon_ )
	trayicon_->setToolTip( uistr );
}

void uiTrayDialog::makeTrayMenu()
{
	uiMenu* mnu = new uiMenu;
	mnu->insertAction( new uiAction(tr("Show Window"),
					mCB(this, uiTrayDialog, showCB)) );
	addApplicationTrayMenuItems( mnu );
	mnu->insertAction( new uiAction(
			tr("Quit %1").arg(ApplicationData::applicationName()),
					mCB(this, uiTrayDialog, exitCB)) );
	trayicon_->setMenu( mnu );
}


void uiTrayDialog::setInTray( bool yn )
{
	mBody->setInTray( yn );
}

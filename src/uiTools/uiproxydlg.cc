/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2012
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uiproxydlg.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uispinbox.h"
#include "odnetworkaccess.h"
#include "settings.h"


uiProxyDlg::uiProxyDlg( uiParent* p )
    : uiDialog(p,Setup("Connection Settings",mNoDlgTitle,"0.4.7"))
{
    useproxyfld_ = new uiGenInput( this, "Use proxy", BoolInpSpec(true) );
    useproxyfld_->valuechanged.notify( mCB(this,uiProxyDlg,useProxyCB) );

    hostfld_ = new uiGenInput( this, "HTTP Proxy", StringInpSpec() );
    hostfld_->attach( alignedBelow, useproxyfld_ );

    portfld_ = new uiLabeledSpinBox( this, "Port" );
    portfld_->attach( rightTo, hostfld_ );
    portfld_->box()->setInterval( 1, 65535 );

    authenticationfld_ =
	new uiCheckBox( this, "Use uthentication" );
    authenticationfld_->activated.notify( 
				    mCB(this,uiProxyDlg,useProxyCB) );
    authenticationfld_->attach( alignedBelow, hostfld_ );

    usernamefld_ = new uiGenInput( this, "User name", StringInpSpec() );
    usernamefld_->attach( alignedBelow, authenticationfld_ );
    pwdfld_ = new uiLineEdit( this, "Password" );
    pwdfld_->setToolTip( "Password is case sensitive" );
    pwdfld_->attach( alignedBelow, usernamefld_ );
    pwdfld_->setPrefWidthInChar( 23 );
    pwdfld_->setPasswordMode();
    pwdlabel_ = new uiLabel( this, "Password" );
    pwdlabel_->attach( leftOf, pwdfld_ );

    initFromSettings();
    useProxyCB(0);
}


uiProxyDlg::~uiProxyDlg()
{}


void uiProxyDlg::initFromSettings()
{
    Settings& setts = Settings::common();
    bool useproxy = false;
    setts.getYN( Network::sKeyUseProxy(), useproxy );
    useproxyfld_->setValue( useproxy );

    BufferString host;
    setts.get( Network::sKeyProxyHost(), host );
    hostfld_->setText( host );

    int port = 1;
    setts.get( Network::sKeyProxyPort(), port );
    portfld_->box()->setValue( port );

    bool needauth = false;
    setts.getYN( Network::sKeyUseAuthentication(), needauth );
    authenticationfld_->setChecked( needauth );

    BufferString username;
    setts.get( Network::sKeyProxyUserName(), username );
    usernamefld_->setText( username );

    BufferString password;
    setts.get( Network::sKeyProxyPassword(), password );
    pwdfld_->setText( password );
}


bool uiProxyDlg::saveInSettings()
{
    Settings& setts = Settings::common();
    const bool useproxy = useproxyfld_->getBoolValue();
    setts.setYN( Network::sKeyUseProxy(), useproxy );

    BufferString host = useproxy ? hostfld_->text() : "";
    setts.set( Network::sKeyProxyHost(), host );

    const int port = useproxy ? portfld_->box()->getValue() : 1;
    setts.set( Network::sKeyProxyPort(), port );

    const bool needauth = useproxy ? authenticationfld_->isChecked() : false;
    setts.setYN( Network::sKeyUseAuthentication(), needauth );
    if ( needauth )
    {
	BufferString username = useproxy ? usernamefld_->text() : "";
	setts.set( Network::sKeyProxyUserName(), username );
	BufferString password = useproxy ? pwdfld_->text() : "";
	setts.set( Network::sKeyProxyPassword(), password );
    }

    return setts.write();
}


void uiProxyDlg::useProxyCB( CallBacker* )
{
    const bool ison = useproxyfld_->getBoolValue();
    hostfld_->setSensitive( ison );
    portfld_->setSensitive( ison );
    authenticationfld_->setSensitive( ison );
    const bool needauth = authenticationfld_->isChecked();
    usernamefld_->setSensitive( ison && needauth );
    pwdfld_->setSensitive( ison && needauth );
    pwdlabel_->setSensitive( ison && needauth );
}


bool uiProxyDlg::acceptOK( CallBacker* )
{
    if ( !saveInSettings() )
    {
	uiMSG().error( "Cannot write to settings file" );
	return false;
    }

    return true;
}

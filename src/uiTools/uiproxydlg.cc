/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiproxydlg.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uispinbox.h"
#include "odnetworkaccess.h"
#include "settings.h"
#include "od_helpids.h"

uiProxyDlg::uiProxyDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Connection Settings"),mNoDlgTitle,
    mODHelpKey(mProxyDlgHelpID) ))
{
    useproxyfld_ = new uiGenInput( this, tr("Use proxy"), BoolInpSpec(true) );
    useproxyfld_->valuechanged.notify( mCB(this,uiProxyDlg,useProxyCB) );

    hostfld_ = new uiGenInput( this, tr("HTTP Proxy"), StringInpSpec() );
    hostfld_->attach( alignedBelow, useproxyfld_ );

    portfld_ = new uiLabeledSpinBox( this, tr("Port") );
    portfld_->attach( rightTo, hostfld_ );
    portfld_->box()->setInterval( 1, 65535 );

    authenticationfld_ =
	new uiCheckBox( this, tr("Use authentication") );
    authenticationfld_->activated.notify(
				    mCB(this,uiProxyDlg,useProxyCB) );
    authenticationfld_->attach( alignedBelow, hostfld_ );

    usernamefld_ = new uiGenInput( this, tr("User name"), StringInpSpec() );
    usernamefld_->attach( alignedBelow, authenticationfld_ );
    pwdfld_ = new uiLineEdit( this, "Password" );
    pwdfld_->setToolTip( tr("Password is case sensitive") );
    pwdfld_->attach( alignedBelow, usernamefld_ );
    pwdfld_->setPrefWidthInChar( 23 );
    pwdfld_->setPasswordMode();
    pwdlabel_ = new uiLabel( this, tr("Password") );
    pwdlabel_->attach( leftOf, pwdfld_ );
    savepwdfld_ = new uiCheckBox( this, tr("Save password (plain text)") );
    savepwdfld_->attach( alignedBelow, pwdfld_ );

    initFromSettings();
    useProxyCB(0);
}


uiProxyDlg::~uiProxyDlg()
{}


void uiProxyDlg::initFromSettings()
{
    Settings& setts = Settings::common();
    usePar( setts );
}


bool uiProxyDlg::usePar( const IOPar& pars )
{
    bool useproxy = false;
    pars.getYN( Network::sKeyUseProxy(), useproxy );
    useproxyfld_->setValue( useproxy );

    BufferString host;
    pars.get( Network::sKeyProxyHost(), host );
    hostfld_->setText( host );

    int port = 1;
    pars.get( Network::sKeyProxyPort(), port );
    portfld_->box()->setValue( port );

    bool needauth = false;
    pars.getYN( Network::sKeyUseAuthentication(), needauth );
    authenticationfld_->setChecked( needauth );

    BufferString username;
    pars.get( Network::sKeyProxyUserName(), username );
    usernamefld_->setText( username );

    bool iscrypt = false;
    BufferString password;
    pars.get( Network::sKeyProxyPassword(), password );
    if ( pars.getYN(Network::sKeyCryptProxyPassword(),iscrypt) )
    {
	uiString str;
	str.setFromHexEncoded( password );
	password = str.getFullString();
    }

    pwdfld_->setText( password );
    savepwdfld_->setChecked( !password.isEmpty() );
    return true;
}


bool uiProxyDlg::saveInSettings()
{
    Settings& setts = Settings::common();
    if ( !fillPar(setts,true) )
	return false;

    return setts.write();
}


bool uiProxyDlg::fillPar( IOPar& pars, bool store ) const
{
    const bool useproxy = useproxyfld_->getBoolValue();
    pars.setYN( Network::sKeyUseProxy(), useproxy );

    BufferString host = useproxy ? hostfld_->text() : "";
    pars.set( Network::sKeyProxyHost(), host );

    const int port = useproxy ? portfld_->box()->getIntValue() : 1;
    pars.set( Network::sKeyProxyPort(), port );

    const bool needauth = useproxy ? authenticationfld_->isChecked() : false;
    pars.setYN( Network::sKeyUseAuthentication(), needauth );
    if ( needauth )
    {
	const bool savepwd = !store || savepwdfld_->isChecked();
	BufferString username = useproxy ? usernamefld_->text() : "";
	pars.set( Network::sKeyProxyUserName(), username );
	BufferString password = useproxy && savepwd ? pwdfld_->text() : "";
	uiString str = toUiString( password );
	str.getHexEncoded( password );
	pars.set( Network::sKeyProxyPassword(), password );
	pars.setYN( Network::sKeyCryptProxyPassword(), savepwd );
    }

    return true;
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
    savepwdfld_->setSensitive( ison && needauth );
}


bool uiProxyDlg::acceptOK( CallBacker* )
{
    IOPar pars;
    if ( !fillPar(pars,false) )
	return false;

    Network::setHttpProxyFromIOPar( pars );
    saveInSettings();
    return true;
}


uiNetworkUserQuery::uiNetworkUserQuery()
    : mainwin_(0)
{
}

bool uiNetworkUserQuery::setFromUser()
{
    if ( !mainwin_ )
	return false;

    uiProxyDlg dlg( mainwin_ );
    return dlg.go();
}

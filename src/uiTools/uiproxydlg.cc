/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2012
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiproxydlg.cc,v 1.1 2012-05-25 19:13:10 cvsnanne Exp $";


#include "uiproxydlg.h"

#include "uigeninput.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "odhttp.h"
#include "settings.h"


uiProxyDlg::uiProxyDlg( uiParent* p )
    : uiDialog(p,Setup("Connection Settings",mNoDlgTitle,mTODOHelpID))
{
    useproxyfld_ = new uiGenInput( this, "Use proxy", BoolInpSpec(true) );
    useproxyfld_->valuechanged.notify( mCB(this,uiProxyDlg,useProxyCB) );

    hostfld_ = new uiGenInput( this, "HTTP Proxy", StringInpSpec() );
    hostfld_->attach( alignedBelow, useproxyfld_ );

    portfld_ = new uiLabeledSpinBox( this, "Port" );
    portfld_->attach( rightTo, hostfld_ );
    portfld_->box()->setInterval( 1, 65535 );

    initFromSettings();
    useProxyCB(0);
}


uiProxyDlg::~uiProxyDlg()
{}


void uiProxyDlg::initFromSettings()
{
    Settings& setts = Settings::common();
    bool useproxy = false;
    setts.getYN( ODHttp::sKeyUseProxy(), useproxy );
    useproxyfld_->setValue( useproxy );

    BufferString host;
    setts.get( ODHttp::sKeyProxyHost(), host );
    hostfld_->setText( host );

    int port = 1;
    setts.get( ODHttp::sKeyProxyPort(), port );
    portfld_->box()->setValue( port );
}


bool uiProxyDlg::saveInSettings()
{
    Settings& setts = Settings::common();
    const bool useproxy = useproxyfld_->getBoolValue();
    setts.setYN( ODHttp::sKeyUseProxy(), useproxy );

    BufferString host = useproxy ? hostfld_->text() : "";
    setts.set( ODHttp::sKeyProxyHost(), host );

    const int port = useproxy ? portfld_->box()->getValue() : 1;
    setts.set( ODHttp::sKeyProxyPort(), port );

    return setts.write();
}


void uiProxyDlg::useProxyCB( CallBacker* )
{
    hostfld_->setSensitive( useproxyfld_->getBoolValue() );
    portfld_->setSensitive( useproxyfld_->getBoolValue() );
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

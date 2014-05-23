/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uihandledlsitefail.h"

#include "uibutton.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uiproxydlg.h"
#include "uislider.h"
#include "oddlsite.h"
#include "genc.h"
#include "od_helpids.h"


static uiString gtWinTitle( const ODDLSite& dlsite )
{
    uiString errmsg = dlsite.errMsg();
    const bool iserr = !errmsg.isEmpty();
    uiString ret( iserr ? "Failure accessing %1" : "Stopped accessing %1" );
    ret.arg( dlsite.host() );
    return ret;
}


static uiString gtCaption( const ODDLSite& dlsite, bool isfatal )
{
    uiString errmsg = dlsite.errMsg();
    const bool iserr = !errmsg.isEmpty();

    const uiString part2 = uiString( isfatal
			      ? "This access is mandatory"
			      : "You can try again" );
    uiString ret;
    if ( iserr )
	ret = uiString( "Error: %1\n%2" ).arg( errmsg ).arg( part2 );
    else
	ret = part2;

    return ret;
}

uiHandleDLSiteFail::uiHandleDLSiteFail( uiParent* p, const ODDLSite& dlsite,
				    bool isfatal, const BufferStringSet* sites )
	: uiDialog(p,Setup(gtWinTitle(dlsite),gtCaption(dlsite,isfatal),
					mODHelpKey(mHandleDLSiteFailHelpID) ))
	, isfatal_(isfatal)
	, site_(dlsite.host())
	, dlsitefld_(0)
{
    setCancelText( isfatal ? "&Exit Program" : "&Give up" );
    setOkText( "&Try again" );

    BufferStringSet dlsites;
    dlsites.add( dlsite.host() );
    if ( sites )
	dlsites.add( *sites, false );

    uiLabeledComboBox* lcb = 0;
    if ( dlsites.size() >= 1 )
    {
	lcb = new uiLabeledComboBox( this, dlsites, "Download from" );
	dlsitefld_ = lcb->box();
	dlsitefld_->setText( site_ );
    }

    proxybut_ = new uiPushButton( this, "&Proxy settings", false );
    proxybut_->setPixmap( "proxysettings" );
    proxybut_->setPrefWidthInChar( 21 );
    proxybut_->activated.notify( mCB(this,uiHandleDLSiteFail,proxyButCB) );
    proxybut_->attach( rightOf, lcb );

    timeoutfld_ = new uiSlider( this, uiSlider::Setup(tr("Timeout (1-60 s)")),
				"Timeout" );
    timeoutfld_->setInterval( StepInterval<float>(1,60,1) );
    timeoutfld_->setTickMarks( uiSlider::Below );
    timeoutfld_->setValue( dlsite.timeout() );
    if ( lcb )
	timeoutfld_->attach( alignedBelow, lcb );
}


float uiHandleDLSiteFail::timeout() const
{
    return timeoutfld_->getValue();
}


void uiHandleDLSiteFail::proxyButCB( CallBacker* )
{
    uiProxyDlg dlg( this );
    dlg.setHelpKey( mNoHelpKey );
    dlg.go();
}


bool uiHandleDLSiteFail::rejectOK( CallBacker* )
{
    if ( isfatal_ )
	ExitProgram( 1 );
    return true;
}


bool uiHandleDLSiteFail::acceptOK( CallBacker* )
{
    if ( dlsitefld_ )
	site_ = dlsitefld_->text();
    return true;
}


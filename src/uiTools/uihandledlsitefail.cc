/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihandledlsitefail.cc,v 1.7 2012/08/07 11:46:34 cvsranojay Exp $";

#include "uihandledlsitefail.h"

#include "uibutton.h"
#include "uiproxydlg.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uislider.h"
#include "oddlsite.h"


static BufferString gtWinTitle( const ODDLSite& dlsite )
{
    const char* errmsg = dlsite.errMsg();
    const bool iserr = errmsg && *errmsg;
    BufferString ret( iserr ? "Failure accessing " : "Stopped accessing " );
    ret.add( dlsite.host() );
    return ret;
}


static BufferString gtCaption( const ODDLSite& dlsite, bool isfatal )
{
    const char* errmsg = dlsite.errMsg();
    const bool iserr = errmsg && *errmsg;
    BufferString ret;
    if ( iserr )
	ret.add( "Error: " ).add( errmsg ).add( "\n" );
    ret.add( isfatal ? "This access is mandatory" : "You can try again" );
    return ret;
}

uiHandleDLSiteFail::uiHandleDLSiteFail( uiParent* p, const ODDLSite& dlsite,
				    bool isfatal, const BufferStringSet* sites )
	: uiDialog(p,Setup(gtWinTitle(dlsite),gtCaption(dlsite,isfatal),
						"0.5.5"))
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

    uiPushButton* proxybut = new uiPushButton( this, "&Proxy settings", false );
    proxybut->setPixmap( "proxysettings" );
    proxybut->setPrefWidthInChar( 21 );
    proxybut->activated.notify( mCB(this,uiHandleDLSiteFail,proxyButCB) );
    proxybut->attach( rightOf, lcb );

    timeoutfld_ = new uiSlider( this, "Timeout" );
    timeoutfld_->setInterval( StepInterval<float>(1,60,1) );
    timeoutfld_->setTickMarks( uiSlider::Below );
    timeoutfld_->setValue( dlsite.timeout() );
    if ( lcb )
	timeoutfld_->attach( alignedBelow, lcb );
    new uiLabel( this, "Timeout (1-60 s)", timeoutfld_ );
}


float uiHandleDLSiteFail::timeout() const
{
    return timeoutfld_->getValue();
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


void uiHandleDLSiteFail::proxyButCB( CallBacker* )
{
    uiProxyDlg dlg( this );
    dlg.setHelpID( mNoHelpID );
    dlg.go();
}

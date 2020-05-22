/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          June 2012
 ________________________________________________________________________

-*/


#include "uiissuereporter.h"

#include "envvars.h"
#include "filepath.h"
#include "settings.h"
#include "uiclipboard.h"
#include "uilabel.h"
#include "uitextedit.h"
#include "uibutton.h"
#include "uidesktopservices.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiproxydlg.h"
#include "safefileio.h"

#include "fstream"

static FixedString sKeyAskBeforeSending()
{ return "Ask before sending issue-report"; }


uiIssueReporterDlg::uiIssueReporterDlg( uiParent* p,
					System::IssueReporter& rep )
    : uiDialog( p, uiDialog::Setup(tr("Problem reporter"),
		mNoDlgTitle,mNoHelpKey) )
    , reporter_(rep)
{
    uiGroup* lblgrp = new uiGroup( this, "Label frame group" );
    lblgrp->setFrame( true );
    uiLabel* plealbl = new uiLabel( lblgrp,
		tr("OpendTect has stopped working.\n\n"
		"An error report has been created,\n"
		"which we would like to use for analysis.\n\n"
		"You would be helping us immensely by giving us\n"
		"as much detail as you wish on what you were doing\n"
		"when this problem occurred.\n\n"
		"For feedback and/or updates on this issue,\n"
		"do please also leave your e-mail address\n") );
    plealbl->setAlignment( OD::Alignment::HCenter );

    const bool senddmp = reporter_.isBinary();
    uiLabel* filelbl = new uiLabel( this, tr("Crash report: ") );
    uiLabel* filenmlbl = new uiLabel( this, toUiString(reporter_.filePath()) );
    filenmlbl->attach( alignedBelow, lblgrp );
    filelbl->attach( leftOf, filenmlbl );

    uiButton* vrbut = new uiPushButton( this,
	    		senddmp ? tr("Open folder") : tr("View report"),
			mCB(this, uiIssueReporterDlg, viewReportCB), false);

    uiGroup* usrinpgrp = new uiGroup( this, "User input group" );
    commentfld_ = new uiTextEdit( usrinpgrp );
    commentfld_->setPrefWidthInChar( 60 );
    commentfld_->setPrefHeightInChar( 8 );
    new uiLabel( usrinpgrp, tr("[Details you wish to share]"), commentfld_ );
    emailfld_ = new uiGenInput( usrinpgrp, tr("[E-mail address]") );
    emailfld_->attach( alignedBelow, commentfld_ );
    emailfld_->setStretch( 2, 1 );

    uiButton* proxybut = new uiPushButton( usrinpgrp, tr("Proxy settings"),
					   false );
    proxybut->setIcon( "proxysettings" );
    proxybut->activated.notify( mCB(this,uiIssueReporterDlg,proxySetCB) );
    proxybut->attach( rightOf, emailfld_ );

    usrinpgrp->setHAlignObj( emailfld_ );
    usrinpgrp->attach( alignedBelow, filenmlbl );
    vrbut->attach( ensureBelow, lblgrp );
    vrbut->attach( rightAlignedAbove, usrinpgrp );

    setCancelText( sDontSendReport() );
    setOkText( sSendReport() );

    uiNetworkUserQuery* uinetquery = new uiNetworkUserQuery;
    uinetquery->setMainWin( this );
    NetworkUserQuery::setNetworkUserQuery( uinetquery );
}


void uiIssueReporterDlg::viewReportCB( CallBacker* )
{
    viewReport( uiString::empty() );
}


bool uiIssueReporterDlg::allowSending() const
{
    const BufferString env = GetEnvVar("PROHIBIT_SENDING_ISSUE_REPORT");
    if ( !env.isEmpty() )
	return false;

    bool res = true;
    Settings::common().getYN(sKeyAllowSending(),res );

    return res;
}


void uiIssueReporterDlg::viewReport( const uiString& cap )
{
    if ( reporter_.isBinary() )
    {
	uiDesktopServices::openUrl( FilePath(reporter_.filePath()).pathOnly() );
	return;
    }

    BufferString report;
    getReport( report );

    uiDialog dlg( this, uiDialog::Setup(tr("View report"),
				    mNoDlgTitle, mNoHelpKey ) );
    uiLabel* label = cap.isEmpty()
	? (uiLabel*) 0
	: new uiLabel( &dlg, cap );

    uiTextBrowser* browser = new uiTextBrowser(&dlg);
    if ( label )
	browser->attach( alignedBelow, label );

    browser->setText( report.buf() );
    dlg.setCancelText( uiString::empty() );

    uiPushButton* copytoclipboard = new uiPushButton( &dlg,
				  tr("Copy to Clipboard"), true );
    copytoclipboard->activated.notify(
			mCB(this,uiIssueReporterDlg,copyToClipBoardCB) );
    copytoclipboard->attach( alignedBelow, browser );

    dlg.go();
}


void uiIssueReporterDlg::copyToClipBoardCB( CallBacker* )
{
    BufferString report;
    getReport( report );

    uiClipboard::setText( toUiString(report) );
}


void uiIssueReporterDlg::proxySetCB( CallBacker* )
{
    uiProxyDlg dlg( this );
    dlg.setHelpKey( mNoHelpKey );
    dlg.go();
}


void uiIssueReporterDlg::setButSensitive( bool yn )
{
    uiButton* okbut = button( OK );
    uiButton* cancelbut = button( CANCEL );
    if ( okbut )
	okbut->setSensitive( yn );

    if ( cancelbut )
	cancelbut->setSensitive( yn );
}



void uiIssueReporterDlg::getReport( BufferString& res ) const
{
    res.set( "From: " ).add( emailfld_->text() );
    res.add( "\n\nDetails:\n\n" ).add( commentfld_->text() );
    res.add( "\n\nReport:\n\n" ).add( reporter_.getReport() );
}


bool uiIssueReporterDlg::acceptOK()
{
    if ( !allowSending() )
    {
	viewReport(
	    tr("Your installation does not allow direct "
	       "reporting of problem reports. Please "
	       "copy this text and send it to "
	       "support@dgbes.com for processing." ));

	return true;
    }

    bool dontaskagain = false;
    Settings::common().getYN( sKeyAskBeforeSending(), dontaskagain );

    bool res = dontaskagain
	? true
	: uiMSG().askGoOn(tr("The report will be sent to dGB Earth Sciences "
		"using unencrypted connections, and possibly "
		"using third party servers."), sSendReport(),
		sDontSendReport(), &dontaskagain );

    if ( !res )
	return false;

    if ( dontaskagain )
    {
	Settings::common().setYN( sKeyAskBeforeSending(), true );
	Settings::common().write();
    }

    uiUserShowWait usw( this, tr("Sending report") );
    setButSensitive( false );

    res = false;
    BufferString report; getReport( report );
    reporter_.getReport() = report;

    if ( reporter_.send() )
    {
	usw.readyNow();
	if ( !reporter_.getMessage().isEmpty() )
	    uiMSG().message( reporter_.getMessage() );
	else
	    uiMSG().message( tr("The report was successfully sent."
		"\n\nThank you for your contribution to OpendTect!") );
    }
    else
    {
	usw.setMessage( uiStrings::sSavingData() );
	filename_ = reporter_.filePath();
	SafeFileIO outfile( filename_, false );
	if ( outfile.open( false ) )
	{
	    od_ostream& outstream = outfile.ostrm();
	    outstream << report;
	    if ( outstream.isOK() )
		outfile.closeSuccess();
	    else
		outfile.closeFail();
	}

	usw.readyNow();
	uiString msg = tr("The report could not be sent automatically");
	msg.appendPhrase(tr("You can still send it manually by e-mail"));
	msg.appendPhrase(tr("Please send the file : %1 to").arg(filename_));
	msg.appendPlainText("\nsupport@opendtect.org");
	msg.appendPhrase( tr("Would you like to retry sending the report "
			    "using different proxy settings?"),
			    uiString::NoSep, uiString::AfterEmptyLine );
	res = uiMSG().askGoOn( msg );
	if ( res )
	    proxySetCB( 0 );
    }

    setButSensitive( true );
    return !res;
}

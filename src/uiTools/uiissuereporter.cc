/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiissuereporter.h"

#include "envvars.h"
#include "filepath.h"
#include "mousecursor.h"
#include "safefileio.h"
#include "settings.h"

#include "uibutton.h"
#include "uiclipboard.h"
#include "uidesktopservices.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uiproxydlg.h"
#include "uiseparator.h"
#include "uitextedit.h"

static StringView sKeyAskBeforeSending()
{ return "Ask before sending issue-report"; }


#define mExtraSpacing 10

uiIssueReporterDlg::uiIssueReporterDlg( uiParent* p,
					System::IssueReporter& rep )
    : uiDialog( p, uiDialog::Setup(tr("Problem reporter"),
		mNoDlgTitle,mNoHelpKey) )
    , reporter_(rep)
{
    auto* infolbl = new uiLabel( this,
				 tr("OpendTect has stopped working\n\n"
				    "An error report has been created,\n"
				    "which we would like to use for analysis"));
    infolbl->setAlignment( Alignment::HCenter );

    auto* filenamefld = new uiLineEdit( this, "filename" );
    filenamefld->setText( FilePath(reporter_.filePath()).fileName() );
    filenamefld->setReadOnly( true );
    filenamefld->setStretch( 2, 1 );
    mUnusedVar auto* filelbl =
	new uiLabel( this, tr("Error report "), filenamefld );

    auto* showbut = new uiPushButton( this, tr("Show in folder"),
			mCB(this,uiIssueReporterDlg,viewReportCB), false );
    showbut->setIcon( "folder" );
    showbut->attach( rightOf, filenamefld );
    infolbl->attach( centeredAbove, filenamefld, mExtraSpacing );

    machinfofld_ = new uiTextBrowser( this );
    machinfofld_->setPrefWidthInChar( 60 );
    machinfofld_->setPrefHeightInChar( 8 );
    machinfofld_->setText( reporter_.getReport() );

    filenamefld->attach( alignedAbove, machinfofld_ );

    machinfobut_ = new uiCheckBox( this, tr("Add System information"),
				   mCB(this,uiIssueReporterDlg,machInfoCB) );
    machinfobut_->setChecked( true );
    machinfobut_->attach( centeredLeftOf, machinfofld_ );

    commentfld_ = new uiTextEdit( this );
    commentfld_->setPrefWidthInChar( 60 );
    commentfld_->setPrefHeightInChar( 8 );
    commentfld_->attach( alignedBelow, machinfofld_, mExtraSpacing );
    commentfld_->setPlaceholderText( tr("Please add as much detail as you wish"
			"\nabout what you were doing when"
			"\nthis problem occurred."
			"\n\nThis would help us immensely"
			"\nin resolving the problem") );

    auto* commentlbl = new uiLabel( this, tr("Description") );
    commentlbl->attach( centeredLeftOf, commentfld_ );

    emailfld_ = new uiLineEdit( this, "email" );
    emailfld_->setPlaceholderText( toUiString("someone@example.com") );
    emailfld_->setToolTip( tr("Please provide an email address\n"
			   "for feedback or update on this issue") );
    emailfld_->attach( alignedBelow, commentfld_ );
    emailfld_->setStretch( 2, 1 );

    auto* emaillbl = new uiLabel( this, tr("Email"), emailfld_ );
    emaillbl->setAlignment( Alignment::Right );

    uiButton* proxybut = new uiPushButton( this, tr("Proxy settings"),
					   false );
    proxybut->setIcon( "proxysettings" );
    proxybut->activated.notify( mCB(this,uiIssueReporterDlg,proxySetCB) );
    proxybut->attach( rightOf, emailfld_ );

    setCancelText( sDontSendReport() );
    setOkText( sSendReport() );

    uiNetworkUserQuery* uinetquery = new uiNetworkUserQuery;
    uinetquery->setMainWin( this );
    NetworkUserQuery::setNetworkUserQuery( uinetquery );
}


uiIssueReporterDlg::~uiIssueReporterDlg()
{}


void uiIssueReporterDlg::viewReportCB( CallBacker* )
{
    uiDesktopServices::showInFolder( reporter_.filePath() );
}


void uiIssueReporterDlg::machInfoCB( CallBacker* )
{
    machinfofld_->setSensitive( machinfobut_->isChecked() );
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
    res.add( "\n\nDetails given by user:\n" ).add( commentfld_->text() );
    res.add( "\n\nSystem Information:\n" ).add( reporter_.getReport() );
}


bool uiIssueReporterDlg::acceptOK(CallBacker *)
{
    filename_ = reporter_.filePath();
    if ( !allowSending() )
    {
	uiMSG().message(
	    tr("Your installation does not allow direct "
	       "reporting of error reports.\n"
	       "Please send the error report file:\n\n%1"
	       "\n\nalong with the system information"
	       "\n\nto support@dgbes.com.").arg(filename_) );

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

    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    setButSensitive( false );

    res = false;
    BufferString report; getReport( report );
    reporter_.getReport() = report;

    if ( reporter_.send() )
    {
	if ( !reporter_.getMessage().isEmpty() )
	{
	    uiMSG().message( reporter_.getMessage() );
	}
	else
	{
	    uiMSG().message( tr("The report was successfully sent."
		"\n\nThank you for your contribution to OpendTect!") );
	}
    }
    else
    {
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

	cursorchanger.restore();
	uiString msg = tr("The report could not be sent automatically.\n"
			  "You can still send it manually by e-mail.\n"
			  "Please send the file:\n\n%1"
			  "\n\nalong with the system information"
			  "\n\nto support@dgbes.com.")
		     .arg(filename_);

	msg.append( tr("\n\nWould you like to retry sending the report using "
		    "different proxy settings?") );
	res = uiMSG().askGoOn( msg );
	if ( res )
	    proxySetCB( 0 );
    }

    setButSensitive( true );
    return !res;
}

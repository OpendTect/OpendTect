/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          June 2012
 ________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiissuereporter.h"

#include "uilabel.h"
#include "uitextedit.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "safefileio.h"

#include "fstream"


uiIssueReporterDlg::uiIssueReporterDlg( uiParent* p )
    : uiDialog( p, uiDialog::Setup("Problem reporter",mNoDlgTitle,mNoHelpID) )
{
    uiGroup* lblgrp = new uiGroup( this, "Label frame group" );
    lblgrp->setFrame( true );
    uiLabel* plealbl = new uiLabel( lblgrp,
		"OpendTect has stopped working.\n\n"
		"An error report has been created,\n"
		"which we would like to use for analysis.\n\n"
		"You would be helping us immensely by giving us\n"
		"as much detail as you wish on what you were doing\n"
		"when this problem occurred.\n\n"
		"For feedback and/or updates on this issue,\n"
		"do please also leave your e-mail address\n" );
    plealbl->setAlignment( Alignment::HCenter );
    uiButton* vrbut = new uiPushButton( this, "View report",
			mCB(this, uiIssueReporterDlg, viewReportCB), false);
    vrbut->attach( centeredRightOf, lblgrp );

    uiGroup* usrinpgrp = new uiGroup( this, "User input group" );
    commentfld_ = new uiTextEdit( usrinpgrp );
    commentfld_->setPrefWidthInChar( 60 );
    commentfld_->setPrefHeightInChar( 8 );
    new uiLabel( usrinpgrp, "[Details you wish to share]", commentfld_ );
    emailfld_ = new uiGenInput( usrinpgrp, "[E-mail address]" );
    emailfld_->attach( alignedBelow, commentfld_ );
    emailfld_->setStretch( 2, 1 );

    usrinpgrp->setHAlignObj( emailfld_ );
    usrinpgrp->attach( alignedBelow, lblgrp );

    setCancelText( "Do &Not Send" );
    setOkText( "&Send Report" );
}
	       

void uiIssueReporterDlg::viewReportCB( CallBacker* )
{
    BufferString report;
    getReport( report );
    
    uiDialog dlg( this, uiDialog::Setup("View report", 0, mNoHelpID ) );
    uiTextBrowser* browser = new uiTextBrowser(&dlg);
    browser->setText( report.buf() );
    dlg.setCancelText( 0 );
    dlg.go();
}
	       


void uiIssueReporterDlg::getReport( BufferString& res ) const
{
    res.set( "From: " ).add( emailfld_->text() );
    res.add( "\n\nDetails:\n\n" ).add( commentfld_->text() );
    res.add( "\n\nReport:\n\n" ).add( reporter_.getReport() );
}


bool uiIssueReporterDlg::acceptOK(CallBacker *)
{
    BufferString report; getReport( report );
    reporter_.getReport() = report;

    if ( reporter_.send() )
	uiMSG().message( "The report was successfully sent."
		"\n\nThank you for your contribution to OpendTect!" );
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
		
	const BufferString msg(
		"The report could not be sent automatically.\n"
		"You can still send it manually by e-mail.\n"
		"Please send the file:\n\n", filename_ ,
		"\n\nto support@opendtect.org." );
	uiMSG().error( msg );
    }

    return true;
}

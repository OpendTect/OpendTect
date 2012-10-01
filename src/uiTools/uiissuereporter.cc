/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          June 2012
 ________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uiissuereporter.cc,v 1.2 2012/06/26 13:09:33 cvskris Exp $";


#include "uiissuereporter.h"

#include "uilabel.h"
#include "uitextedit.h"
#include "uibutton.h"
#include "uimsg.h"
#include "safefileio.h"

#include "fstream"


uiIssueReporterDlg::uiIssueReporterDlg( uiParent* p )
    : uiDialog( p, uiDialog::Setup( "Error reporter", 0, mNoHelpID ) )
{
    uiLabel* label = new uiLabel( this,
     "OpendTect has stopped working, and an error report has been created.\n"
     "Do you want to send it to dGB Earth Sciences for analysis?\n\n"
     "It is also appreciated if you give some details on what you were\n"
     "working on when this happened.\n"
     "You can also give your e-mail if we are allowed to contact you for\n"
     "further questions" );
    
    commentfld_ = new uiTextEdit( this );
    commentfld_->attach( alignedBelow, label );
    
    uiLabel* commentlabel = new uiLabel( this, "Details (voluntary)",
					commentfld_);
    
    viewreportbut_ = new uiPushButton( this, "View report",
			mCB(this, uiIssueReporterDlg, viewReportCB), false);
    viewreportbut_->attach( alignedBelow, commentfld_ );
    
    setCancelText( "Do not send" );
    setOkText( "Send" );
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
    res.setEmpty();
    
    res.add( "Details:\n" ).add( commentfld_->text() );
    res.add( "\nReport:\n" ).add( reporter_.getReport() );
}


bool uiIssueReporterDlg::acceptOK(CallBacker *)
{
    BufferString report;
    getReport( report );
    
    reporter_.getReport() = report;
    
    if ( !reporter_.send() )
    {
	SafeFileIO outfile( filename_, false );
	if ( outfile.open( false ) )
	{
	    bool success = true;
	    std::ostream& outstream = outfile.ostrm();
	    if ( outstream )
	    {
		outstream << report.buf();
		success = outstream;
	    }
	    else
		success = false;
	    
	    if ( success )
		outfile.closeSuccess();
	    else
		outfile.closeFail();
	}
		
	BufferString msg =
	    "The report could not be sent automatically.\n"
	    "You can still send it manually by sending an e-mail \n"
	    "with the file ";
	msg.add( filename_ ).add( " to support@opendtect.org." );
	uiMSG().error( msg );
	return true;
    }
    
    uiMSG().message( "The report was successfully sent.\nThe development crew"
		    " want to thank you for your help!" );		    
    
    return true;
}


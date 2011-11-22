/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihandledlsitefail.cc,v 1.1 2011-11-22 12:58:17 cvsbert Exp $";

#include "uihandledlsitefail.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uislider.h"
#include "oddlsite.h"


uiHandleDLSiteFail::uiHandleDLSiteFail( uiParent* p, const ODDLSite& dlsite,
				    bool isfatal, const BufferStringSet* sites )
	: uiDialog(p,Setup(BufferString("Failure accessing ",dlsite.host()),
		BufferString("Error: ",dlsite.errMsg()),mTODOHelpID))
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
    if ( dlsites.size() > 1 )
    {
	lcb = new uiLabeledComboBox( this, dlsites, "Download from" );
	dlsitefld_ = lcb->box();
	dlsitefld_->setText( site_ );
    }

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

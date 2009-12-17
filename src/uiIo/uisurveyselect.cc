/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Dec 2009
________________________________________________________________________

-*/

#include "uisurveyselect.h"

#include "uibutton.h"
#include "oddirs.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "uisurvey.h"
#include "uisetdatadir.h"


uiSurveySelectDlg::uiSurveySelectDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Survey Slection",
				 "Select Survey",mNoHelpID))
    , basedirnm_(GetBaseDataDir())
{

    datarootfld_ = new uiFileInput( this, "Data Root",
			    uiFileInput::Setup(uiFileDialog::Gen,basedirnm_)
			    .directories(true) );
    datarootfld_->valuechanged.notify( 
		  mCB(this,uiSurveySelectDlg,surveySelectCB) );

    surveylistfld_ = new uiListBox( this,"Survey list", false, 
				    uiLabeledListBox::AboveLeft );
    surveylistfld_->attach( alignedBelow, datarootfld_ );
    fillSurveyList();
}


uiSurveySelectDlg::~uiSurveySelectDlg()
{
}


const char* uiSurveySelectDlg::getDataRoot() const
{
    return datarootfld_->text();
}

const BufferString uiSurveySelectDlg::getSurveyName() const
{
    return surveylistfld_->getText();
}


void uiSurveySelectDlg::fillSurveyList()
{
    BufferStringSet surveylist;
    uiSurvey::getSurveyList( surveylist, getDataRoot() );
    surveylistfld_->empty();
    surveylistfld_->addItems( surveylist );
}


void uiSurveySelectDlg::surveySelectCB( CallBacker* )
{
    fillSurveyList();
}


uiSurveySelect::uiSurveySelect( uiParent* p )
	: uiIOSelect(p,uiIOSelect::Setup("Select Survey"),
		     mCB(this,uiSurveySelect,selectCB))
{ 
}


uiSurveySelect::~uiSurveySelect()
{
}


void uiSurveySelect::selectCB( CallBacker* )
{
    uiSurveySelectDlg dlg( this );
    if( !dlg.go() ) return;
    
    setInputText( dlg.getSurveyName() );
}


void uiSurveySelect::enableButton( bool ison )
{
    selbut_->setSensitive( ison );
}

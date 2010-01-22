/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Dec 2009
________________________________________________________________________

-*/

#include "uisurveyselect.h"

#include "oddirs.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uisurvey.h"


uiSurveySelectDlg::uiSurveySelectDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Survey Slection",
				 "Select Survey",mNoHelpID))
    
{
    datarootfld_ = new uiFileInput( this, "Data Root",
		uiFileInput::Setup(uiFileDialog::Gen,GetBaseDataDir())
		.directories(true) );
    datarootfld_->valuechanged.notify( 
		mCB(this,uiSurveySelectDlg,rootSelectCB) );

    surveylistfld_ = new uiListBox( this, "Survey list", false, 
				    uiLabeledListBox::AboveLeft );
    surveylistfld_->attach( alignedBelow, datarootfld_ );
    surveylistfld_->selectionChanged.notify( 
		mCB(this,uiSurveySelectDlg,surveyListCB) );
    newsurveyfld_ = new uiGenInput( this, "Name" );
    
    newsurveyfld_->attach( alignedBelow, surveylistfld_ );
    fillSurveyList();
}


uiSurveySelectDlg::~uiSurveySelectDlg()
{}


const char* uiSurveySelectDlg::getDataRoot() const
{ return datarootfld_->text(); }


const BufferString uiSurveySelectDlg::getSurveyName() const
{ return newsurveyfld_->text(); }


void uiSurveySelectDlg::fillSurveyList()
{
    BufferStringSet surveylist;
    uiSurvey::getSurveyList( surveylist, getDataRoot() );
    surveylistfld_->empty();
    surveylistfld_->addItems( surveylist );
}


void uiSurveySelectDlg::rootSelectCB( CallBacker* )
{
    fillSurveyList();
}


void uiSurveySelectDlg::surveyListCB( CallBacker* )
{
    newsurveyfld_->setText( surveylistfld_->getText() );
}


bool uiSurveySelectDlg::isNewSurvey()
{
   return !surveylistfld_->isPresent( newsurveyfld_->text() );
}


uiSurveySelect::uiSurveySelect( uiParent* p )
	: uiIOSelect(p,uiIOSelect::Setup("Select Survey"),
		     mCB(this,uiSurveySelect,selectCB))
{
}


uiSurveySelect::~uiSurveySelect()
{}


void uiSurveySelect::selectCB( CallBacker* )
{
    uiSurveySelectDlg dlg( this );
    if( !dlg.go() ) return;
    
    setInputText( dlg.getSurveyName() );
    isnewsurvey_ = dlg.isNewSurvey();
}


bool uiSurveySelect::isNewSurvey()
{
    return isnewsurvey_;
}



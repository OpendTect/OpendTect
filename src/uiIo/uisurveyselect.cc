/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Dec 2009
________________________________________________________________________

-*/

#include "uisurveyselect.h"

#include "filepath.h"
#include "oddirs.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uisurvey.h"

extern "C" const char* GetSurveyName();

uiSurveySelectDlg::uiSurveySelectDlg( uiParent* p, const char* survnm,
       				      const char* dataroot )
    : uiDialog(p,uiDialog::Setup("Survey Selection",
				 "Select Survey",mTODOHelpID))
    
{
    datarootfld_ = new uiFileInput( this, "Data Root",
		uiFileInput::Setup(uiFileDialog::Gen,dataroot)
		.directories(true) );
    setDataRoot( dataroot );
    datarootfld_->valuechanged.notify( 
		mCB(this,uiSurveySelectDlg,rootSelCB) );

    surveylistfld_ = new uiListBox( this, "Survey list", false, 10 );
    surveylistfld_->attach( alignedBelow, datarootfld_ );
    surveylistfld_->selectionChanged.notify( 
		mCB(this,uiSurveySelectDlg,surveySelCB) );

    surveyfld_ = new uiGenInput( this, "Name" );
    surveyfld_->attach( alignedBelow, surveylistfld_ );
    fillSurveyList();
    setSurveyName( survnm );
}


uiSurveySelectDlg::~uiSurveySelectDlg()
{}


void uiSurveySelectDlg::setDataRoot( const char* dataroot )
{
    BufferString basedatadir( dataroot );
    if ( basedatadir.isEmpty() )
	basedatadir = GetBaseDataDir();
    datarootfld_->setText( dataroot );
}


const char* uiSurveySelectDlg::getDataRoot() const
{ return datarootfld_->text(); }

void uiSurveySelectDlg::setSurveyName( const char* nm )
{ surveylistfld_->setCurrentItem( nm ); }

const char* uiSurveySelectDlg::getSurveyName() const
{ return surveyfld_->text(); }

BufferString uiSurveySelectDlg::getSurveyPath() const
{
    FilePath fp( getDataRoot() );
    fp.add( getSurveyName() );
    return fp.fullPath();
}


void uiSurveySelectDlg::fillSurveyList()
{
    BufferStringSet surveylist;
    uiSurvey::getSurveyList( surveylist, getDataRoot() );
    surveylistfld_->empty();
    surveylistfld_->addItems( surveylist );
}


void uiSurveySelectDlg::rootSelCB( CallBacker* )
{
    fillSurveyList();
}


void uiSurveySelectDlg::surveySelCB( CallBacker* )
{
    surveyfld_->setText( surveylistfld_->getText() );
}


bool uiSurveySelectDlg::isNewSurvey() const
{
   return !surveylistfld_->isPresent( surveyfld_->text() );
}


// uiSurveySelect
uiSurveySelect::uiSurveySelect( uiParent* p )
    : uiIOSelect(p,uiIOSelect::Setup("Select Survey"),
		 mCB(this,uiSurveySelect,selectCB))
    , dataroot_(GetBaseDataDir())
    , surveyname_(GetSurveyName())
{}


uiSurveySelect::~uiSurveySelect()
{}


void uiSurveySelect::selectCB( CallBacker* )
{
    uiSurveySelectDlg dlg( this, surveyname_, dataroot_ );
    if( !dlg.go() ) return;

    isnewsurvey_ = dlg.isNewSurvey();
    surveyname_ = dlg.getSurveyName();
    dataroot_ = dlg.getDataRoot();
    setInputText( surveyname_ );
}

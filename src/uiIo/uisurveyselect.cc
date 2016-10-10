/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Dec 2009
________________________________________________________________________

-*/


#include "uisurveyselect.h"

#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "survinfo.h"
#include "uistrings.h"

#define mErrRet(s) { uiMSG().error(s); return; }

static bool checkIfDataDir( const char* path )
{
    FilePath fpo( path, ".omf" ), fps( path, SurveyInfo::sKeySetupFileName() );
    return File::exists( fpo.fullPath() ) && !File::exists( fps.fullPath() );
}


uiSurveySelectDlg::uiSurveySelectDlg( uiParent* p,
				      const char* survnm, const char* dr,
				      bool forread, bool needvalidrootdir )
    : uiDialog(p,uiDialog::Setup(tr("Select Data Root and Survey"),
		mNoDlgTitle, mODHelpKey(mSurveySelectDlgHelpID)))
    , forread_(forread)
    , needvalidrootdir_(needvalidrootdir)
    , surveyfld_(0)
    , dataroot_(dr && *dr ? dr : GetBaseDataDir())

{
    datarootfld_ = new uiFileInput( this, tr("%1 Root").arg(uiStrings::sData()),
		uiFileInput::Setup(uiFileDialog::Gen,dataroot_)
		.directories(true) );
    datarootfld_->setFileName( dataroot_ );
    datarootfld_->valuechanged.notify(
		mCB(this,uiSurveySelectDlg,rootSelCB) );

    surveylistfld_ = new uiListBox( this, "Survey list", OD::ChooseOnlyOne );
    surveylistfld_->setNrLines( 10 );
    surveylistfld_->attach( alignedBelow, datarootfld_ );
    surveylistfld_->selectionChanged.notify(
		mCB(this,uiSurveySelectDlg,surveySelCB) );

    if ( !forread_ )
    {
	surveyfld_ = new uiGenInput( this, uiStrings::sName() );
	surveyfld_->attach( alignedBelow, surveylistfld_ );
    }

    fillSurveyList( true );
    setSurveyName( survnm );
}


uiSurveySelectDlg::~uiSurveySelectDlg()
{}


const char* uiSurveySelectDlg::getDataRoot() const
{
    return datarootfld_->fileName();
}


void uiSurveySelectDlg::setSurveyName( const char* nm )
{
    surveylistfld_->setCurrentItem( nm );
}


const char* uiSurveySelectDlg::getSurveyName() const
{
    return surveyfld_ ? surveyfld_->text() : surveylistfld_->getText();
}

const BufferString uiSurveySelectDlg::getSurveyPath() const
{
    return FilePath(getDataRoot(),getSurveyName()).fullPath();
}


bool uiSurveySelectDlg::continueAfterErrMsg()
{
    if ( needvalidrootdir_ )
    {
	uiMSG().error( tr("Selected directory is not a valid Data Root") );
	return false;
    }

    const bool res = uiMSG().askGoOn(
	    tr("Selected directory is not a valid Data Root. Do you still "
	       "want to search for OpendTect Surveys in this location?") );
    return res;

}


void uiSurveySelectDlg::fillSurveyList( bool initial )
{
    surveylistfld_->setEmpty();
    if ( !initial )
	dataroot_ = getDataRoot();
    if ( !checkIfDataDir(dataroot_) && !continueAfterErrMsg()  )
	return;

    BufferStringSet surveylist;
    uiSurvey::getDirectoryNames( surveylist, false, dataroot_ );
    surveylistfld_->addItems( surveylist );
}


void uiSurveySelectDlg::rootSelCB( CallBacker* )
{
    fillSurveyList( false );
}


void uiSurveySelectDlg::surveySelCB( CallBacker* )
{
    if ( surveyfld_ )
	surveyfld_->setText( surveylistfld_->getText() );
}


bool uiSurveySelectDlg::isNewSurvey() const
{
   return surveyfld_ && !surveylistfld_->isPresent( surveyfld_->text() );
}

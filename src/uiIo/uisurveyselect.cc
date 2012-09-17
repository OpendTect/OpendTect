/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Dec 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisurveyselect.cc,v 1.15 2012/01/10 22:33:14 cvsnanne Exp $";


#include "uisurveyselect.h"

#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uisurvey.h"

extern "C" const char* GetSurveyName();

#define mErrRet(s) { uiMSG().error(s); return; }

static bool checkIfDataDir( const char* path )
{
    FilePath fpo( path, ".omf" ), fps( path, ".survey" );
    return File::exists( fpo.fullPath() ) && !File::exists( fps.fullPath() );
}


uiSurveySelectDlg::uiSurveySelectDlg( uiParent* p, const char* survnm,
       				      const char* dataroot )
    : uiDialog(p,uiDialog::Setup("Survey Selection",
				 "Select Survey",mNoHelpID))
    
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

const BufferString uiSurveySelectDlg::getSurveyPath() const
{
    return FilePath(getDataRoot(),getSurveyName()).fullPath();
}


void uiSurveySelectDlg::fillSurveyList()
{
    if( !checkIfDataDir(getDataRoot()) )
    {
	uiMSG().error( "Selected directory is not a valid Data Root" );
	return;
    }
    
    BufferStringSet surveylist;
    uiSurvey::getSurveyList( surveylist, getDataRoot() );  
    surveylistfld_->setEmpty();
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
uiSurveySelect::uiSurveySelect( uiParent* p, const char* lbl )
    : uiIOSelect(p,uiIOSelect::Setup( lbl && *lbl ? lbl : "Survey Select" ),
		 mCB(this,uiSurveySelect,selectCB))
    , dataroot_(GetBaseDataDir())
    , surveyname_(0)
{}


uiSurveySelect::~uiSurveySelect()
{}


void uiSurveySelect::selectCB( CallBacker* )
{
    uiSurveySelectDlg dlg( this, GetSurveyName(), dataroot_ );
    if( !dlg.go() ) return;

    isnewsurvey_ = dlg.isNewSurvey();
    surveyname_ = dlg.getSurveyName();
    dataroot_ = dlg.getDataRoot();
    setInputText( surveyname_ );
    selok_ = true;
}


static BufferString makeFullSurveyPath( const char* survnm,
					const char* dataroot )
{
    BufferString surveyname( survnm );
    cleanupString( surveyname.buf(), false, false, true );
    return FilePath(dataroot,surveyname).fullPath();
}


bool uiSurveySelect::getFullSurveyPath( BufferString& fullpath ) const
{
    BufferString input = getInput();
    if ( input.isEmpty() )
	return false;

    FilePath fp( input );
    if ( fp.fileName() == input )
    {
	fullpath = makeFullSurveyPath( input, dataroot_ );
	return fullpath.isEmpty() ? false : true;
    }
   
    BufferString path( fp.pathOnly() ), survnm( fp.fileName() );
    const bool isdatadir = checkIfDataDir( path );
    fullpath = makeFullSurveyPath( survnm, path );
    return isdatadir && !fullpath.isEmpty() ? true : false;
}


void uiSurveySelect::setSurveyPath( const char* fullpath )
{
    if ( !File::exists(fullpath) )
	mErrRet( "Selected directory does not exist.\n"
		 "Please specify the full path" );
    if ( !File::isDirectory(fullpath) )
	mErrRet( "Please select a valid directory" );
    if ( !File::exists( FilePath(fullpath,".survey").fullPath() ) )
	mErrRet( "This is not an OpendTect survey directory" );

    setInputText( fullpath );
}

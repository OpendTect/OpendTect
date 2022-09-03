/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisurveyfile.h"

#include "dbman.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "surveyfile.h"
#include "survinfo.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "uistringset.h"
#include "uisurvey.h"
#include "uisurvinfoed.h"
#include "uitaskrunner.h"
#include "ziputils.h"

extern "C" { mGlobal(Basic) void SetCurBaseDataDirOverrule(const char*); }

uiSurveyFile::uiSurveyFile( uiParent* p )
    : parent_(p)
{}


uiSurveyFile::~uiSurveyFile()
{
    delete survfile_;
}


bool uiSurveyFile::newFile()
{
    uiFileDialog fdlg( parent_, false, nullptr, SurveyFile::filtStr(),
						      tr("New Project File") );
    fdlg.setMode(uiFileDialog::AnyFile);
    fdlg.setDirectory( GetPersonalDir() );
    fdlg.setDefaultExtension( SurveyFile::extStr() );
    fdlg.setConfirmOverwrite(true);
    if (!fdlg.go())
	return false;

    FilePath openfile_fp = FilePath( fdlg.fileName() );
    openfile_fp.setExtension( SurveyFile::extStr() );
    BufferString surveyname = openfile_fp.baseName();

    uiTaskRunner uitr( parent_ );
    PtrMan<SurveyFile> newsurvey = new SurveyFile( openfile_fp.fullPath(),
						   surveyname.buf() );
    if ( !newsurvey->mount(true, &uitr).isOK() )
    {
	uiMSG().errorWithDetails( newsurvey->errMsg() );
	return false;
    }

    const FilePath fp( mGetSWDirDataDir(), SurveyInfo::sKeyBasicSurveyName());
    PtrMan<SurveyInfo> newsurvinfo = SurveyInfo::read( fp.fullPath() );
    if ( !newsurvinfo )
    {
	uiMSG().error( tr( "Cannot read software default survey\n"
			   "Try to reinstall the OpendTect package") );
	return false;
    }
    newsurvinfo->setName( surveyname );

    BufferString dataroot( newsurvey->getTempBaseDir() );
    uiStartNewSurveySetup setup_dlg( parent_, dataroot, *newsurvinfo );
    setup_dlg.setSurveyNameFld( surveyname, false );
    if ( !setup_dlg.go() )
	return false;

    const BufferString orgdirname = newsurvinfo->getDirName().buf();
    if ( !uiSurveyInfoEditor::copySurv(
	mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()), 0, dataroot,
								   orgdirname) )
    {
	uiMSG().error( tr("Cannot make a copy of the default survey") );
	return false;
    }

    const_cast<SurveyDiskLocation&>(
			newsurvinfo->diskLocation()).setBasePath( dataroot );
    const_cast<SurveyDiskLocation&>(
			newsurvinfo->diskLocation()).setDirName( orgdirname );
    const BufferString storagedir = FilePath( dataroot ).add( orgdirname )
								.fullPath();

    File::setSystemFileAttrib( storagedir, true );
    if ( !File::makeWritable( storagedir, true, true ) )
    {
	uiMSG().error( tr("Cannot set the permissions for the new survey") );
	return false;
    }
    SetCurBaseDataDirOverrule( dataroot );
    uiSurveyInfoEditor info_dlg( parent_, *(newsurvinfo.release()), true );
    info_dlg.setNameandPathSensitive( false, false );
    if ( !info_dlg.isOK() || !info_dlg.go() )
	return false;

    if ( !newsurvey->activate().isOK() )
    {
	uiMSG().errorWithDetails( newsurvey->errMsg() );
	return false;
    }

    if ( survfile_ && !closeFile() )
    {
	survfile_->activate();
	return false;
    }

    survfile_ = newsurvey.release();
    IOPar* impiop = info_dlg.getImportPars();
    uiSurvInfoProvider* impsip = info_dlg.getSIP();
    if ( impiop && impsip )
    {
	const char* askq = impsip->importAskQuestion();
	if ( askq && *askq && uiMSG().askGoOn(mToUiStringTodo(askq)) )
	{
	    MultiID mid;
	    if ( impiop->get( sKey::ID(), mid ) )
	    {
		IOM().to( mid );
	    }
	    else
		IOM().to( IOObjContext::Seis );

	    impsip->startImport( parent_, *impiop );
	}
    }

    return true;
}


bool uiSurveyFile::openFile()
{
    uiFileDialog fdlg( parent_, uiFileDialog::ExistingFile, nullptr,
			 SurveyFile::filtStr(), tr("Select a project file") );
    fdlg.setDirectory( GetPersonalDir() );
    fdlg.setDefaultExtension( SurveyFile::extStr() );
    if ( !fdlg.go() )
	return false;

    FilePath openfile_fp = FilePath( fdlg.fileName() );
    openfile_fp.setExtension( SurveyFile::extStr() );

    return openFile( openfile_fp.fullPath() );
}


bool uiSurveyFile::openFile( const char* filenm )
{
    if ( !File::exists(filenm) )
    {
	uiMSG().error( uiStrings::phrCannotOpenForRead( filenm ) );
	return false;
    }

    uiTaskRunner uitr( parent_ );
    PtrMan<SurveyFile> newsurvey = new SurveyFile( filenm, false );
    if ( !newsurvey->mount(false, &uitr).isOK() )
    {
	uiMSG().errorWithDetails( newsurvey->errMsg() );
	return false;
    }

    if ( !newsurvey->activate().isOK() )
    {
	uiMSG().errorWithDetails( newsurvey->errMsg() );
	return false;
    }

    if ( survfile_ && !closeFile() )
    {
	survfile_->activate();
	return false;
    }

    survfile_ = newsurvey.release();
    return true;
}


bool uiSurveyFile::closeFile()
{
    if ( !survfile_ || !survfile_->isMounted() || !survfile_->isOK() )
	return false;

    const int res = uiMSG().askSave( tr("Save project changes to %1")
					    .arg(survfile_->getSurveyFile()) );

    if ( res==-1 )
	return false;

    if ( res==1 )
    {
	uiTaskRunner uitr( parent_ );
	if ( !survfile_->unmount(true, &uitr).isOK() )
	{
	    uiMSG().errorWithDetails( survfile_->errMsg() );
	    return false;
	}
    }
    deleteAndZeroPtr( survfile_ );
    return true;
}

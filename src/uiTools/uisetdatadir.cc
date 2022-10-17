/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisetdatadir.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uifiledlg.h"
#include "uifileinput.h"
#include "uihelpview.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uisurveyzip.h"
#include "uitoolbutton.h"

#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "oddirs.h"
#include "odinst.h"
#include "od_helpids.h"
#include "settings.h"

#ifdef __win__
# include "winutils.h"
#endif


extern "C" { mGlobal(Basic) void SetCurBaseDataDir(const char*); }

static const char* doSetRootDataDir( const char* inpdatadir )
{
    mDeclStaticString(msg);
    BufferString datadir = inpdatadir;

    uiRetVal uirv = IOMan::isValidDataRoot( datadir );
    if ( !uirv.isOK() )
    {
	msg = uirv.getText();
	return msg.buf();
    }

    SetCurBaseDataDir( datadir );
    if ( !SetSettingsDataDir(datadir,uirv) )
    {
	msg = uirv.getText();
	return msg.buf();
    }

    return nullptr;
}


static const char* getPrefix()
{ return __iswin__ ? "Windows" : "Unix"; }

static void getRecentDataRoots( BufferStringSet& dirs )
{
    Settings& setts = Settings::fetch( "dataroot" );
    PtrMan<IOPar> dr = setts.subselect( getPrefix() );
    if ( !dr )
	return;

    BufferString path;
    for ( int idx=0; idx<dr->size(); idx++ )
    {
	if ( dr->get(toString(idx),path) && !path.isEmpty() )
	    dirs.add( path );
    }
}


static void addDataRootIfNew( BufferStringSet& dataroots, const char* newdr )
{
    for ( int idx=0; idx<dataroots.size(); idx++ )
    {
	const BufferString cpath = File::getCanonicalPath( dataroots.get(idx) );
	const BufferString newcpath = File::getCanonicalPath( newdr );
	if ( cpath==newcpath )
	    return;
    }

    dataroots.insertAt( new BufferString(newdr), 0 );
}


uiSetDataDir::uiSetDataDir( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Set OpendTect's Survey Data Root"),
				     tr("Specify a Data Root folder"),
				     mODHelpKey(mSetDataDirHelpID) ))
    , curdatadir_(GetBaseDataDir())
{
    const uiRetVal uirv = IOMan::isValidDataRoot( curdatadir_ );
    const bool oldok = uirv.isOK();
    BufferString basedirnm;
    uiString titletxt;

    if ( !curdatadir_.isEmpty() )
    {
	if ( oldok )
	{
	    titletxt = uiString::emptyString();
	    basedirnm = curdatadir_;
	}
	else
	{
	    titletxt = tr("OpendTect needs a place to store your projects.\n"
			"\nThe current Survey Data Root is invalid.\n"
			"Please locate a valid Data Root folder or\n"
			"select a recent Data Root");

	    basedirnm = curdatadir_;
	}
    }
    else
    {
	titletxt =
	    tr("OpendTect needs a place to store your projects:"
	    " the Survey Data Root.\n\n"
	    "You have not yet specified a location for it,\n"
	    "and there is no 'DTECT_DATA' set in your environment.\n\n"
	    "Please specify where the Survey Data Root should\n"
	    "be created or select an existing Survey Data Root folder.\n"
#ifndef __win__
	    "\nNote that you can still put surveys and "
	    "individual cubes on other disks;\nbut this is where the "
	    "'base' data store will be."
#endif
	    );
	const FilePath fp( GetPersonalDir(), "ODData" );
	basedirnm = fp.fullPath();
    }

    setTitleText( titletxt );

    const uiString basetxt = tr("Survey Data Root");
    basedirfld_ = new uiFileInput( this, basetxt,
			      uiFileInput::Setup(uiFileDialog::Gen,basedirnm)
			      .directories(true) );
    basedirfld_->setStretch( 2, 0 );
    mAttachCB( basedirfld_->valueChanged, uiSetDataDir::rootCheckCB );

    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Recent Data Roots") );
    dirlistfld_ = new uiListBox( this, su );
    dirlistfld_->attach( alignedBelow, basedirfld_ );

    getRecentDataRoots( dirlist_ );
    addDataRootIfNew( dirlist_, curdatadir_ );
    updateListFld();
    dirlistfld_->resizeToContents();

    auto* sortgrp = new uiButtonGroup( this, "", OD::Vertical );
    new uiToolButton( sortgrp, uiToolButton::UpArrow,uiStrings::sMoveUp(),
		      mCB(this,uiSetDataDir,rootMoveUpCB) );
    new uiToolButton( sortgrp, uiToolButton::DownArrow, uiStrings::sMoveDown(),
		      mCB(this,uiSetDataDir,rootMoveDownCB) );
    new uiToolButton( sortgrp, "remove", uiStrings::sRemove(),
		      mCB(this,uiSetDataDir,rootRemoveCB) );
    sortgrp->attach( rightOf, dirlistfld_ );

    dirlistfld_->setCurrentItem( curdatadir_.buf() );
    mAttachCB( dirlistfld_->selectionChanged, uiSetDataDir::rootSelCB );
}


uiSetDataDir::~uiSetDataDir()
{
    detachAllNotifiers();
}


void uiSetDataDir::updateListFld()
{
    const BufferString curtext = dirlistfld_->getText();
    dirlistfld_->setEmpty();
    dirlistfld_->addItems( dirlist_ );

    dirlistfld_->setCurrentItem( curtext.buf() );
}


void uiSetDataDir::rootCheckCB( CallBacker* )
{
    // Check if in survey
    BufferString seldir = basedirfld_->text();
    const FilePath fpsurvey( seldir, ".survey" );
    const FilePath fpseis( seldir, "Seismics" );
    if ( fpsurvey.exists() && fpseis.exists() )
    {
	seldir = fpsurvey.dirUpTo( fpsurvey.nrLevels()-3 );
	basedirfld_->setFileName( seldir );
    }
}


void uiSetDataDir::rootSelCB( CallBacker* cb )
{
    const BufferString newdr = dirlistfld_->getText();
    basedirfld_->setFileName( newdr );

    rootCheckCB( cb );
}


void uiSetDataDir::rootMoveUpCB( CallBacker* )
{
    const int curitm = dirlistfld_->currentItem();
    if ( curitm<=0 )
	return;

    dirlist_.swap( curitm, curitm-1 );
    updateListFld();
}


void uiSetDataDir::rootMoveDownCB( CallBacker* )
{
    const int curitm = dirlistfld_->currentItem();
    if ( curitm<0 || curitm==dirlist_.size()-1 )
	return;

    dirlist_.swap( curitm, curitm+1 );
    updateListFld();
}


void uiSetDataDir::rootRemoveCB( CallBacker* )
{
    const int curitm = dirlistfld_->currentItem();
    if ( dirlist_.isEmpty() )
	return;

    dirlist_.removeSingle( curitm );
    int newcuritm = curitm;
    if ( newcuritm >= dirlist_.size() )
	newcuritm = dirlist_.size()-1;

    dirlistfld_->setCurrentItem( newcuritm );
    updateListFld();
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiSetDataDir::acceptOK( CallBacker* )
{
    seldir_ = basedirfld_->text();
    if ( seldir_.isEmpty() ||
	 (File::exists(seldir_) && !File::isDirectory(seldir_)) )
	mErrRet( tr("Please enter a valid (existing) location") )
    else if ( !File::isDirectory(seldir_) )
    {
	const FilePath fp( seldir_ );
	const BufferString selbasedir = fp.pathOnly();
	if ( File::isDirectory(selbasedir.buf()) &&
	     File::isWritable(selbasedir.str()) &&
	     !File::exists(seldir_.str()) )
	{
	    if ( uiMSG().askGoOn(tr("Directory '%1' does not exists")
				    .arg(seldir_),
				uiStrings::sCreate(), uiStrings::sCancel()) )
		File::createDir( seldir_ );
	    else
		return false;
	}
	else
	    mErrRet( tr("Please enter a valid (existing) location") )
    }

    if ( seldir_ == curdatadir_ && IOMan::isValidDataRoot(seldir_).isOK() )
    {
	writeSettings();
	return true;
    }

    FilePath fpdd( seldir_ ); FilePath fps( GetSoftwareDir(0) );
    const int nrslvls = fps.nrLevels();
    if ( fpdd.nrLevels() >= nrslvls )
    {
	const BufferString ddatslvl( fpdd.dirUpTo(nrslvls-1) );
	if ( ddatslvl == fps.fullPath() )
	{
	    uiMSG().error( tr("The folder you have chosen is"
		   "\n *INSIDE*\nthe software installation folder."
		   "\nThis leads to many problems, and we cannot support this."
		   "\n\nPlease choose another folder") );
	    return false;
	}
    }

    writeSettings();
    return true;
}


bool uiSetDataDir::writeSettings()
{
    addDataRootIfNew( dirlist_, seldir_ );

    const char* prefix = getPrefix();
    Settings& setts = Settings::fetch( "dataroot" );
    setts.removeSubSelection( prefix );
    for ( int idx=0; idx<dirlist_.size(); idx++ )
	setts.set( IOPar::compKey(prefix,idx), dirlist_.get(idx) );

    return setts.write( false );
}


static BufferString getInstalledDemoSurvey()
{
    BufferString ret;
    if ( ODInst::getPkgVersion("demosurvey") )
    {
	FilePath demosurvfp( mGetSWDirDataDir(), "DemoSurveys",
			     "F3_Start.zip" );
	ret = demosurvfp.fullPath();
    }

    if ( !File::exists(ret) )
	ret.setEmpty();

    return ret;
}


bool uiSetDataDir::setRootDataDir( uiParent* par, const char* inpdatadir )
{
    BufferString datadir = inpdatadir;
    const char* retmsg = doSetRootDataDir( datadir );
    if ( !retmsg )
    {
	const DirList survdl( datadir, File::DirsInDir );
	for ( int idx=0; idx<survdl.size(); idx++ )
	{
	    if ( IOMan::isValidSurveyDir(survdl.fullPath(idx)).isOK() )
		return true;
	}

	offerUnzipSurv( par, datadir );
	return true;
    }

    const BufferString stdomf( mGetSetupFileName("omf") );

#define mCrOmfFname FilePath( datadir ).add( ".omf" ).fullPath()
    BufferString omffnm = mCrOmfFname;

    if ( !File::exists(datadir) )
    {
#ifdef __win__
	BufferString progfiles=GetSpecialFolderLocation(CSIDL_PROGRAM_FILES);

	if ( ( !progfiles.isEmpty() &&
	       !strncasecmp(progfiles, datadir, strlen(progfiles)) )
	  || datadir.contains( "Program Files" )
	  || datadir.contains( "program files" )
	  || datadir.contains( "PROGRAM FILES" ) )
	    mErrRet( tr("Please do not try to use 'Program Files' for data.\n"
		     "Instead, a folder like 'My Documents' would be OK.") )
#endif
	if ( !File::createDir( datadir ) )
	    mErrRet( uiStrings::phrCannotCreateDirectory(toUiString(datadir)) )
    }

    bool offerunzipsurv = false;
    while ( !IOMan::isValidDataRoot(datadir).isOK() )
    {
	if ( !File::isDirectory(datadir) )
	   mErrRet(tr("A file (not a folder) with this name already exists"))

	if ( !File::isWritable(datadir) )
	    mErrRet( tr("Folder %1 is not writable.\n\n"
			"Please change permissions or select\n"
			"another Data Root folder.").arg(datadir) )

	if ( File::exists(omffnm) )
	{
	    // most likely a survey directory (see IOMan::isValidDataRoot())
	    const BufferString parentdir = FilePath(datadir).pathOnly();
	    uiString msg = tr( "Target folder:\n%1\nappears to be an OpendTect "
		"survey folder.\n\nDo you want to set its parent:\n%2\nas the "
		"OpendTect Data Root?").arg(datadir).arg(parentdir);
	    if ( !uiMSG().askGoOn(msg) )
		return false;

	    datadir = parentdir;
	    omffnm = mCrOmfFname;
	    offerunzipsurv = false;
	    continue;
	}

	offerunzipsurv = true;
	if ( !DirList(datadir).isEmpty() )
	{
	    const DirList survdl( datadir, File::DirsInDir );
	    bool hasvalidsurveys = false;
	    for ( int idx=0; idx<survdl.size(); idx++ )
	    {
		if ( IOMan::isValidSurveyDir(survdl.fullPath(idx)).isOK() )
		    hasvalidsurveys = true;
	    }

	    if ( hasvalidsurveys )
		offerunzipsurv = false;
	    else
	    {
		uiString msg = tr("The target folder:\n%1"
		    "\nis not an OpendTect Data Root folder. "
		    "It already contains files though."
		    "\n\nDo you want to convert this folder into an "
		    "OpendTect Data Root?"
		    "\n(this process will not remove the existing files)")
		    .arg(datadir);
		if ( !uiMSG().askGoOn( msg ) )
		    return false;
	    }
	}

	File::copy( stdomf, omffnm );
	if ( !File::exists(omffnm) )
	    mErrRet(tr("Could not convert selected folder into an "
		       "OpendTect Data Root.\n"
		       "Most probably you have no write permissions for:\n%1")
		  .arg(datadir))

	break;
    }

    if ( offerunzipsurv )
	offerUnzipSurv( par, datadir );

    retmsg = doSetRootDataDir( datadir );
    if ( retmsg )
	{ uiMSG().error( mToUiStringTodo(retmsg) ); return false; }

    return true;
}


static void terraNubisCB( CallBacker* )
{
    const HelpKey key( WebsiteHelp::sKeyFactoryName(),
		      WebsiteHelp::sKeyFreeProjects() );
    HelpProvider::provideHelp( key );
}


void uiSetDataDir::offerUnzipSurv( uiParent* par, const char* datadir )
{
    BufferString zipfilenm = getInstalledDemoSurvey();
    const bool havedemosurv = File::exists( zipfilenm.buf() );
    BufferStringSet opts;
    if ( havedemosurv )
	opts.add("Install the F3 Demo Survey from the OpendTect installation");
    opts.add( "Unzip a survey zip file" );
    opts.add( "I will set up a new survey myself" );

    uiGetChoice uigc( par, opts, uiStrings::phrSelect(tr("next action")) );
    auto* pb = new uiPushButton( &uigc,
			tr("visit TerraNubis (for public domain surveys)"),
			mSCB(terraNubisCB), true );
    pb->attach( rightAlignedBelow, uigc.bottomFld() );
    if ( !par )
	uigc.setModal( true );

    if ( uigc.go() == uiDialog::Rejected || uigc.choice() == opts.size()-1 )
	return;

    if ( (havedemosurv && uigc.choice() == 1) ||
	 (!havedemosurv && uigc.choice() == 0) )
    {
	uiFileDialog dlg( par, true, "", "*.zip", tr("Select zip file") );
	if ( !dlg.go() )
	    return;

	zipfilenm = dlg.fileName();
	if ( !File::exists(zipfilenm.buf()) )
	    return;
    }

    const bool isstartsurv = havedemosurv && uigc.choice() == 0;
    FilePath startsurvcp;
    if ( isstartsurv )
    {
	startsurvcp.set( datadir )
		   .add( FilePath(zipfilenm).fileName() );
	if ( !File::copy(zipfilenm.str(),startsurvcp.fullPath()) )
	    return;

	zipfilenm = startsurvcp.fullPath();
    }

    (void)uiSurvey_UnzipFile( par, zipfilenm.str(), datadir );
    if ( isstartsurv )
	File::remove( startsurvcp.fullPath() );
}


using fromFromSdlUiParSdlPtrFn = bool(*)(SurveyDiskLocation&,uiParent*,
			const SurveyDiskLocation*,uiDialog::DoneResult* );
static fromFromSdlUiParSdlPtrFn dosurvselfn_ = nullptr;

mGlobal(uiTools) void setGlobal_uiTools_SurvSelFns(fromFromSdlUiParSdlPtrFn);
void setGlobal_uiTools_SurvSelFns( fromFromSdlUiParSdlPtrFn dosurvselfn )
{
    dosurvselfn_ = dosurvselfn;
}

extern "C" {
mGlobal(uiTools) bool doSurveySelectionDlg(SurveyDiskLocation&,uiParent*,
					const SurveyDiskLocation*,
					uiDialog::DoneResult*);
}

mExternC(uiTools) bool doSurveySelectionDlg( SurveyDiskLocation& newsdl,
				uiParent* p, const SurveyDiskLocation* cursdl,
				uiDialog::DoneResult* doneres )
{
    return dosurvselfn_ ? (*dosurvselfn_)(newsdl,p,cursdl,doneres) : false;
}

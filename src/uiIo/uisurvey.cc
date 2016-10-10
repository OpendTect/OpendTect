/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uisurvey.h"

#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uiconvpos.h"
#include "uicoordsystem.h"
#include "uidesktopservices.h"
#include "uifileinput.h"
#include "uifont.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uipixmap.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uisplitter.h"
#include "uisetdatadir.h"
#include "uisettings.h"
#include "uisip.h"
#include "uisurveyselect.h"
#include "uisurvinfoed.h"
#include "uisurvmap.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "ui2dsip.h"

#include "angles.h"
#include "ioobjctxt.h"
#include "trckeyzsampling.h"
#include "dirlist.h"
#include "envvars.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "dbman.h"
#include "iopar.h"
#include "iostrm.h"
#include "latlong.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "odver.h"
#include "od_ostream.h"
#include "od_helpids.h"
#include "settings.h"
#include "survinfo.h"


#include "uilineedit.h"


static const char*	sZipFileMask = "ZIP files (*.zip *.ZIP)";
#define mErrRetVoid(s)	{ if ( s.isSet() ) uiMSG().error(s); return; }
#define mErrRet(s)	{ if ( s.isSet() ) uiMSG().error(s); return false; }

static int sMapWidth = 300;
static int sMapHeight = 300;

//--- General tools


static ObjectSet<uiSurvey::Util>& getUtils()
{
    mDefineStaticLocalObject( PtrMan<ManagedObjectSet<uiSurvey::Util> >,
			      utils, = 0 );
    if ( !utils )
    {
	ManagedObjectSet<uiSurvey::Util>* newutils =
				    new ManagedObjectSet<uiSurvey::Util>;
	*newutils += new uiSurvey::Util( "xy2ic",
		od_static_tr("uiSurvey_getUtils",
		"Convert (X,Y) to/from Inline/Crossline"), CallBack() );
	*newutils += new uiSurvey::Util( "spherewire",
				od_static_tr("uiSurvey_getUtils",
				"Setup geographical coordinates"), CallBack() );

	utils.setIfNull(newutils,true);
    }

    return *utils;
}


static BufferString getTrueDir( const char* dn )
{
    BufferString dirnm = dn;
    FilePath fp;
    while ( File::isLink(dirnm) )
    {
	BufferString newdirnm = File::linkEnd(dirnm);
	fp.set( newdirnm );
	if ( !fp.isAbsolute() )
	{
	    FilePath dirnmfp( dirnm );
	    dirnmfp.setFileName( 0 );
	    fp.setPath( dirnmfp.fullPath() );
	}
	dirnm = fp.fullPath();
    }
    return dirnm;
}


//--- uiNewSurveyByCopy


class uiNewSurveyByCopy : public uiDialog
{ mODTextTranslationClass(uiNewSurveyByCopy);

public:

uiNewSurveyByCopy( uiParent* p, const char* dataroot, const char* dirnm )
	: uiDialog(p,uiDialog::Setup(uiStrings::phrCopy(uiStrings::sSurvey()),
        mNoDlgTitle, mODHelpKey(mNewSurveyByCopyHelpID)))
	, dataroot_(dataroot)
{
    BufferString curfnm;
    if ( dirnm && *dirnm )
	curfnm = FilePath( dataroot_, dirnm ).fullPath();
    else
	curfnm = dataroot_;

    new uiLabel( this, mTODONotImplPhrase() );
}

void inpSel( CallBacker* )
{
    BufferString fullpath;
    FilePath fp( fullpath );
}

bool copySurv()
{
    if ( File::exists(newdirnm_) )
    {
        uiMSG().error(tr("A survey '%1' already exists.\n"
			 "You will have to remove it first").arg(newdirnm_));
        return false;
    }

    uiTaskRunner taskrunner( this );
    const BufferString fromdir = getTrueDir( inpdirnm_ );
    PtrMan<Executor> copier = File::getRecursiveCopier( fromdir, newdirnm_ );
    if ( !taskrunner.execute(*copier) )
	{ uiMSG().error(tr("Cannot copy the survey data")); return false; }

    File::makeWritable( newdirnm_, true, true );
    return true;
}

bool acceptOK()
{
    return copySurv();
}

    const BufferString	dataroot_;
    BufferString	inpdirnm_;
    BufferString	newdirnm_;

};



//--- uiSurvey


uiSurvey::uiSurvey( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Survey Setup and Selection"),
				 mNoDlgTitle,mODHelpKey(mSurveyHelpID)))
    , orgdataroot_(GetBaseDataDir())
    , dataroot_(GetBaseDataDir())
    , initialsurveyname_(GetSurveyName())
    , cursurvinfo_(0)
    , survmap_(0)
    , dirfld_(0)
    , impiop_(0)
    , impsip_(0)
    , parschanged_(false)
    , cursurvremoved_(false)
    , freshsurveyselected_(false)
{
    const CallBack selchgcb( mCB(this,uiSurvey,selChange) );

    if ( dataroot_.isEmpty() )
    {
	new uiLabel( this,
		tr("Cannot establish a 'Survey Data Root' directory."
		"\nOpendTect needs a place to store its files."
		"\nPlease consult the documentation at opendtect.org,"
		"\nor contact support@opendtect.org.") );
    }

    if ( !DBM().isBad() )
	setCurrentSurvInfo( new SurveyInfo(SI()) );

    uiGroup* topgrp = new uiGroup( this, "TopGroup" );
    uiPushButton* datarootbut =
		new uiPushButton( topgrp, tr("Survey Data Root"), false );

    datarootbut->setIcon( "database" );
    datarootbut->activated.notify( mCB(this,uiSurvey,dataRootPushed) );
    datarootbut->attach( leftBorder );

    datarootlbl_ = new uiLineEdit( topgrp, "Data Root Label" );
    datarootlbl_->setHSzPol( uiObject::WideMax );
    datarootlbl_->setReadOnly();
    datarootlbl_->setBackgroundColor( backgroundColor() );
    datarootlbl_->attach( rightOf, datarootbut );

    uiPushButton* settbut = new uiPushButton( topgrp, tr("General Settings"),
				mCB(this,uiSurvey,odSettsButPush), false );
    settbut->setIcon( "settings" );
    settbut->attach( rightBorder );

    uiSeparator* sep1 = new uiSeparator( topgrp, "Separator 1" );
    sep1->attach( stretchedBelow, datarootbut );

    uiGroup* leftgrp = new uiGroup( topgrp, "Survey selection left" );
    uiGroup* rightgrp = new uiGroup( topgrp, "Survey selection right" );

    fillLeftGroup( leftgrp );
    fillRightGroup( rightgrp );
    leftgrp->attach( ensureBelow, sep1 );
    rightgrp->attach( rightOf, leftgrp );

    uiLabel* infolbl = new uiLabel( topgrp, uiString::emptyString() );
    infolbl->setPixmap( uiPixmap("info") );
    infolbl->setToolTip( tr("Survey Information") );
    infolbl->attach( alignedBelow, leftgrp );
    infofld_ = new uiTextEdit( topgrp, "Info", true );
    infofld_->setPrefHeightInChar( 7 );
    infofld_->setStretch( 2, 1 );
    infofld_->attach( rightTo, infolbl );
    infofld_->attach( ensureBelow, rightgrp );

    uiGroup* botgrp = new uiGroup( this, "Bottom Group" );
    uiLabel* notelbl = new uiLabel( botgrp, uiStrings::sEmptyString() );
    notelbl->setPixmap( uiPixmap("notes") );
    notelbl->setToolTip( tr("Notes") );
    notelbl->setMaximumWidth( 32 );

    notesfld_ = new uiTextEdit( botgrp, "Survey Notes" );
    notesfld_->attach( rightTo, notelbl );
    notesfld_->setPrefHeightInChar( 5 );
    notesfld_->setStretch( 2, 2 );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

    putToScreen();
    updateDataRootLabel();
    setOkText( uiStrings::sSelect() );
    postFinalise().notify( selchgcb );
}


uiSurvey::~uiSurvey()
{
    delete impiop_;
    delete cursurvinfo_;
}


static void osrbuttonCB( void* )
{
    uiDesktopServices::openUrl( "https://opendtect.org/osr" );
}


void uiSurvey::fillLeftGroup( uiGroup* grp )
{
    dirfld_ = new uiListBox( grp, "Surveys" );
    updateSurvList();
    dirfld_->selectionChanged.notify( mCB(this,uiSurvey,selChange) );
    dirfld_->doubleClicked.notify( mCB(this,uiSurvey,accept) );
    dirfld_->setHSzPol( uiObject::WideVar );
    dirfld_->setStretch( 2, 2 );

    uiButtonGroup* butgrp =
	new uiButtonGroup( grp, "Buttons", OD::Vertical );
    butgrp->attach( rightTo, dirfld_ );
    new uiToolButton( butgrp, "addnew", uiStrings::phrCreate(mJoinUiStrs(sNew(),
				sSurvey())), mCB(this,uiSurvey,newButPushed) );
    editbut_ = new uiToolButton( butgrp, "edit", tr("Edit Survey Parameters"),
				 mCB(this,uiSurvey,editButPushed) );
    new uiToolButton( butgrp, "copyobj",
	tr("Copy Survey"), mCB(this,uiSurvey,copyButPushed) );
    new uiToolButton( butgrp, "compress",
	tr("Compress survey as zip archive"),
	mCB(this,uiSurvey,compressButPushed) );
    new uiToolButton( butgrp, "extract",
	tr("Extract survey from zip archive"),
	mCB(this,uiSurvey,extractButPushed) );
    new uiToolButton( butgrp, "share",
	tr("Share surveys through the OpendTect Seismic Repository"),
	mSCB(osrbuttonCB) );
    rmbut_ = new uiToolButton( butgrp, "delete", tr("Delete Survey"),
			       mCB(this,uiSurvey,rmButPushed) );
}


void uiSurvey::fillRightGroup( uiGroup* grp )
{
    survmap_ = new uiSurveyMap( grp );
    survmap_->attachGroup().setPrefWidth( sMapWidth );
    survmap_->attachGroup().setPrefHeight( sMapHeight );

    uiButton* lastbut = 0;
    ObjectSet<uiSurvey::Util>& utils = getUtils();
    const CallBack cb( mCB(this,uiSurvey,utilButPush) );
    for ( int idx=0; idx<utils.size(); idx++ )
    {
	const uiSurvey::Util& util = *utils[idx];
	uiToolButton* but = new uiToolButton( grp, util.pixmap_,
					      util.tooltip_, cb );
	but->setToolTip( util.tooltip_ );
	utilbuts_ += but;
	if ( !lastbut )
	    but->attach( rightTo, &survmap_->attachGroup() );
	else
	    but->attach( alignedBelow, lastbut );
	lastbut = but;
    }
}


void uiSurvey::add( const uiSurvey::Util& util )
{
    getUtils() += new uiSurvey::Util( util );
}


const char* uiSurvey::selectedSurveyName() const
{
    return dirfld_->getText();
}


bool uiSurvey::rootDirWritable() const
{
    if ( !File::isWritable(dataroot_) )
    {
	uiString msg = tr("Cannot create new survey in\n"
			  "%1.\nDirectory is write protected.")
	    .arg(dataroot_);
	uiMSG().error( msg );
	return false;
    }
    return true;
}


void uiSurvey::updateDataRootInSettings()
{
    Settings::common().set( "Default DATA directory", dataroot_ );
    if ( !Settings::common().write() )
	uiMSG().warning( uiStrings::phrCannotSave(tr("Survey Data Root "
			    "location in the settings file")) );
}


extern "C" { mGlobal(Basic) void SetBaseDataDir(const char*); }


bool uiSurvey::acceptOK()
{
    if ( !dirfld_ )
	return true;

    if ( dirfld_->isEmpty() )
	mErrRet(tr("Please create a survey (or press Cancel)"))

    const BufferString selsurv( selectedSurveyName() );
    const bool samedataroot = dataroot_ == orgdataroot_;
    const bool samesurvey = samedataroot && initialsurveyname_ == selsurv;

    // Step 1: write local changes
    if ( !writeSurvInfoFileIfCommentChanged() )
	mErrRet(uiString::emptyString())
    if ( samedataroot && samesurvey && !parschanged_ && !DBM().isBad() )
	return true;

    // Step 2: write default/current survey file
    if ( !writeSettingsSurveyFile() )
	mErrRet(uiString::emptyString())

    // Step 3: record data root preference
    if ( !samedataroot )
	updateDataRootInSettings();

    // Step 4: Do the IOMan changes necessary
    if ( samesurvey && !DBM().isBad() )
    {
	if ( cursurvinfo_ )
	    eSI() = *cursurvinfo_;

	// DBM().surveyParsChanged();
    }
    else
    {
	if ( !samedataroot )
	{
	    if ( !uiSetDataDir::setRootDataDir(this,dataroot_) )
		return false;
	}

	uiRetVal uirv = DBM().setDataSource( dataroot_,
					     cursurvinfo_->getDirName() );
	if ( uirv.isOK() )
	{
	    delete cursurvinfo_; cursurvinfo_ = 0;
	    if ( survmap_ )
		survmap_->setSurveyInfo( 0 );
	}
	else
	{
	    if ( cursurvinfo_ == &SI() )
	    {
		cursurvinfo_ = 0;
		if ( survmap_ )
		    survmap_->setSurveyInfo( 0 );
	    }

	    uiMSG().error( uirv );
	    return false;
	}
    }

    // Step 5: if fresh survey, help user on his/her way
    if ( impiop_ && impsip_ )
    {
	freshsurveyselected_ = true;
	readSurvInfoFromFile();
	const uiString askq = impsip_->importAskUiQuestion();
	if ( !askq.isEmpty() && uiMSG().askGoOn(askq) )
	    impsip_->startImport( parent(), *impiop_ );
    }

    return true;
}


bool uiSurvey::rejectOK()
{
    if ( cursurvremoved_ && !hasSurveys() )
    {
	uiString msg(tr("You have removed the current survey.\n"
	       "No surveys found in the list.\n"
		"Do you want to exit OpendTect?") );

	return uiMSG().askGoOn( msg );
    }

    return true;
}


bool uiSurvey::hasSurveys() const
{
    return dirfld_ && !dirfld_->isEmpty();
}


void uiSurvey::setCurrentSurvInfo( SurveyInfo* newsi, bool updscreen )
{
    delete cursurvinfo_; cursurvinfo_ = newsi;

    if ( updscreen )
	putToScreen();
    else if ( survmap_ )
	survmap_->setSurveyInfo( 0 );
}


void uiSurvey::rollbackNewSurvey( const uiString& errmsg )
{
    if ( !cursurvinfo_ )
	return;

    FilePath fp( cursurvinfo_->getBasePath(), cursurvinfo_->getDirName() );
    const bool haverem = File::removeDir( fp.fullPath() );
    setCurrentSurvInfo( 0, false );
    readSurvInfoFromFile();
    if ( !errmsg.isEmpty()  )
    {
	const uiString tousr = haverem ? tr("New survey removed because:\n%1")
		.arg(errmsg)
		: tr("New survey directory is invalid because:\n%1")
		.arg(errmsg);
	uiMSG().error( tousr );
    }
}


void uiSurvey::newButPushed( CallBacker* )
{
    if ( !rootDirWritable() )
    {
	uiMSG().error( tr("Current data root\n%1\ndoes not allow writing")
			.arg(dataroot_) );
	return;
    }

    uiMSG().error( mTODONotImplPhrase() );
    //TODO start creation program
}


void uiSurvey::rmButPushed( CallBacker* )
{
    const BufferString selnm( selectedSurveyName() );
    const BufferString seldirnm = FilePath(dataroot_).add(selnm).fullPath();
    const BufferString truedirnm = getTrueDir( seldirnm );

    uiString msg = tr("This will delete the entire survey directory:\n\t%1"
		      "\nFull path: %2").arg(selnm).arg(truedirnm);
    if ( !uiMSG().askRemove(msg) ) return;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    const bool rmisok = File::remove( truedirnm );
    MouseCursorManager::restoreOverride();
    if ( !rmisok )
	uiMSG().error(tr("%1\nnot removed properly").arg(truedirnm));

    if ( seldirnm != truedirnm ) // must have been a link
	if ( !File::remove(seldirnm) )
	    uiMSG().error( uiStrings::phrCannotRemove(tr(
					    "link to the removed survey")) );

    updateSurvList();
    const char* ptr = GetSurveyName();
    if ( ptr && selnm == ptr )
    {
	cursurvremoved_ = true;
	if ( button(CANCEL) )
	    button(CANCEL)->setSensitive( dirfld_->isEmpty() );
    }

    if ( dirfld_->isEmpty() )
    {
	button(CANCEL)->setText( tr("Exit") );
	setCurrentSurvInfo( 0, true );
    }

    selChange( 0 );
}


void uiSurvey::editButPushed( CallBacker* )
{
    if ( !cursurvinfo_ ) return; // defensive
    if ( doSurvInfoDialog(false) )
	putToScreen();
}


void uiSurvey::copyButPushed( CallBacker* )
{
    if ( !cursurvinfo_ || !rootDirWritable() ) return;

    uiNewSurveyByCopy dlg( this, dataroot_, selectedSurveyName() );
    if ( !dlg.go() )
	return;

    uiRetVal uirv = uiRetVal::OK();
    setCurrentSurvInfo( SurveyInfo::read(dlg.newdirnm_,uirv) );
    if ( !cursurvinfo_ )
    {
	uirv.insert( uiStrings::phrCannotRead(tr("the copied survey")) );
	uiMSG().error( uirv );
	return;
    }

    cursurvinfo_->setName( FilePath(dlg.newdirnm_).fileName() );
    cursurvinfo_->updateDirName();
    if ( !cursurvinfo_->write() )
	uiMSG().warning( uiStrings::phrCannotWrite(tr("updated survey info")) );

    updateSurvList();
    dirfld_->setCurrentItem( dlg.newdirnm_ );
}


void uiSurvey::extractButPushed( CallBacker* )
{
    if ( !rootDirWritable() ) return;

    uiFileDialog fdlg( this, true, 0, "*.zip", tr("Select survey zip file") );
    fdlg.setSelectedFilter( sZipFileMask );
    if ( !fdlg.go() )
	return;

    Survey::unzipFile( this, fdlg.fileName(), dataroot_ );
    updateSurvList();
    readSurvInfoFromFile();
    //TODO set unpacked survey as current with dirfld_->setCurrentItem()
}


void uiSurvey::compressButPushed( CallBacker* )
{
    const char* survnm( selectedSurveyName() );
    const uiString title = tr("Compress %1 survey as zip archive")
						.arg(survnm);
    uiDialog dlg( this,
    uiDialog::Setup(title,mNoDlgTitle,
		    mODHelpKey(mSurveyCompressButPushedHelpID) ));
    uiFileInput* fnmfld = new uiFileInput( &dlg,uiStrings::phrSelect(
		    uiStrings::phrOutput(tr("Destination"))),
		    uiFileInput::Setup().directories(false).forread(false)
		    .allowallextensions(false));
    fnmfld->setDefaultExtension( "zip" );
    fnmfld->setFilter( sZipFileMask );
    uiLabel* sharfld = new uiLabel( &dlg,
			  tr("You can share surveys to Open Seismic Repository."
			   "To know more ") );
    sharfld->attach( leftAlignedBelow,  fnmfld );
    uiPushButton* osrbutton =
	new uiPushButton( &dlg, tr("Click here"), mSCB(osrbuttonCB), false );
    osrbutton->attach( rightOf, sharfld );
    if ( !dlg.go() )
	return;

    FilePath zippath( fnmfld->fileName() );
    BufferString zipext = zippath.extension();
    if ( zipext != "zip" )
	mErrRetVoid(tr("Please add .zip extension to the file name"))

    Survey::zipDirectory( this, survnm, zippath.fullPath() );
}


void uiSurvey::dataRootPushed( CallBacker* )
{
    uiSetDataDir dlg( this );
    if ( !dlg.go() || dataroot_ == dlg.selectedDir() )
	return;

    dataroot_ = dlg.selectedDir();
    SetBaseDataDir( dataroot_ );

    updateSurvList();
    updateDataRootLabel();
    const char* ptr = GetSurveyName();
    if ( ptr && dirfld_->isPresent(ptr) )
	dirfld_->setCurrentItem( GetSurveyName() );

    selChange( 0 );
}


void uiSurvey::odSettsButPush( CallBacker* )
{
    uiSettingsDlg dlg( this );
    dlg.go();
}


void uiSurvey::utilButPush( CallBacker* cb )
{
    if ( !cursurvinfo_ )
	return;
    mDynamicCastGet(uiButton*,tb,cb)
    if ( !tb )
	{ pErrMsg("Huh"); return; }

    const int butidx = utilbuts_.indexOf( tb );
    if ( butidx < 0 ) { pErrMsg("Huh"); return; }

    if ( butidx == 0 )
    {
	uiConvertPos dlg( this, *cursurvinfo_ );
	dlg.go();
    }
    else if ( butidx == 1 )
    {
	if ( !cursurvinfo_ ) return;

	uiSingleGroupDlg<Coords::uiPositionSystemSel> dlg( this,
	    new Coords::uiPositionSystemSel( 0, true, cursurvinfo_,
					     cursurvinfo_->getCoordSystem() ));
	if ( dlg.go() )
	{
	    cursurvinfo_->setCoordSystem( dlg.getDlgGroup()->outputSystem() );
	    if ( !cursurvinfo_->write() )
		mErrRetVoid(uiStrings::phrCannotWrite(
					 uiStrings::sSetup().toLower()));
	}
    }
    else
    {
	Util* util = getUtils()[butidx];
	util->cb_.doCall( this );
    }
}


void uiSurvey::updateDataRootLabel()
{
    datarootlbl_->setText( dataroot_ );
}


void uiSurvey::updateSurvList()
{
    NotifyStopper ns( dirfld_->selectionChanged );
    int newselidx = dirfld_->currentItem();
    const BufferString prevsel( dirfld_->getText() );
    dirfld_->setEmpty();
    BufferStringSet dirlist; Survey::getDirectoryNames( dirlist, false,
	    						dataroot_ );
    dirfld_->addItems( dirlist );

    if ( dirfld_->isEmpty() )
	return;

    const int idxofprevsel = dirfld_->indexOf( prevsel );
    const int idxofcursi = cursurvinfo_ ? dirfld_->indexOf(
					  cursurvinfo_->getDirName() ) : -1;
    if ( idxofcursi >= 0 )
	newselidx = idxofcursi;
    else if ( idxofprevsel >= 0 )
	newselidx = idxofprevsel;

    if ( newselidx < 0 )
	newselidx = 0;
    if ( newselidx >= dirfld_->size() )
	newselidx = dirfld_->size()-1 ;

    dirfld_->setCurrentItem( newselidx );
}


bool uiSurvey::writeSettingsSurveyFile()
{
    if ( dirfld_->isEmpty() )
	{ pErrMsg( "No survey in the list" ); return false; }

    BufferString seltxt( selectedSurveyName() );
    if ( seltxt.isEmpty() )
	mErrRet(tr("Survey folder name cannot be empty"))

    if ( !File::exists(FilePath(dataroot_,seltxt).fullPath()) )
	mErrRet(tr("Survey directory does not exist anymore"))

    const char* survfnm = GetLastSurveyFileName();
    if ( !survfnm )
	mErrRet(tr("Internal error: cannot construct last-survey-filename"))

    od_ostream strm( survfnm );
    if ( !strm.isOK() )
	mErrRet(tr("Cannot open %1 for write").arg(survfnm))

    strm << seltxt;
    if ( !strm.isOK() )
	mErrRet( tr("Error writing to %1").arg(survfnm) )

    return true;
}


void uiSurvey::readSurvInfoFromFile()
{
    const BufferString survnm( selectedSurveyName() );
    SurveyInfo* newsi = 0;
    if ( !survnm.isEmpty() )
    {
	const BufferString fname = FilePath( dataroot_ )
			    .add( selectedSurveyName() ).fullPath();
	uiRetVal uirv = uiRetVal::OK();
	newsi = SurveyInfo::read( fname, uirv );
	if ( !newsi )
	    uiMSG().error( uirv );
    }

    if ( newsi )
	setCurrentSurvInfo( newsi );
}


bool uiSurvey::doSurvInfoDialog( bool isnew )
{
    delete impiop_; impiop_ = 0; impsip_ = 0;
    uiSurveyInfoEditor dlg( this, *cursurvinfo_ );
    if ( !dlg.isOK() )
	return false;

    dlg.survParChanged.notify( mCB(this,uiSurvey,updateInfo) );
    if ( !dlg.go() )
    {
	if ( !isnew )
	    readSurvInfoFromFile();
	return false;
    }

    if ( initialsurveyname_ == selectedSurveyName() )
	parschanged_ = true;

    updateSurvList();
    dirfld_->setCurrentItem( dlg.dirName() );

    impiop_ = dlg.impiop_; dlg.impiop_ = 0;
    impsip_ = dlg.lastsip_;

    return true;
}


void uiSurvey::selChange( CallBacker* )
{
    if ( dirfld_->isEmpty() )
	return;

    writeSurvInfoFileIfCommentChanged();
    readSurvInfoFromFile();
    putToScreen();
}


void uiSurvey::putToScreen()
{
    if ( !survmap_ ) return;

    survmap_->setSurveyInfo( cursurvinfo_ );
    const bool hassurveys = !dirfld_->isEmpty();
    rmbut_->setSensitive( hassurveys );
    editbut_->setSensitive( hassurveys );
    for ( int idx=0; idx<utilbuts_.size(); idx++ )
	utilbuts_[idx]->setSensitive( hassurveys );

    if ( !hassurveys )
    {
	notesfld_->setText( uiString::emptyString() );
	infofld_->setText( uiString::emptyString() );
	return;
    }

    BufferString locinfo( "Location: " );
    BufferString inlinfo( "In-line range: " );
    BufferString crlinfo( "Cross-line range: " );
    BufferString zinfo( "Z range" );
    BufferString bininfo( "Inl/Crl bin size" );
    BufferString areainfo( "Area" );
    BufferString survtypeinfo( "Survey type: " );
    BufferString orientinfo( "In-line Orientation: " );

    if ( cursurvinfo_ )
    {
	const SurveyInfo& si = *cursurvinfo_;
	notesfld_->setText( si.comment() );

	zinfo.add( "(" )
	     .add( si.zIsTime() ? ZDomain::Time().unitStr()
				: getDistUnitString(si.zInFeet(), false) )
	     .add( "): " );

	bininfo.add( " (" ).add( si.getXYUnitString(false) ).add( "/line): " );
	areainfo.add( " (sq " ).add( si.xyInFeet() ? "mi" : "km" ).add( "): ");

	if ( si.sampling(false).hsamp_.totalNr() > 0 )
	{
	    inlinfo.add( si.sampling(false).hsamp_.start_.inl() );
	    inlinfo.add( " - ").add( si.sampling(false).hsamp_.stop_.inl() );
	    inlinfo.add( " - " ).add( si.inlStep() );
	    crlinfo.add( si.sampling(false).hsamp_.start_.crl() );
	    crlinfo.add( " - ").add( si.sampling(false).hsamp_.stop_.crl() );
	    crlinfo.add( " - " ).add( si.crlStep() );

	    const float inldist = si.inlDistance(), crldist = si.crlDistance();

	    bininfo.add( toString(inldist,2) ).add( "/" );
	    bininfo.add( toString(crldist,2) );
	    float area = (float) ( si.getArea(false) * 1e-6 ); //in km2
	    if ( si.xyInFeet() )
		area /= 2.590; // square miles

	    areainfo.add( toString(area,2) );
	}

	#define mAdd2ZString(nr) zinfo += istime ? mNINT32(1000*nr) : nr;

	const bool istime = si.zIsTime();
	mAdd2ZString( si.zRange(false).start );
	zinfo += " - "; mAdd2ZString( si.zRange(false).stop );
	zinfo += " - "; mAdd2ZString( si.zRange(false).step );
	survtypeinfo.add( SurveyInfo::toString(si.survDataType()) );

	FilePath fp( si.getBasePath(), si.getDirName() );
	fp.makeCanonical();
	locinfo.add( fp.fullPath() );

	const float usrang = Math::degFromNorth( si.angleXInl() );
	orientinfo.add( toString(usrang,2) ).add( " Degrees from N" );
    }
    else
    {
	notesfld_->setText( "" );
	zinfo.add( ":" ); bininfo.add( ":" ); areainfo.add( ":" );
    }

    BufferString infostr;
    infostr.add( inlinfo ).addNewLine().add( crlinfo ).addNewLine()
	.add( zinfo ).addNewLine().add( bininfo ).addNewLine()
	.add( areainfo ).add( "; ").add( survtypeinfo )
	.addNewLine().add( orientinfo ).addNewLine().add( locinfo );
    infofld_->setText( infostr );

}


bool uiSurvey::writeSurvInfoFileIfCommentChanged()
{
    if ( !cursurvinfo_ || !notesfld_->isModified() )
	return true;

    cursurvinfo_->setComment( notesfld_->text() );
    if ( !cursurvinfo_->write( dataroot_ ) )
	mErrRet(tr("Failed to write survey info.\nNo changes committed."))

    return true;
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uisurveymanager.h"

#include "uidatarootsel.h"
#include "uisurvinfoed.h"
#include "uiusercreatesurvey.h"
#include "uisurvmap.h"

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
#include "uisettings.h"
#include "uisip.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

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


static const char*	sZipFileMask = "ZIP files (*.zip *.ZIP)";
#define mErrRetVoid(s)	{ if ( s.isSet() ) uiMSG().error(s); return; }
#define mErrRet(s)	{ if ( s.isSet() ) uiMSG().error(s); return false; }

static int sMapWidth = 300;
static int sMapHeight = 300;

//--- General tools


static ObjectSet<uiSurveyManager::Util>& getUtils()
{
    mDefineStaticLocalObject( PtrMan<ManagedObjectSet<uiSurveyManager::Util> >,
			      utils, = 0 );
    if ( !utils )
    {
	ManagedObjectSet<uiSurveyManager::Util>* newutils =
				    new ManagedObjectSet<uiSurveyManager::Util>;
	*newutils += new uiSurveyManager::Util( "xy2ic",
		od_static_tr("uiSurveyManager_getUtils",
		"Convert (X,Y) to/from Inline/Crossline"), CallBack() );
	*newutils += new uiSurveyManager::Util( "spherewire",
				od_static_tr("uiSurveyManager_getUtils",
				"Setup geographical coordinates"), CallBack() );

	utils.setIfNull(newutils,true);
    }

    return *utils;
}



//--- uiNewSurveyByCopy


class uiNewSurveyByCopy : public uiDialog
{ mODTextTranslationClass(uiNewSurveyByCopy);

public:

uiNewSurveyByCopy( uiParent* p, const char* dataroot, const char* dirnm )
	: uiDialog(p,uiDialog::Setup(uiStrings::phrCopy(uiStrings::sSurvey()),
			mNoDlgTitle, mODHelpKey(mNewSurveyByCopyHelpID)))
	, dataroot_(dataroot)
	, survinfo_(0)
	, survdirsfld_(0)
{
    BufferStringSet survdirnms;
    uiSurvey::getDirectoryNames( survdirnms, false, dataroot );
    if ( survdirnms.isEmpty() )
	{ new uiLabel( this, tr("No surveys fond in this Data Root") ); return;}

    uiListBox::Setup su;
    su.lbl( tr("Survey to copy") );
    survdirsfld_ = new uiListBox( this, su );
    survdirsfld_->addItems( survdirnms );
    survdirsfld_->setHSzPol( uiObject::WideVar );
    survdirsfld_->setStretch( 2, 2 );
    survdirsfld_->setCurrentItem( dirnm );
    mAttachCB( survdirsfld_->selectionChanged, uiNewSurveyByCopy::survDirChgCB);

    newsurvnmfld_ = new uiGenInput( this, tr("New survey name") );
    newsurvnmfld_->attach( alignedBelow, survdirsfld_ );

    uiFileInput::Setup fisu( dataroot_ );
    fisu.defseldir( dataroot_ ).directories( true );
    targetpathfld_ = new uiFileInput( this, tr("Target location"), fisu );
    targetpathfld_->setSelectMode( uiFileDialog::DirectoryOnly );
    targetpathfld_->attach( alignedBelow, newsurvnmfld_ );
#ifdef __win__
    targetpathfld_->setSensitive( false );
#endif

    postFinalise().notify( mCB(this,uiNewSurveyByCopy,survDirChgCB) );
}

void survDirChgCB( CallBacker* )
{
    BufferString newsurvnm( "Copy of ", survdirsfld_->getText() );
    newsurvnmfld_->setText( newsurvnm );
}

bool acceptOK()
{
    if ( !anySurvey() )
	return true;

    const BufferString newsurvnm( newsurvnmfld_->text() );
    if ( newsurvnm.size() < 2 )
	return false;
    const BufferString survdirtocopy( survdirsfld_->getText() );
    if ( survdirtocopy.isEmpty() )
	return false;

    survinfo_ = uiSurvey::copySurvey( this, newsurvnm, dataroot_, survdirtocopy,
				     targetpathfld_->fileName() );
    return survinfo_;
}

bool anySurvey() const
{
    return survdirsfld_;
}

    const BufferString	dataroot_;
    uiListBox*		survdirsfld_;
    uiGenInput*		newsurvnmfld_;
    uiFileInput*	targetpathfld_;
    SurveyInfo*		survinfo_;

};



//--- uiSurveyManager


uiSurveyManager::uiSurveyManager( uiParent* p, bool standalone )
    : uiDialog(p,uiDialog::Setup(tr("Survey Setup and Selection"),
				 mNoDlgTitle,mODHelpKey(mSurveyHelpID)))
    , orgdataroot_(GetBaseDataDir())
    , dataroot_(GetBaseDataDir())
    , survinfo_(0)
    , survmap_(0)
    , survdirfld_(0)
    // TODO , standalone_(standalone)
    , standalone_(true)
    , freshsurveyselected_(false)
{
    if ( !DBM().isBad() )
    {
	survinfo_ = new SurveyInfo( SI() );
	initialsurveyname_.set( survinfo_->getDirName() );
    }

    uiGroup* topgrp = new uiGroup( this, "TopGroup" );

    datarootfld_ = new uiDataRootSel( topgrp, orgdataroot_ );
    mAttachCB( datarootfld_->selectionChanged, uiSurveyManager::dataRootChgCB );

    uiPushButton* settbut = new uiPushButton( topgrp, tr("General Settings"),
			    mCB(this,uiSurveyManager,odSettsButPushed), false );
    settbut->setIcon( "settings" );
    settbut->attach( rightTo, datarootfld_ );
    settbut->attach( rightBorder );

    uiSeparator* sep1 = new uiSeparator( topgrp, "Separator 1" );
    sep1->attach( stretchedBelow, datarootfld_ );

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
    setOkText( uiStrings::sSelect() );
    postFinalise().notify( mCB(this,uiSurveyManager,survDirChgCB) );
}


uiSurveyManager::~uiSurveyManager()
{
    delete survinfo_;
}


static void osrbuttonCB( void* )
{
    uiDesktopServices::openUrl( "https://opendtect.org/osr" );
}


void uiSurveyManager::fillLeftGroup( uiGroup* grp )
{
    survdirfld_ = new uiListBox( grp, "Surveys" );
    updateSurvList();
    survdirfld_->selectionChanged.notify(
				mCB(this,uiSurveyManager,survDirChgCB) );
    survdirfld_->doubleClicked.notify( mCB(this,uiSurveyManager,accept) );
    survdirfld_->setHSzPol( uiObject::WideVar );
    survdirfld_->setStretch( 2, 2 );

    uiButtonGroup* butgrp =
	new uiButtonGroup( grp, "Buttons", OD::Vertical );
    butgrp->attach( rightTo, survdirfld_ );
    new uiToolButton( butgrp, "addnew", uiStrings::phrCreate(mJoinUiStrs(sNew(),
			sSurvey())), mCB(this,uiSurveyManager,newButPushed) );
    editbut_ = new uiToolButton( butgrp, "edit", tr("Edit Survey Parameters"),
				 mCB(this,uiSurveyManager,editButPushed) );
    new uiToolButton( butgrp, "copyobj",
	tr("Copy Survey"), mCB(this,uiSurveyManager,copyButPushed) );
    new uiToolButton( butgrp, "compress",
	tr("Compress survey as zip archive"),
	mCB(this,uiSurveyManager,compressButPushed) );
    new uiToolButton( butgrp, "extract",
	tr("Extract survey from zip archive"),
	mCB(this,uiSurveyManager,extractButPushed) );
    new uiToolButton( butgrp, "share",
	tr("Share surveys through the OpendTect Seismic Repository"),
	mSCB(osrbuttonCB) );
    rmbut_ = new uiToolButton( butgrp, "delete", tr("Delete Survey"),
			       mCB(this,uiSurveyManager,rmButPushed) );
}


void uiSurveyManager::fillRightGroup( uiGroup* grp )
{
    survmap_ = new uiSurveyMap( grp );
    survmap_->attachGroup().setPrefWidth( sMapWidth );
    survmap_->attachGroup().setPrefHeight( sMapHeight );

    uiButton* lastbut = 0;
    ObjectSet<Util>& utils = getUtils();
    const CallBack cb( mCB(this,uiSurveyManager,utilButPushed) );
    for ( int idx=0; idx<utils.size(); idx++ )
    {
	const Util& util = *utils[idx];
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


void uiSurveyManager::add( const Util& util )
{
    getUtils() += new Util( util );
}


const char* uiSurveyManager::selectedSurveyName() const
{
    return survdirfld_->getText();
}


bool uiSurveyManager::rootDirWritable() const
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


bool uiSurveyManager::acceptOK()
{
    if ( !survdirfld_ )
	return true;
    else if ( !haveSurveys() || !survinfo_ )
	mErrRet(tr("Please create a survey (or press Cancel)"))

    const BufferString selsurv( selectedSurveyName() );
    const bool dbmisbad = DBM().isBad();
    const bool samedataroot = dataroot_ == orgdataroot_;
    const bool samesurvey = samedataroot && initialsurveyname_ == selsurv;

    if ( samedataroot && samesurvey )
	return writeSurvInfoFile( !parschanged_ && !dbmisbad );

    if ( !writeSettingsSurveyFile() )
	mErrRet(uiString::emptyString())

    if ( !writeSurvInfoFile(false) )
	return false;

    if ( samesurvey )
    {
	eSI() = *survinfo_;

	//TODO need another notification to signal current survey setup changed
	// DBM().surveyParsChanged();
    }
    else
    {
	uiRetVal uirv = uiDataRootSel::setSurveyDirTo(
						survinfo_->getFullDirPath() );
	if ( !uirv.isOK() )
	    { uiMSG().error( uirv ); return false; }
    }

    // TODO determine if we can help user if new survey is selected
    return true;
}


bool uiSurveyManager::rejectOK()
{
    return true;
}


bool uiSurveyManager::haveSurveys() const
{
    return survdirfld_ && !survdirfld_->isEmpty();
}


void uiSurveyManager::setCurrentSurvInfo( SurveyInfo* newsi, bool updscreen )
{
    delete survinfo_; survinfo_ = newsi;

    if ( updscreen )
	putToScreen();
    else if ( survmap_ )
	survmap_->setSurveyInfo( 0 );
}


void uiSurveyManager::newButPushed( CallBacker* )
{
    if ( !rootDirWritable() )
    {
	uiMSG().error( tr("Current data root\n%1\ndoes not allow writing")
			.arg(dataroot_) );
	return;
    }

    if ( standalone_ )
    {
	uiUserCreateSurvey dlg( this, dataroot_ );
	if ( dlg.go() )
	    setCurrentSurvInfo( dlg.getSurvInfo() );
    }
    else
    {
	//TODO pop up od_Edit_Survey
    }
}


void uiSurveyManager::rmButPushed( CallBacker* )
{
    const BufferString selnm( selectedSurveyName() );
    const BufferString seldirnm = FilePath(dataroot_).add(selnm).fullPath();
    const BufferString truedirnm = File::linkEnd( seldirnm );

    uiString msg = tr("This will delete the entire survey directory:\n\t%1"
		      "\nFull path: %2").arg(selnm).arg(truedirnm);
    if ( !uiMSG().askRemove(msg) )
	return;

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
    survDirChgCB( 0 );
}


void uiSurveyManager::editButPushed( CallBacker* )
{
    if ( !survinfo_ )
	return;
    if ( standalone_ )
    {
	const uiRetVal uirv = DBM().setDataSource(
					survinfo_->getFullDirPath() );
	if ( uirv.isError() )
	    { uiMSG().error( uirv ); return; }

	uiSurveyInfoEditor dlg( this );
	if ( dlg.go() )
	    putToScreen();
    }

    //TODO pop up od_Edit_Survey
}


void uiSurveyManager::copyButPushed( CallBacker* )
{
    if ( !survinfo_ || !rootDirWritable() )
	return;

    uiNewSurveyByCopy dlg( this, dataroot_, selectedSurveyName() );
    if ( !dlg.anySurvey() || !dlg.go() )
	return;

    setCurrentSurvInfo( dlg.survinfo_ );

    updateSurvList();
    survdirfld_->setCurrentItem( dlg.survinfo_->getDirName() );
}


void uiSurveyManager::extractButPushed( CallBacker* )
{
    if ( !rootDirWritable() )
	return;

    uiFileDialog fdlg( this, true, 0, "*.zip", tr("Select survey zip file") );
    fdlg.setSelectedFilter( sZipFileMask );
    if ( !fdlg.go() )
	return;

    uiSurvey::unzipFile( this, fdlg.fileName(), dataroot_ );
    updateSurvList();
    readSurvInfoFromFile();
    //TODO set unpacked survey as current with survdirfld_->setCurrentItem()
}


void uiSurveyManager::compressButPushed( CallBacker* )
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

    uiSurvey::zipDirectory( this, survnm, zippath.fullPath() );
}


void uiSurveyManager::odSettsButPushed( CallBacker* )
{
    uiSettingsDlg dlg( this );
    dlg.go();
}


void uiSurveyManager::utilButPushed( CallBacker* cb )
{
    if ( !survinfo_ )
	return;
    mDynamicCastGet(uiButton*,tb,cb)
    if ( !tb )
	{ pErrMsg("Huh"); return; }

    const int butidx = utilbuts_.indexOf( tb );
    if ( butidx < 0 )
	{ pErrMsg("Huh"); return; }

    if ( butidx == 0 )
    {
	uiConvertPos dlg( this, *survinfo_ );
	dlg.go();
    }
    else if ( butidx == 1 )
    {
	uiSingleGroupDlg<Coords::uiPositionSystemSel> dlg( this,
	    new Coords::uiPositionSystemSel( 0, true, survinfo_,
					     survinfo_->getCoordSystem() ));
	if ( dlg.go() )
	{
	    survinfo_->setCoordSystem( dlg.getDlgGroup()->outputSystem() );
	    if ( !survinfo_->write() )
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


void uiSurveyManager::updateSurvList()
{
    NotifyStopper ns( survdirfld_->selectionChanged );
    int newselidx = survdirfld_->currentItem();
    const BufferString prevsel( survdirfld_->getText() );
    survdirfld_->setEmpty();
    BufferStringSet dirlist; uiSurvey::getDirectoryNames( dirlist, false,
							  dataroot_ );
    survdirfld_->addItems( dirlist );

    if ( !haveSurveys() )
	return;

    const int idxofprevsel = survdirfld_->indexOf( prevsel );
    const int idxofcursi = survinfo_ ? survdirfld_->indexOf(
					  survinfo_->getDirName() ) : -1;
    if ( idxofcursi >= 0 )
	newselidx = idxofcursi;
    else if ( idxofprevsel >= 0 )
	newselidx = idxofprevsel;

    if ( newselidx < 0 )
	newselidx = 0;
    if ( newselidx >= survdirfld_->size() )
	newselidx = survdirfld_->size()-1 ;

    survdirfld_->setCurrentItem( newselidx );
}


bool uiSurveyManager::writeSettingsSurveyFile()
{
    if ( !haveSurveys() )
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


void uiSurveyManager::readSurvInfoFromFile()
{
    const BufferString survnm( selectedSurveyName() );
    SurveyInfo* newsi = 0;
    if ( !survnm.isEmpty() )
    {
	const BufferString fname = FilePath( dataroot_ )
			    .add( selectedSurveyName() ).fullPath();
	uiRetVal uirv;
	newsi = SurveyInfo::read( fname, uirv );
	if ( !newsi )
	    uiMSG().error( uirv );
    }

    if ( newsi )
	setCurrentSurvInfo( newsi );
}


void uiSurveyManager::dataRootChgCB( CallBacker* cb )
{
    dataroot_ = datarootfld_->getDir();
    updateSurvList();
    survDirChgCB( cb );
}


void uiSurveyManager::survDirChgCB( CallBacker* )
{
    if ( !haveSurveys() )
	setCurrentSurvInfo( 0, true );
    else
    {
	writeSurvInfoFile( true );
	readSurvInfoFromFile();
	putToScreen();
    }
}


void uiSurveyManager::putToScreen()
{
    if ( !survmap_ ) return;

    survmap_->setSurveyInfo( survinfo_ );
    const bool hassurveys = haveSurveys();
    rmbut_->setSensitive( hassurveys );
    editbut_->setSensitive( hassurveys );
    for ( int idx=0; idx<utilbuts_.size(); idx++ )
	utilbuts_[idx]->setSensitive( hassurveys );

    if ( !hassurveys )
    {
	notesfld_->setText( uiString::emptyString() );
	infofld_->setText( uiString::emptyString() );
	if ( !haveSurveys() )
	    button(CANCEL)->setText( uiStrings::sExit() );
	return;
    }

    if ( button(CANCEL) )
	button(CANCEL)->setText( uiStrings::sCancel() );
    BufferString locinfo( "Location: " );
    BufferString inlinfo( "In-line range: " );
    BufferString crlinfo( "Cross-line range: " );
    BufferString zinfo( "Z range" );
    BufferString bininfo( "Inl/Crl bin size" );
    BufferString areainfo( "Area" );
    BufferString survtypeinfo( "Survey type: " );
    BufferString orientinfo( "In-line Orientation: " );

    if ( survinfo_ )
    {
	const SurveyInfo& si = *survinfo_;
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


bool uiSurveyManager::writeSurvInfoFile( bool onlyforcomments )
{
    if ( !survinfo_ || (onlyforcomments && !notesfld_->isModified()) )
	return true;

    survinfo_->setComment( notesfld_->text() );
    if ( !survinfo_->write() )
	mErrRet(tr("Failed to write survey info.\nNo changes committed."))

    return true;
}

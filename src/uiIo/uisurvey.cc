/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisurvey.h"

#include "uicombobox.h"
#include "uiconvpos.h"
#include "uidesktopservices.h"
#include "uifileinput.h"
#include "uifont.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilatlong2coord.h"
#include "uilistbox.h"
#include "uimain.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uisetdatadir.h"
#include "ui2dsip.h"
#include "uisip.h"
#include "uisplitter.h"
#include "uisurveyselect.h"
#include "uisurvinfoed.h"
#include "uisurvmap.h"
#include "uisurveyzip.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "cubesampling.h"
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "latlong.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "odver.h"
#include "settings.h"
#include "strmprov.h"
#include "survinfo.h"

#include <iostream>
#include <math.h>

#define mMapWidth	300
#define mMapHeight	300
static const char* sZip = "*.zip";

static ObjectSet<uiSurvey::Util>& getUtils()
{
    static ObjectSet<uiSurvey::Util>* utils = 0;
    if ( !utils )
    {
	utils = new ObjectSet<uiSurvey::Util>;
	*utils += new uiSurvey::Util( "xy2ic", "Convert (X,Y) to/from I/C",
				      CallBack() );
	*utils += new uiSurvey::Util( "spherewire",
				"Setup geographical coordinates",
				      CallBack() );
    }
    return *utils;
}


void uiSurvey::add( const uiSurvey::Util& util )
{
    getUtils() += new uiSurvey::Util( util );
}


static BufferString getTrueDir( const char* dn )
{
    BufferString dirnm = dn;
    FilePath fp;
    while ( File::isLink(dirnm) )
    {
	BufferString newdirnm = File::linkTarget(dirnm);
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


static bool copySurv( const char* from, const char* todirnm, int mb )
{
    const BufferString todir( todirnm );
    if ( File::exists(todir) )
    {
	BufferString msg( "A survey '" );
	msg += todirnm;
	msg += "' already exists.\nYou will have to remove it first";
        uiMSG().error( msg );
        return false;
    }

    BufferString msg;
    if ( mb > 0 )
	{ msg += mb; msg += " MB of data"; }
    else
	msg += "An unknown amount";
    msg += " of data needs to be copied.\nDuring the copy, OpendTect will "
	    "freeze.\nDepending on the data transfer rate, this can take "
	    "a long time!\n\nDo you wish to continue?";
    if ( !uiMSG().askContinue( msg ) )
	return false;

    const BufferString fromdir = getTrueDir( FilePath(from).fullPath() );

    MouseCursorChanger cc( MouseCursor::Wait );
    if ( !File::copy( fromdir, todir ) )
    {
        uiMSG().error( "Cannot copy the survey data" );
        return false;
    }

    File::makeWritable( todir, true, true );
    return true;
}


uiSurvey::uiSurvey( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Survey selection",
	       "Select and setup survey","0.3.1").nrstatusflds(1))
    , initialdatadir_(GetBaseDataDir())
    , initialsurvey_(GetSurveyName())
    , survinfo_(const_cast<SurveyInfo*>(&SI()))
    , survmap_(0)
    , impiop_(0)
    , impsip_(0)
    , initialsurveyparchanged_(false)
{
    const int lbwidth = 250;
    const int noteshght = 5;
    const int totwdth = lbwidth + mMapWidth + 10;

    const char* ptr = GetBaseDataDir();
    if ( !ptr ) return;

    static int sipidx2d mUnusedVar =
		uiSurveyInfoEditor::addInfoProvider( new ui2DSurvInfoProvider );
    static int sipidxcp mUnusedVar =
		uiSurveyInfoEditor::addInfoProvider( new uiCopySurveySIP );

    uiGroup* topgrp = new uiGroup( this, "TopGroup" );
    uiGroup* rightgrp = new uiGroup( topgrp, "Survey selection right" );

    survmap_ = new uiSurveyMap( rightgrp );
    survmap_->setPrefWidth( mMapWidth );
    survmap_->setPrefHeight( mMapHeight );

    uiGroup* leftgrp = new uiGroup( topgrp, "Survey selection left" );
    listbox_ = new uiListBox( leftgrp, dirlist_, "Surveys" );
    updateSvyList();
    listbox_->setCurrentItem( GetSurveyName() );
    listbox_->selectionChanged.notify( mCB(this,uiSurvey,selChange) );
    listbox_->doubleClicked.notify( mCB(this,uiSurvey,accept) );
    listbox_->setPrefWidth( lbwidth );
    listbox_->setStretch( 2, 2 );
    leftgrp->attach( leftOf, rightgrp );

    newbut_ = new uiPushButton( leftgrp, "&New",
	    			mCB(this,uiSurvey,newButPushed), false );
    newbut_->attach( rightOf, listbox_ );
    newbut_->setPrefWidthInChar( 12 );
    rmbut_ = new uiPushButton( leftgrp, "&Remove",
	    		       mCB(this,uiSurvey,rmButPushed), false );
    rmbut_->attach( alignedBelow, newbut_ );
    rmbut_->setPrefWidthInChar( 12 );
    editbut_ = new uiPushButton( leftgrp, "&Edit",
	    			 mCB(this,uiSurvey,editButPushed), false );
    editbut_->attach( alignedBelow, rmbut_ );
    editbut_->setPrefWidthInChar( 12 );
    copybut_ = new uiPushButton( leftgrp, "C&opy",
	    			 mCB(this,uiSurvey,copyButPushed), false );
    copybut_->attach( alignedBelow, editbut_ );
    copybut_->setPrefWidthInChar( 12 );

    exportbut_ = new uiToolButton( leftgrp, "share",
				    "Pack survey into a zip file",
				    mCB(this,uiSurvey,exportButPushed) );
    exportbut_->attach( alignedBelow, copybut_ );
    importbut_ = new uiToolButton( leftgrp, "unpack",
				    "Unpack survey from zip file", 
				    mCB(this,uiSurvey,importButPushed) );
    importbut_->attach( rightAlignedBelow, copybut_ );

    ObjectSet<uiSurvey::Util>& utils = getUtils();
    uiGroup* utilbutgrp = new uiGroup( rightgrp, "Surv Util buttons" );
    const CallBack cb( mCB(this,uiSurvey,utilButPush) );
    for ( int idx=0; idx<utils.size(); idx++ )
    {
	const uiSurvey::Util& util = *utils[idx];
	uiToolButton* but = new uiToolButton( utilbutgrp, util.pixmap_,
						util.tooltip_, cb );
	but->setToolTip( util.tooltip_ );
	utilbuts_ += but;
	if ( idx > 0 )
	    utilbuts_[idx]->attach( rightOf, utilbuts_[idx-1] );
    }
    utilbutgrp->attach( centeredBelow, survmap_ );

    datarootbut_ = new uiPushButton( leftgrp, "&Set Data Root",
	    			mCB(this,uiSurvey,dataRootPushed), false );
    datarootbut_->attach( centeredBelow, listbox_ );

    uiSeparator* horsep1 = new uiSeparator( topgrp );
    horsep1->setPrefWidth( totwdth );
    horsep1->attach( stretchedBelow, rightgrp, -2 );
    horsep1->attach( ensureBelow, leftgrp );

    uiGroup* infoleft = new uiGroup( topgrp, "Survey info left" );
    uiGroup* inforight = new uiGroup( topgrp, "Survey info right" );
    infoleft->attach( alignedBelow, leftgrp );
    infoleft->attach( ensureBelow, horsep1 );
    inforight->attach( alignedBelow, rightgrp );
    inforight->attach( ensureBelow, horsep1 );

    infoleft->setFont( FontList().get(FontData::key(FontData::ControlSmall)) );
    inforight->setFont( FontList().get(FontData::key(FontData::ControlSmall)));

    inllbl_ = new uiLabel( infoleft, "" ); 
    crllbl_ = new uiLabel( infoleft, "" );
    arealbl_ = new uiLabel( infoleft, "" );
    zlbl_ = new uiLabel( inforight, "" ); 
    binlbl_ = new uiLabel( inforight, "" );
    typelbl_ = new uiLabel( inforight, "" );
#if 0
    inllbl_->setHSzPol( uiObject::widevar );
    crllbl_->setHSzPol( uiObject::widevar );
    zlbl_->setHSzPol( uiObject::widevar );
    binlbl_->setHSzPol( uiObject::widevar );
    arealbl_->setHSzPol( uiObject::widevar );
#else
    inllbl_->setPrefWidthInChar( 40 );
    crllbl_->setPrefWidthInChar( 40 );
    zlbl_->setPrefWidthInChar( 40 );
    binlbl_->setPrefWidthInChar( 40 );
    arealbl_->setPrefWidthInChar( 40 );
    typelbl_->setPrefWidthInChar( 40 );
#endif

    crllbl_->attach( alignedBelow, inllbl_ );
    arealbl_->attach( alignedBelow, crllbl_ );
    binlbl_->attach( alignedBelow, zlbl_ );
    typelbl_->attach( alignedBelow, binlbl_ );

    uiGroup* botgrp = new uiGroup( this, "Bottom Group" );
    uiLabel* notelbl = new uiLabel( botgrp, "Notes:" );
    notes_ = new uiTextEdit( botgrp, "Notes" );
    notes_->attach( alignedBelow, notelbl );
    notes_->setPrefHeightInChar( noteshght );
    notes_->setPrefWidth( totwdth );
    notes_->setStretch( 2, 2 );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

    mkInfo();
    survmap_->drawMap( survinfo_ );
    setOkText( "&Ok (Select)" );
}


uiSurvey::~uiSurvey()
{
    delete impiop_;
    delSurvInfo();
}


#define mCheckRootDirIsWritable(rootdir) \
{ \
    if ( !File::isWritable(rootdir) ) \
    { \
	BufferString msg( "Cannot create new survey in\n",rootdir, \
			  ".\nDirectory is write protected."); \
	uiMSG().error( msg ); \
	return; \
    } \
}


#define mErrRet(s) { uiMSG().error(s); return; }
#define mErrRetYN(s) { uiMSG().error(s); return false; }


void uiSurvey::getSurveyList( BufferStringSet& list, const char* dataroot )
{
    BufferString basedir = dataroot;
    if ( basedir.isEmpty() )
	basedir = GetBaseDataDir();
    DirList dl( basedir, DirList::DirsOnly );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const char* dirnm = dl.get( idx );
	const FilePath fp( basedir, dirnm, SurveyInfo::sKeySetupFileName() );
	if ( File::exists(fp.fullPath()) )
	    list.add( dirnm );
    }

    list.sort();
}


bool uiSurvey::survTypeOKForUser( bool is2d )
{
    const bool dowarn = (is2d && !SI().has2D()) || (!is2d && !SI().has3D());
    if ( !dowarn ) return true;

    BufferString warnmsg( "Your survey is set up as '" );
    warnmsg += is2d ? "3-D only'.\nTo be able to actually use 2-D"
		    : "2-D only'.\nTo be able to actually use 3-D";
    warnmsg += " data\nyou will have to change the survey setup.";
    warnmsg += "\n\nDo you wish to continue?";

    return uiMSG().askContinue( warnmsg );
}

#define mPrevSurv(survnm) \
{ \
    IOMan::enableSurveyChangeTriggers( false ); \
    listbox_->setCurrentItem( survnm  ); \
    updateSvyFile(); \
    return false; \
}

bool uiSurvey::acceptOK( CallBacker* )
{
    if ( listbox_->isEmpty() )
    {
	uiMSG().error( "Please create a survey (or press Cancel)" );
	return false;
    }
    writeComments();

    const bool samesurvey = initialsurvey_ == listbox_->getText();
    if ( !samesurvey || initialdatadir_ != GetBaseDataDir() ||
	 (samesurvey && initialsurveyparchanged_) )
	IOMan::enableSurveyChangeTriggers( true );

    const BufferString prevsurvey = GetSurveyName();
    if ( !updateSvyFile() )
	mPrevSurv( prevsurvey )

    SurveyInfo::deleteInstance();
    SurveyInfo::pushSI( survinfo_ );
    if ( !IOMan::newSurvey() )
    {
	if ( !IOM().message().isEmpty() )
	    uiMSG().error( IOM().message() );

	mPrevSurv( prevsurvey )
    }

    IOMan::enableSurveyChangeTriggers( false );
    Settings::common().set( "Default DATA directory", GetDataDir() );
    if ( !Settings::common().write() )
	uiMSG().warning( "Could not save the survey location in the settings"
	       		 " file" );

    if ( impiop_ && impsip_ )
    {
	const char* askq = impsip_->importAskQuestion();
	if ( askq && *askq && uiMSG().askGoOn(askq) )
	{
	    IOM().to( "100010" );
	    impsip_->startImport( parent(), *impiop_ );
	}
    }

    return true;
}


bool uiSurvey::rejectOK( CallBacker* )
{
    if ( initialdatadir_ != GetBaseDataDir() )
    {
	if ( !uiSetDataDir::setRootDataDir(this,initialdatadir_) )
	    mErrRetYN( "As we cannot reset to the old Data Root,\n"
		       "You *have to* select a survey now!" )
    }

    return true;
}


#define mRmTmpSurvey(dirnm,msg) \
{ \
    if ( File::exists(dirnm) ) \
    	File::remove( dirnm ); \
\
    delSurvInfo(); \
    mErrRet( msg )\
}


void uiSurvey::newButPushed( CallBacker* )
{
    const FileNameString rootdir = GetBaseDataDir();
    mCheckRootDirIsWritable(rootdir);

    FilePath fp( GetSoftwareDir(0), "data", SurveyInfo::sKeyBasicSurveyName() );
    delSurvInfo();
    survinfo_ = SurveyInfo::read( fp.fullPath() );
    if ( !survinfo_ )
    {
	BufferString errmsg( "Cannot read software default survey\n" );
	errmsg.add( "Try to reinstall the OpendTect package" );
	mErrRet( errmsg )
    }

    uiSurveyNewDlg dlg( this, *survinfo_ );
    if ( !dlg.go() )
	return;

    const BufferString orgstorepath = rootdir;
    const BufferString orgdirname = survinfo_->getDirName().buf();
    const BufferString storagedir = FilePath( orgstorepath ).add( orgdirname )
						       	    .fullPath();
    if ( !uiSurveyInfoEditor::copySurv(
		mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()),0,
				  orgstorepath,orgdirname) )
	mErrRet( "Cannot initiate new survey" )

    survinfo_->datadir_ = rootdir;
    if ( !File::makeWritable(storagedir,true,true) )
    {
	BufferString msg( "Cannot set the permissions for the new survey" );
	mRmTmpSurvey( storagedir, msg )
    }

    if ( !survinfo_->write(rootdir) )
    {
	BufferString msg( "Failed to write survey info.\n" );
	msg.add( "No changes committed." );
	mRmTmpSurvey( storagedir, msg )
    }

    if ( !survInfoDialog(true) )
	updateInfo(0);

    rmbut_->setSensitive(true);
    editbut_->setSensitive(true);
    for ( int idx=0; idx<utilbuts_.size(); idx++ )
	utilbuts_[idx]->setSensitive(true);
}


void uiSurvey::rmButPushed( CallBacker* )
{
    BufferString selnm( listbox_->getText() );
    const BufferString seldirnm = FilePath(GetBaseDataDir())
					    .add(selnm).fullPath();
    const BufferString truedirnm = getTrueDir( seldirnm );

    BufferString msg( "This will remove the entire survey directory:\n\t" );
    msg += selnm;
    msg += "\nFull path: "; msg += truedirnm;
    if ( !uiMSG().askRemove( msg ) ) return;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    const bool rmisok = File::remove( truedirnm );
    MouseCursorManager::restoreOverride();
    if ( !rmisok )
	mErrRet( BufferString( truedirnm, "\nnot removed properly" ) )

    if ( seldirnm != truedirnm ) // must have been a link
	if ( !File::remove(seldirnm) )
	    uiMSG().error( "Could not remove link to the removed survey" );

    updateSvyList();
    const char* ptr = GetSurveyName();
    if ( ptr && selnm == ptr )
	if ( button(CANCEL) ) button(CANCEL)->setSensitive( false );
}


void uiSurvey::editButPushed( CallBacker* )
{
    if ( !survInfoDialog(false) )
	updateInfo(0);
}


void uiSurvey::copyButPushed( CallBacker* )
{
    const FileNameString rootdir = GetBaseDataDir();
    mCheckRootDirIsWritable(rootdir);

    uiSurveyGetCopyDir dlg( this, listbox_->getText() );
    if ( !dlg.go() )
	return;

    if ( !copySurv(dlg.fname_,dlg.newdirnm_,-1) )
	return;

    delSurvInfo();
    survinfo_ = SurveyInfo::read( dlg.fname_ );
    if ( !survinfo_ )
	mErrRet( "Could not read the copied survey" )

    BufferString newnm( FilePath(dlg.newdirnm_).fileName() );
    survinfo_->setName( newnm );
    survinfo_->updateDirName();
    if ( !survinfo_->write() )
	mErrRet( "Failed to write survey info." )

    updateSvyList();
    listbox_->setCurrentItem( dlg.newdirnm_ );
}


void osrbuttonCB( void* )
{
     uiDesktopServices::openUrl( "https://opendtect.org/osr" );
}

void uiSurvey::importButPushed( CallBacker* )
{
    uiFileDialog fdlg( this, true, 0, "*.zip", "Select survey zip file" );
    fdlg.setSelectedFilter( sZip );
    if ( !fdlg.go() )
	return;

    uiSurvey_UnzipFile( this, fdlg.fileName(), GetBaseDataDir() );
    updateSvyList();
    //TODO set unpacked survey as current witn listbox_->setCurrentItem()
}


void uiSurvey::exportButPushed( CallBacker* )
{
    const BufferString survnm( listbox_->getText() );
    const BufferString title( "Pack ", survnm, " survey into zip file" );
    uiDialog dlg( this,
    uiDialog::Setup(title,mNoDlgTitle,"0.3.12"));
    uiFileInput* filepinput = new uiFileInput( &dlg,"Select output destination",
		    uiFileInput::Setup().directories(false).forread(false)
		    .allowallextensions(false));
    filepinput->setFilter( sZip );
    uiLabel* sharfld = new uiLabel( &dlg, 
			   "You can share surveys to Open Seismic Repository."
			   "To know more " );
    sharfld->attach( leftAlignedBelow,  filepinput );
    uiPushButton* osrbutton = new uiPushButton( &dlg, 
				    "Click here", mSCB(osrbuttonCB), false );
    osrbutton->attach( rightOf, sharfld );
    if ( !dlg.go() )
	return;
	
    FilePath exportzippath( filepinput->fileName() );
    BufferString zipext = exportzippath.extension();
    if ( zipext != "zip" )
	mErrRet( "Please add .zip extension to the file name" )

    uiSurvey_ZipDirectory( this, survnm, exportzippath.fullPath() );
}


void uiSurvey::dataRootPushed( CallBacker* )
{
    uiSetDataDir dlg( this );
    if ( !dlg.go() )
	return;

    updateSvyList();
    const char* ptr = GetSurveyName();
    if ( ptr && listbox_->isPresent(ptr) )
	listbox_->setCurrentItem( GetSurveyName() );
}


void uiSurvey::utilButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    if ( !tb ) { pErrMsg("Huh"); return; }

    const int butidx = utilbuts_.indexOf( tb );
    if ( butidx < 0 ) { pErrMsg("Huh"); return; }
    
    if ( butidx == 0 )
    {
	uiConvertPos dlg( this, *survinfo_ );
	dlg.go();
    }
    else if ( butidx == 1 )
    {
	if ( !survinfo_ ) return;
	uiLatLong2CoordDlg dlg( this, survinfo_->latlong2Coord(), survinfo_ );
	if ( dlg.go() && !survinfo_->write() )
	    mErrRet( "Could not write the setup" )
    }
    else
    {
	Util* util = getUtils()[butidx];
	util->cb_.doCall( this );
    }
}


void uiSurvey::updateSvyList()
{
    mkDirList();
    if ( dirlist_.isEmpty() )
	updateInfo(0);

    NotifyStopper ns( listbox_->selectionChanged );
    listbox_->setEmpty();
    listbox_->addItems( dirlist_ );
}


bool uiSurvey::updateSvyFile()
{
    if ( listbox_->isEmpty() )
    {
	pErrMsg( "Should save survey name but no survey in the list" );
	return false;
    }

    BufferString seltxt( listbox_->getText() );
    if ( seltxt.isEmpty() )
	mErrRetYN( "Survey folder name cannot be empty" )

    if ( !File::exists(FilePath(GetBaseDataDir()).add(seltxt).fullPath()) )
	mErrRetYN( "Survey directory does not exist anymore" )

    if ( !writeSurveyName(seltxt) )
	mErrRetYN( "Cannot update the 'survey' file in $HOME/.od/" )

    SetSurveyName( seltxt );
    return true;
}


bool uiSurvey::getSurvInfoNew()
{
    BufferString fname = FilePath( GetBaseDataDir() )
	    		.add( listbox_->getText() ).fullPath();
    delSurvInfo();
    survinfo_ = SurveyInfo::read( fname );
    return (bool)survinfo_;
}


void uiSurvey::delSurvInfo()
{
    if ( survinfo_ && survinfo_ != &SI() )
	delete survinfo_;
}


void uiSurvey::updateInfo( CallBacker* cb )
{
    mDynamicCastGet(uiSurveyInfoEditor*,dlg,cb);
    if ( !dlg )
	if ( !getSurvInfoNew() )
	    return;

    mkInfo();
    survmap_->drawMap( survinfo_ );
}


bool uiSurvey::survInfoDialog( bool isnew )
{
    delete impiop_; impsip_ = 0;
    uiSurveyInfoEditor dlg( this, *survinfo_, isnew );
    if ( !dlg.isOK() )
	return false;

    dlg.survParChanged.notify( mCB(this,uiSurvey,updateInfo) );
    if ( !dlg.go() )
	return false;

    if ( initialsurvey_==listbox_->getText() )
	initialsurveyparchanged_ = true;

    updateSvyList();
    listbox_->setCurrentItem( dlg.dirName() );

    impiop_ = dlg.impiop_; dlg.impiop_ = 0;
    impsip_ = dlg.lastsip_;
    return true;
}


void uiSurvey::selChange( CallBacker* )
{
    writeComments();
    updateInfo(0);
}


void uiSurvey::mkDirList()
{
    dirlist_.erase();
    getSurveyList( dirlist_ );
}


void uiSurvey::mkInfo()
{
    if ( !survinfo_ )
	return;

    const SurveyInfo& si = *survinfo_;
    BufferString inlinfo( "In-line range: " );
    BufferString crlinfo( "Cross-line range: " );
    BufferString zinfo( "Z range " );
    zinfo += si.getZUnitString(); zinfo += ": ";
    BufferString bininfo( "Bin size (", si.getXYUnitString(false), "/line): ");
    BufferString areainfo( "Area (sq ", si.xyInFeet() ? "mi" : "km", "): " );

    if ( si.sampling(false).hrg.totalNr() )
    {
	inlinfo += si.sampling(false).hrg.start.inl;
	inlinfo += " - "; inlinfo += si.sampling(false).hrg.stop.inl;
	inlinfo += "  step: "; inlinfo += si.inlStep();
	
	crlinfo += si.sampling(false).hrg.start.crl;
	crlinfo += " - "; crlinfo += si.sampling(false).hrg.stop.crl;
	crlinfo += "  step: "; crlinfo += si.crlStep();

	float inldist = si.inlDistance();
	float crldist = si.crlDistance();

	#define mkString(dist) \
	nr = (int)(dist*100+.5); bininfo += nr/100; \
	rest = nr%100; bininfo += rest < 10 ? ".0" : "."; bininfo += rest; \

	int nr, rest;    
	bininfo += "inl: "; mkString(inldist);
	bininfo += "  crl: "; mkString(crldist);
	float area = (float) ( si.computeArea(false) * 1e-6 ); //in km2
	if ( si.xyInFeet() )
	    area /= 2.590; // square miles

	areainfo += area;
    }

    #define mkZString(nr) \
    zinfo += istime ? mNINT32(1000*nr) : nr;

    const bool istime = si.zIsTime(); 
    mkZString( si.zRange(false).start );
    zinfo += " - "; mkZString( si.zRange(false).stop );
    zinfo += "  step: "; mkZString( si.zRange(false).step );

    inllbl_->setText( inlinfo );
    crllbl_->setText( crlinfo );
    binlbl_->setText( bininfo );
    arealbl_->setText( areainfo );
    zlbl_->setText( zinfo );
    typelbl_->setText( BufferString("Survey type: ",
			SurveyInfo::toString(si.survDataType())) );
    notes_->setText( si.comment() );

    bool anysvy = dirlist_.size();
    rmbut_->setSensitive( anysvy );
    editbut_->setSensitive( anysvy );
    for ( int idx=0; idx<utilbuts_.size(); idx++ )
	utilbuts_[idx]->setSensitive( anysvy );

    FilePath fp( si.datadir_, si.dirname_ );
    fp.makeCanonical();
    toStatusBar( fp.fullPath() );
}


void uiSurvey::writeComments()
{
    if ( !notes_->isModified() )
	return;

    if ( !survinfo_ )
	return;

    survinfo_->setComment( notes_->text() );
    if ( !survinfo_->write( GetBaseDataDir() ) )
	mErrRet( "Failed to write survey info.\nNo changes committed." )
}


bool uiSurvey::writeSurveyName( const char* nm )
{
    const char* ptr = GetSurveyFileName();
    if ( !ptr )
	mErrRetYN( "Error in survey system. Please check $HOME/.od/" )

    StreamData sd = StreamProvider( ptr ).makeOStream();
    if ( !sd.usable() )
	mErrRetYN( BufferString( "Cannot write to ", ptr ) )

    *sd.ostrm << nm;

    sd.close();
    return true;
}


void uiSurvey::getSurvInfo()
{
    BufferString fname = FilePath( GetBaseDataDir() )
	    		.add( listbox_->getText() ).fullPath();
    delete survinfo_;
    survinfo_ = SurveyInfo::read( fname );
}


bool uiSurvey::survInfoDialog()
{
    delete impiop_; impsip_ = 0;
    uiSurveyInfoEditor dlg( this, *survinfo_, false );
    if ( !dlg.isOK() )
	return false;

    dlg.survParChanged.notify( mCB(this,uiSurvey,updateInfo) );
    if ( !dlg.go() )
	return false;

    if ( initialsurvey_==listbox_->getText() )
	initialsurveyparchanged_ = true;

    updateSvyList();
    listbox_->setCurrentItem( dlg.dirName() );

    impiop_ = dlg.impiop_; dlg.impiop_ = 0;
    impsip_ = dlg.lastsip_;
    return true;
}


void uiSurvey::newSurvey()
{
    SI().setInvalid();
}



uiSurveyNewDlg::uiSurveyNewDlg( uiParent* p, SurveyInfo& survinfo )
    	: uiDialog(p,uiDialog::Setup("Create new survey",
		    "Specify new survey parameters",mTODOHelpID))
	, survinfo_(survinfo)
	, sips_(uiSurveyInfoEditor::survInfoProvs())
{
    survnmfld_ = new uiGenInput( this, "Give a name" );
    survnmfld_->setElemSzPol( uiObject::Wide );

    uiGroup* pol2dgrp = new uiGroup( this, "Data type group" );
    uiLabel* pol2dlbl = new uiLabel( pol2dgrp, "Available data" );
    pol2dfld_ = new uiCheckList( pol2dgrp, "3D", "2D", uiCheckList::AtLeastOne);
    pol2dfld_->attach( rightOf, pol2dlbl );
    pol2dfld_->setChecked( 0, true );
    pol2dfld_->setChecked( 1, false );
    pol2dfld_->changed.notify( mCB(this,uiSurveyNewDlg,pol2dChg) );
    pol2dgrp->attach( leftAlignedBelow, survnmfld_ );

    uiLabeledComboBox* siplcb = new uiLabeledComboBox( this,
	    					     "Get from" );
    siplcb->attach( leftAlignedBelow, pol2dgrp );
    sipfld_ = siplcb->box();

    uiGroup* zdomaingrp = new uiGroup( this, "Domain group" );
    uiLabel* domainlbl = new uiLabel( zdomaingrp, "Domain" );
    zdomainfld_ = new uiCheckList( zdomaingrp, "Time", "Depth",
	    			   uiCheckList::OneOnly );
    zdomainfld_->attach( rightOf, domainlbl );
    zdomainfld_->changed.notify( mCB(this,uiSurveyNewDlg,zdomainChg) );
    zdomaingrp->attach( leftAlignedBelow, siplcb );

    zunitgrp_ = new uiGroup( this, "Z unit group" );
    uiLabel* zunitlbl = new uiLabel( zunitgrp_, "Depth unit" );
    zunitfld_ = new uiCheckList( zunitgrp_, "Meter", "Feet",
	    			 uiCheckList::OneOnly );
    zunitfld_->attach( rightOf, zunitlbl );
    zunitgrp_->attach( leftAlignedBelow, zdomaingrp );
    zunitgrp_->display( false );

    setSip( false );
}


bool uiSurveyNewDlg::isOK()
{
    if ( !survnmfld_ || !pol2dfld_ || !sipfld_ || !zdomainfld_ )
    {
	pErrMsg( "New survey dialog is missing a field" );
	return false;
    }

    BufferString survnm = survName();
    if ( survnm.isEmpty() )
    	mErrRetYN( "Please enter a new survey name" )

    char* str = survnm.buf();
    cleanupString( str, false, false, true );
    const BufferString storagedir = FilePath( GetBaseDataDir() ).add( str )
								.fullPath();
    if ( File::exists(storagedir) )
    {
	BufferString errmsg( "A survey called ", survnm, " already exists\n" );
	errmsg.add( "Please remove it first or use another survey name" );
	mErrRetYN( errmsg )
    }

    const int sipidx = sipfld_->currentItem();
    if ( !sips_.validIdx(sipidx) )
    {
	pErrMsg( "Cannot use this geometry provider method" );
	return false;
    }

    uiSurvInfoProvider* sip = sips_[sipidx];
    if ( !sip )
	mErrRetYN( "Cannot use this geometry provider method" )

    return true;
}


bool uiSurveyNewDlg::acceptOK( CallBacker* cb )
{
    if ( !isOK() )
	return false;

    const BufferString survnm = survName();
    survinfo_.setName( survnm );
    survinfo_.updateDirName();
    survinfo_.setSurvDataType( pol2D() );
    survinfo_.setZUnit( isTime(), isInFeet() );

    const int sipidx = sipfld_->currentItem();
    if ( sipidx < sipfld_->size() - 1 )
	survinfo_.setSipName( sipName() );

    return true;
}


BufferString uiSurveyNewDlg::sipName() const
{
    const int sipidx = sipfld_->currentItem();
    return sipidx == sipfld_->size()-1 ? "" : sipfld_->textOfItem( sipidx );
}


void uiSurveyNewDlg::setSip( bool for2donly )
{
    sipfld_->setEmpty();

    const int nrprovs = sips_.size();
    int preferedsel = nrprovs ? -1 : 0;
    for ( int idx=0; idx<nrprovs; idx++ )
    {
	if ( !sips_.validIdx(idx) )
	    continue;

	mDynamicCastGet(const ui2DSurvInfoProvider*,sip,sips_[idx]);
	if ( sip && for2donly && preferedsel == -1 )
	    preferedsel = idx;

	BufferString txt( sips_[idx]->usrText() );
	txt += " ...";
	sipfld_->addItem( txt );
    }
    sipfld_->addItem( "Manual selection" ); // always last
    sipfld_->setCurrentItem( preferedsel );

    int maxlen = 0;
    for ( int idx=0; idx<sipfld_->size(); idx++ )
    {
	const int len = strlen( sipfld_->textOfItem(idx) );
	if ( len > maxlen ) maxlen = len;
    }
    sipfld_->setPrefWidthInChar( maxlen + 5 );
}


SurveyInfo::Pol2D uiSurveyNewDlg::pol2D() const
{
    return has3D() ? ( has2D() ? SurveyInfo::Both2DAnd3D
	    		       : SurveyInfo::No2D )
		   : SurveyInfo::Only2D;
}


void uiSurveyNewDlg::pol2dChg( CallBacker* cb )
{
    setSip( has2D() && !has3D() );
}


void uiSurveyNewDlg::zdomainChg( CallBacker* cb )
{
    zunitgrp_->display( !isTime() );
}



uiSurveyGetCopyDir::uiSurveyGetCopyDir( uiParent* p, const char* cursurv )
	: uiDialog(p,uiDialog::Setup("Import survey from location",
		   "Copy surveys from any data root",mTODOHelpID))
{
    BufferString curfnm;
    if ( cursurv && *cursurv )
	curfnm = FilePath( GetBaseDataDir(), cursurv ).fullPath();
    else
	curfnm = GetBaseDataDir();

    inpsurveyfld_ = new uiSurveySelect( this,"Survey to copy" );
    inpsurveyfld_->setSurveyPath( curfnm );
    newsurveyfld_ = new uiSurveySelect( this, "New Survey" );
    newsurveyfld_->attach( alignedBelow,  inpsurveyfld_ );
}


void uiSurveyGetCopyDir::inpSel( CallBacker* )
{
    BufferString fullpath;
    inpsurveyfld_->getFullSurveyPath( fullpath );
    FilePath fp( fullpath );
    newsurveyfld_->setInputText( fp.fullPath() );
}


bool uiSurveyGetCopyDir::acceptOK( CallBacker* )
{
    if ( !inpsurveyfld_->getFullSurveyPath( fname_ ) ||
	 !newsurveyfld_->getFullSurveyPath( newdirnm_) )
	mErrRetYN( "No Valid or Empty Input" )

    return true;
}


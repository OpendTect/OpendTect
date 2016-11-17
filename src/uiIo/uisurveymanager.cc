/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra / Bert
 Date:          June 2001 / Oct 2016
________________________________________________________________________

-*/

#include "uisurveymanager.h"

#include "uisurveyselect.h"
#include "uisurvinfoed.h"
#include "uiusercreatesurvey.h"
#include "uidatarootsel.h"
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
#include "oscommand.h"
#include "oddirs.h"
#include "odver.h"
#include "od_ostream.h"
#include "od_helpids.h"
#include "settings.h"
#include "survinfo.h"
#include "filemonitor.h"


static const char*	sZipFileMask = "ZIP files (*.zip *.ZIP)";
static const int cMapWidth = 300;
static const int cMapHeight = 300;


class uiNewSurveyByCopy : public uiDialog
{ mODTextTranslationClass(uiNewSurveyByCopy);

public:

uiNewSurveyByCopy( uiParent* p, const char* dataroot, const char* dirnm )
	: uiDialog(p,uiDialog::Setup(uiStrings::phrCopy(uiStrings::sSurvey()),
			mNoDlgTitle, mODHelpKey(mNewSurveyByCopyHelpID)))
	, dataroot_(dataroot)
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
    survdirname_.setEmpty();
    if ( !anySurvey() )
	return true;

    const BufferString newsurvnm( newsurvnmfld_->text() );
    if ( newsurvnm.size() < 2 )
	return false;
    const BufferString survdirtocopy( survdirsfld_->getText() );
    if ( survdirtocopy.isEmpty() )
	return false;

    SurveyInfo* newsi = uiSurvey::copySurvey( this, newsurvnm, dataroot_,
				survdirtocopy, targetpathfld_->fileName() );
    if ( newsi )
    {
	survdirname_.set( newsi->getDirName() );
	delete newsi;
	return true;
    }
    return false;
}

bool anySurvey() const
{
    return survdirsfld_;
}

    const BufferString	dataroot_;
    uiListBox*		survdirsfld_;
    uiGenInput*		newsurvnmfld_;
    uiFileInput*	targetpathfld_;
    BufferString	survdirname_;

};


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

	utils.setIfNull(newutils,true);
    }

    return *utils;
}


uiSurveyManager::uiSurveyManager( uiParent* p, bool standalone )
    : uiSurveySelect(p,false)
    , survinfo_(0)
    , survmap_(0)
    , rootdirnotwritablestr_(tr("Current data root\n%1\nis not writable"))
{
    if ( !standalone )
	survinfo_ = new SurveyInfo( SI() );

    uiPushButton* settbut = new uiPushButton( this, tr("General Settings"),
			    mCB(this,uiSurveyManager,odSettsButPushed), false );
    settbut->setIcon( "settings" );
    settbut->attach( rightTo, datarootfld_ );
    settbut->attach( rightBorder );

    uiGroup* topgrp = new uiGroup( this, "Top group" );
    uiGroup* leftgrp = new uiGroup( topgrp, "Left group" );
    uiGroup* survmapgrp = new uiGroup( topgrp, "Surv map group" );
    mkSurvManTools( leftgrp );
    mkSurvMapWithUtils( survmapgrp );
    mkInfoTabs();
    survmapgrp->setFrame( true );

    uiSplitter* vsplit = new uiSplitter( this, "V Split" );
    vsplit->addGroup( survmapgrp );
    vsplit->addGroup( leftgrp );

    uiSplitter* hsplit = new uiSplitter( this, "V Split" );
    hsplit->addGroup( topgrp );
    hsplit->addGroup( infotabs_ );

    putToScreen();

    mAttachCB( survDirChg, uiSurveyManager::survDirChgCB );
    mAttachCB( survParsChg, uiSurveyManager::survParsChgCB );
    mAttachCB( postFinalise(), uiSurveyManager::survDirChgCB );
}


uiSurveyManager::~uiSurveyManager()
{
    delete survinfo_;
}


const SurveyInfo* uiSurveyManager::curSI() const
{
    return survinfo_ ? survinfo_ : &SI();
}


static void osrbuttonCB( CallBacker* )
{
    uiDesktopServices::openUrl( "https://opendtect.org/osr" );
}


void uiSurveyManager::mkSurvManTools( uiGroup* grp )
{
    survmanbuts_ = new uiButtonGroup( grp, "Surv Man Buttons", OD::Vertical );
    survmanbuts_->attach( rightTo, survdirfld_ );
    new uiToolButton( survmanbuts_, "addnew",
			uiStrings::phrCreate(mJoinUiStrs(sNew(),
			sSurvey())), mCB(this,uiSurveyManager,newButPushed) );
    editbut_ = new uiToolButton( survmanbuts_, "edit",
				 tr("Edit Survey Parameters"),
				 mCB(this,uiSurveyManager,editButPushed) );
    new uiToolButton( survmanbuts_, "copyobj",
	tr("Copy Survey"), mCB(this,uiSurveyManager,copyButPushed) );
    new uiToolButton( survmanbuts_, "compress",
	tr("Compress survey as zip archive"),
	mCB(this,uiSurveyManager,compressButPushed) );
    new uiToolButton( survmanbuts_, "extract",
	tr("Extract survey from zip archive"),
	mCB(this,uiSurveyManager,extractButPushed) );
    new uiToolButton( survmanbuts_, "share",
	tr("Share surveys through the OpendTect Seismic Repository"),
	mSCB(osrbuttonCB) );
    rmbut_ = new uiToolButton( survmanbuts_, "delete", tr("Delete Survey"),
			       mCB(this,uiSurveyManager,rmButPushed) );
}


void uiSurveyManager::mkSurvMapWithUtils( uiGroup* grp )
{
    survmap_ = new uiSurveyMap( grp );
    survmap_->attachGroup().setPrefWidth( cMapWidth );
    survmap_->attachGroup().setPrefHeight( cMapHeight );

    inlgridview_ = new uiGrid2DMapObject();
    inlgridview_->setLineStyle( OD::LineStyle(OD::LineStyle::Dot) );
    inlgrid_ = new Grid2D();
    inlgridview_->setGrid( inlgrid_ );
    survmap_->addObject( inlgridview_ );

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
    survmap_->attachGroup().attach( rightOf, editbut_->parent() );
}


void uiSurveyManager::mkInfoTabs()
{
    infotabs_ = new uiTabStack( this, "Survey Info" );
    uiGroup* infogrp = new uiGroup( infotabs_->tabGroup(), "Info" );
    infofld_ = new uiTextEdit( infogrp, "Info", true );
    infofld_->setPrefHeightInChar( 7 );
    infofld_->setStretch( 2, 1 );
    infotabs_->addTab( infogrp );
    infotabs_->setTabIcon( infogrp, "info" );

    uiGroup* commentgrp = new uiGroup( infotabs_->tabGroup(), "Comment" );
    notesfld_ = new uiTextEdit( commentgrp, "Survey Notes" );
    notesfld_->setPrefHeightInChar( 5 );
    notesfld_->setStretch( 2, 2 );
    infotabs_->addTab( commentgrp );
    infotabs_->setTabIcon( commentgrp, "notes" );
}


void uiSurveyManager::add( const Util& util )
{
    getUtils() += util.clone();
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


bool uiSurveyManager::haveSurveys() const
{
    return !survdirfld_->isEmpty();
}


void uiSurveyManager::setCurrentSurvey( const char* survdirnm )
{
    reReadSurvInfoFromFile( survdirnm );
    putToScreen();
}


void uiSurveyManager::reReadSurvInfoFromFile( const char* survdirnm )
{
    survreadstatus_.setEmpty();
    if ( isStandAlone() )
	survreadstatus_ = DBM().setDataSource(
			    File::Path(dataroot_,survdirnm).fullPath(), true );
    else
    {
	delete survinfo_; survinfo_ = 0;
	const BufferString survnm( getDirName() );
	if ( survnm.isEmpty() )
	    survreadstatus_.set( tr("Please create a survey") );
	else
	{
	    const BufferString fname = File::Path(dataroot_,survnm).fullPath();
	    survinfo_ = SurveyInfo::read( fname, survreadstatus_ );
	}
	if ( !survinfo_ )
	    survinfo_ = new SurveyInfo;
    }
}


#define mCheckRootDirWritable() \
    if ( !rootDirWritable() ) \
	{ uiMSG().error( rootdirnotwritablestr_.arg(dataroot_) ); return; }



void uiSurveyManager::newButPushed( CallBacker* )
{
    mCheckRootDirWritable();

    if ( !isStandAlone() )
	launchEditor( true );
    else
    {
	uiUserCreateSurvey dlg( this, dataroot_ );
	if ( dlg.go() )
	    setCurrentSurvey( dlg.dirName() );
    }
}


void uiSurveyManager::editButPushed( CallBacker* )
{
    if ( !isStandAlone() )
	{ launchEditor( false ); return; }

    uiSurveyInfoEditor dlg( this );
    dlg.go();
}


void uiSurveyManager::launchEditor( bool forcreate )
{
    const char* prognm = "od_Edit_Survey";
    BufferString cmd( prognm );
    OS::CommandLauncher::addQuotesIfNeeded( cmd );

    BufferString dirnmincmd( dataroot_ );
    OS::CommandLauncher::addQuotesIfNeeded( dirnmincmd );
    cmd.add( " --dataroot " ).add( dirnmincmd ).add( " " );
    cmd.add( forcreate ? "--create" : getDirName() );

    const bool res = OS::ExecCommand( cmd, OS::RunInBG, 0, 0 );
    if ( !res )
	uiMSG().error( tr("Could not launch '%1'.\nIt should be located in:"
			"\n%2\n\nPlease check your installation.")
			.arg( prognm ).arg( GetExecPlfDir() ) );
}


void uiSurveyManager::rmButPushed( CallBacker* )
{
    mCheckRootDirWritable();

    const BufferString selnm( getDirName() );
    const BufferString seldirnm = File::Path(dataroot_).add(selnm).fullPath();
    const BufferString truedirnm = File::linkEnd( seldirnm );

    uiString msg = tr("This will delete the entire survey directory:\n\t%1"
		      "\nFull path: %2").arg(selnm).arg(truedirnm);
    if ( !uiMSG().askRemove(msg) )
	return;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    stopFileMonitoring();
    const bool rmisok = File::remove( truedirnm );
    MouseCursorManager::restoreOverride();
    if ( !rmisok )
	uiMSG().error(tr("%1\nnot removed properly").arg(truedirnm));

    if ( seldirnm != truedirnm ) // must have been a link
	if ( !File::remove(seldirnm) )
	    uiMSG().error( uiStrings::phrCannotRemove(tr(
					    "link to the removed survey")) );
    updateList();
}


void uiSurveyManager::copyButPushed( CallBacker* )
{
    uiNewSurveyByCopy dlg( this, dataroot_, getDirName() );
    if ( !dlg.anySurvey() || !dlg.go() )
	return;

    setCurrentSurvey( dlg.survdirname_ );
}


void uiSurveyManager::extractButPushed( CallBacker* )
{
    mCheckRootDirWritable();

    uiFileDialog fdlg( this, true, 0, "*.zip", tr("Select survey zip file") );
    fdlg.setSelectedFilter( sZipFileMask );
    if ( !fdlg.go() )
	return;

    uiSurvey::unzipFile( this, fdlg.fileName(), dataroot_ );
    //TODO set unpacked survey as current but what is the name of it?
}


void uiSurveyManager::compressButPushed( CallBacker* )
{
    const char* survnm( getDirName() );
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

    File::Path zippath( fnmfld->fileName() );
    if ( FixedString(zippath.extension()) != "zip" )
	uiMSG().error( tr("Please add .zip extension to the file name") );
    else
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
	uiConvertPos dlg( this, *curSI() );
	dlg.go();
    }
    else
    {
	Util* util = getUtils()[butidx];
	util->cb_.doCall( this );
    }
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiSurveyManager::writeSettingsSurveyFile( const char* dirnm )
{
    if ( !File::exists(File::Path(dataroot_,dirnm).fullPath()) )
	mErrRet(tr("Survey directory does not exist anymore"))

    const char* survfnm = GetLastSurveyFileName();
    if ( !survfnm )
	mErrRet(tr("Internal error: cannot construct last-survey-filename"))

    od_ostream strm( survfnm );
    if ( !strm.isOK() )
	mErrRet(tr("Cannot open %1 for write").arg(survfnm))

    strm << dirnm;
    if ( !strm.isOK() )
	mErrRet( tr("Error writing to %1").arg(survfnm) )

    return true;
}


void uiSurveyManager::survDirChgCB( CallBacker* )
{
    writeCommentsIfChanged();
    setCurrentSurvey( getDirName() );
}


void uiSurveyManager::survParsChgCB( CallBacker* )
{
    setCurrentSurvey( getDirName() );
}


void uiSurveyManager::putToScreen()
{
    const bool isok = survreadstatus_.isOK();
    const SurveyInfo* cursi = isok ? curSI() : 0;
    if ( cursi )
    {
	TypeSet<int> inlines, crlines;
	const TrcKeyZSampling& cs = cursi->sampling( false );
	const TrcKeySampling& hs = cs.hsamp_;
	const int inlstep = hs.nrInl() / 5;
	for ( int idx=0; idx<4; idx++ )
	{
	    const int inl = hs.start_.inl() + (idx+1) * inlstep;
	    inlines += inl;
	}

	inlgrid_->set( inlines, crlines, hs );
	inlgridview_->setGrid( inlgrid_, cursi );
    }

    survmap_->setSurveyInfo( cursi );
    const bool havesurveys = haveSurveys();
    rmbut_->setSensitive( havesurveys );
    editbut_->setSensitive( havesurveys );
    for ( int idx=0; idx<utilbuts_.size(); idx++ )
    {
	const Util& util = *getUtils()[idx];
	utilbuts_[idx]->setSensitive( havesurveys
			&& cursi && util.willRunFor(*cursi) );
    }

    if ( !havesurveys || !isok )
    {
	notesfld_->setText( uiString::emptyString() );
	if ( havesurveys )
	    infofld_->setText( survreadstatus_ );
	else
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

    const SurveyInfo& si = *curSI();
    notesfld_->setText( si.comments() );

    zinfo.add( "(" )
	 .add( si.zIsTime() ? ZDomain::Time().unitStr().getFullString()
			    : getDistUnitString(si.zInFeet(), false) )
	 .add( "): " );

    bininfo.add( " (" ).add( si.xyUnitString(false).getFullString() )
	    .add( "/line): " );
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

    File::Path fp( si.getBasePath(), si.getDirName() );
    fp.makeCanonical();
    locinfo.add( fp.fullPath() );

    const float usrang = Math::degFromNorth( si.angleXInl() );
    orientinfo.add( toString(usrang,2) ).add( " Degrees from N" );

    BufferString infostr;
    infostr.add( inlinfo ).addNewLine().add( crlinfo ).addNewLine()
	.add( zinfo ).addNewLine().add( bininfo ).addNewLine()
	.add( areainfo ).add( "; ").add( survtypeinfo )
	.addNewLine().add( orientinfo ).addNewLine().add( locinfo );
    infofld_->setText( infostr );

}


void uiSurveyManager::writeCommentsIfChanged()
{
    if ( !notesfld_->isModified() )
	return;

    const_cast<SurveyInfo*>(curSI())->setComments( notesfld_->text() );
    curSI()->saveComments();
}


bool uiSurveyManager::commit()
{
    if ( isStandAlone() )
	{ pErrMsg("Huh"); return true; }
    else if ( !haveSurveys() )
	mErrRet(tr("Please create a survey (or press Cancel)"))

    const BufferString selsurv( getDirName() );
    if ( selsurv.isEmpty() )
	mErrRet(tr("No survey selected"))

    if ( !writeSettingsSurveyFile(selsurv) )
	return false; // err msg already emitted

    writeCommentsIfChanged();

    reReadSurvInfoFromFile( getDirName() );
    const SurveyInfo::ChangeData chgdata = SI().compareWith( *survinfo_ );
    if ( chgdata.isNoChange() )
	return true;

    if ( !SurveyInfo::isSetupChange(chgdata.changeType()) )
    {
	const_cast<SurveyInfo&>( SI() ) = *survinfo_;
	return true;
    }

    uiRetVal uirv = uiDataRootSel::setSurveyDirTo(
				File::Path(dataroot_,selsurv).fullPath() );
    if ( !uirv.isOK() )
    {
	if ( !uiStrings::sCancel().isEqualTo(uirv) )
	    uiMSG().error( uirv );
	return false;
    }

    return true;
}


uiSurveyManagerDlg::uiSurveyManagerDlg( uiParent* p, bool standalone )
    : uiDialog(p,uiDialog::Setup(tr("Survey Setup and Selection"),
			     mNoDlgTitle,mODHelpKey(mSurveySelectDlgHelpID)))
{
    mgrfld_ = new uiSurveyManager( this, standalone );
    if ( standalone )
	setCtrlStyle( CloseOnly );
    else
	setOkText( uiStrings::sSelect() );
    mAttachCB( mgrfld_->survDirAccept, uiSurveyManagerDlg::accept );
}


bool uiSurveyManagerDlg::acceptOK()
{
    return mgrfld_->commit();
}

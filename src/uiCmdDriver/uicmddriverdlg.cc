/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicmddriverdlg.h"

#include "cmddriverbasics.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uicursor.h"
#include "uidesktopservices.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "uitreeview.h"

#include "cmddriver.h"
#include "cmdrecorder.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "timer.h"


namespace CmdDrive
{

uiCmdInteractDlg::uiCmdInteractDlg( uiParent* p, const InteractSpec& ispec )
	: uiDialog( p, Setup(tr("Command interaction"), ispec.dlgtitle_,
                             mNoHelpKey).modal(false) )
	, unhide_( !ispec.wait_ )
{
    showAlwaysOnTop();
    setOkText( ispec.okbuttext_ );
    setCancelText( ispec.cancelbuttext_ );

    const char* ptr = ispec.infotext_.getFullString();
    int rows = 0;
    int cols = 0;
    while ( ptr && *ptr )
    {
	if ( *ptr++=='\n' || cols++==80 )
	{
	    cols = 0;
	    rows++;
	}
    }

    if ( cols )
	rows++;

    infofld_ = new uiTextEdit( this, "Info", true );
    infofld_->setPrefHeightInChar( mMIN(rows+1,20) );
    infofld_->setText( ispec.infotext_ );

    if ( !ispec.resumetext_.isEmpty() )
    {
	resumelbl_ = new uiLabel( this, ispec.resumetext_ );
	resumelbl_->attach( centeredBelow, infofld_ );
    }

    mDynamicCastGet( const uiMainWin*, parentwin, p );
    if ( ispec.launchpad_ )
	parentwin = ispec.launchpad_;

    if ( parentwin )
    {
	uiRect rect = parentwin->geometry( false );
	setCornerPos( rect.get(uiRect::Left), rect.get(uiRect::Top) );
    }
}


bool uiCmdInteractDlg::rejectOK( CallBacker* )
{ return button( CANCEL ); }


//=========================================================================


const char* optstrs[] = { "Run", "Record", 0 };

uiCmdDriverDlg::uiCmdDriverDlg( uiParent* p, CmdDriver& d, CmdRecorder& r,
			    const char* defscriptsdir, const char* deflogdir )
        : uiDialog( p, Setup( mToUiStringTodo(controllerTitle()),
			      tr("Specify your command script"),
			      mODHelpKey(mmcmddriverimpsHelpID) ).modal(false))
    , inpfldsurveycheck_(false)
    , outfldsurveycheck_(false)
    , logfldsurveycheck_(false)
    , drv_(d)
    , rec_(r)
    , defaultscriptsdir_(defscriptsdir)
    , defaultlogdir_(deflogdir)
    , interactdlg_(0)
{
    setCtrlStyle( CloseOnly );
    setCancelText( uiStrings::sHide() );

    cmdoptionfld_ = new uiLabeledComboBox( this, optstrs,
					   tr("Select script to") );
    mAttachCB( cmdoptionfld_->box()->selectionChanged,
		uiCmdDriverDlg::selChgCB );

    tooltipfld_ = new uiCheckBox( this, tr("Show Tooltips"),
				  mCB(this,uiCmdDriverDlg,toolTipChangeCB) );
    tooltipfld_->attach( rightOf, cmdoptionfld_ );
    tooltipfld_->setChecked( GetEnvVarYN("DTECT_USE_TOOLTIP_NAMEGUIDE") );
    toolTipChangeCB(0);

    const uiString commandfile = tr( "command file" );

    inpfld_ = new uiFileInput( this,
			uiStrings::phrInput(commandfile),
			uiFileInput::Setup(uiFileDialog::Gen)
			.filter("Script files (*.odcmd)")
			.forread(true)
			.withexamine(true)
			.exameditable(true)
			.displaylocalpath(true) );
    inpfld_->attach( alignedBelow, cmdoptionfld_ );
    mAttachCB( inpfld_->valueChanged, uiCmdDriverDlg::inpSelCB );

    logfld_ = new uiFileInput( this,
			uiStrings::phrOutput(uiStrings::sLogFile()),
			uiFileInput::Setup()
			.filter("Log files (*.log)")
			.forread(false)
			.withexamine(true)
			.examstyle(File::Log)
			.displaylocalpath(true) );
    logfld_->attach( alignedBelow, inpfld_ );

    outfld_ = new uiFileInput( this,
			uiStrings::phrOutput(commandfile),
			uiFileInput::Setup(uiFileDialog::Gen)
			.filter("Script files (*.odcmd)")
			.forread(false)
			.confirmoverwrite(false)
			.withexamine(true)
			.examstyle(File::Log)
			.displaylocalpath(true) );
    outfld_->attach( alignedBelow, cmdoptionfld_ );

    gobut_ = new uiPushButton( this, uiStrings::sGo(),
			mCB(this,uiCmdDriverDlg,selectGoCB), true );
    gobut_->attach( alignedBelow, logfld_ );

    pausebut_ = new uiPushButton( this, uiStrings::sPause(),
			mCB(this,uiCmdDriverDlg,selectPauseCB), true );
    pausebut_->attach( alignedBelow, logfld_ );

    abortbut_ = new uiPushButton( this, uiStrings::sAbort(),
			mCB(this,uiCmdDriverDlg,selectAbortCB), true );
    abortbut_->attach( rightOf, pausebut_ );

    startbut_ = new uiPushButton( this, uiStrings::sStart(),
			mCB(this,uiCmdDriverDlg,selectStartRecordCB), true );
    startbut_->attach( alignedBelow, logfld_ );

    stopbut_ = new uiPushButton( this, uiStrings::sStop(),
			mCB(this,uiCmdDriverDlg,selectStopRecordCB), true );
    stopbut_->attach( alignedBelow, logfld_ );

    uiLabel* cmddriverhackdummy mUnusedVar = new uiLabel( this,
					     uiString::emptyString() );

    mAttachCB( drv_.interactRequest, uiCmdDriverDlg::interactCB );

    setDefaultSelDirs();
    setDefaultLogFile();

    selChgCB( nullptr );

    mDynamicCastGet(const uiMainWin*,parentwin,p);
    if ( parentwin )
    {
	const uiRect rect = parentwin->geometry( false );
	setCornerPos( rect.get(uiRect::Left), rect.get(uiRect::Top) );
    }
}


#define mDeleteInteractDlg() \
    if ( interactdlg_ ) \
    { \
	delete interactdlg_; \
	interactdlg_ = 0; \
	button(CANCEL)->setSensitive( true ); \
    }

uiCmdDriverDlg::~uiCmdDriverDlg()
{
    detachAllNotifiers();
    mDeleteInteractDlg();
}


void uiCmdDriverDlg::popUp()
{
    showNormal();
    raise();

    if ( interactdlg_ && (!interactdlg_->isHidden() || interactdlg_->unHide()) )
    {
	interactdlg_->showNormal();
	interactdlg_->raise();
    }
}


void uiCmdDriverDlg::refreshDisplay( bool runmode, bool idle )
{
    cmdoptionfld_->box()->setCurrentItem( runmode ? "Run" : "Record" );
    logfld_->displayField( runmode );
    inpfld_->displayField( runmode );
    outfld_->displayField( !runmode );

    gobut_->display( runmode && idle );
    abortbut_->setText( uiStrings::sAbort() );
    abortbut_->display( runmode && !idle );
    startbut_->display( !runmode && idle );
    stopbut_->display( !runmode && !idle );

    pausebut_->setText( uiStrings::sPause() );
    pausebut_->setSensitive( true );
    pausebut_->display( runmode && !idle );

    cmdoptionfld_->setSensitive( idle );
    inpfld_->setSensitive( idle );
    inpfld_->enableExamine( true );

    logfld_->setSensitive( idle );
    logfld_->enableExamine( true );
    outfld_->setSensitive( idle );
    outfld_->enableExamine( true );
}


bool uiCmdDriverDlg::rejectOK( CallBacker* )
{
    if ( interactdlg_ && !interactdlg_->isHidden() )
	return false;

    // Remember last position
    const uiRect frame = geometry();
    setCornerPos( frame.get(uiRect::Left), frame.get(uiRect::Top) );

    mDeleteInteractDlg();
    return true;
}


void uiCmdDriverDlg::interactClosedCB( CallBacker* )
{
    uiCursorManager::setPriorityCursor( MouseCursor::Forbidden );
    pausebut_->setSensitive();
    button(CANCEL)->setSensitive( true );
    drv_.pause( false );
}


void uiCmdDriverDlg::toolTipChangeCB( CallBacker* )
{
    uiMain::useNameToolTip( tooltipfld_->isChecked() );
}


void uiCmdDriverDlg::selChgCB( CallBacker* )
{
    refreshDisplay( !cmdoptionfld_->box()->currentItem(), true );
}


void uiCmdDriverDlg::inpSelCB( CallBacker* )
{
    setDefaultLogFile();
}


static bool isRefToDataDir( uiFileInput& fld, bool base=false )
{
    FilePath fp( fld.fileName() );
    BufferString dir = base ? GetBaseDataDir() : GetDataDir();
    dir += FilePath::dirSep(FilePath::Local);
    return dir.isStartOf( fp.fullPath() );
}


static bool passSurveyCheck( uiFileInput& fld, bool& surveycheck )
{
    if ( !surveycheck )
	return true;

    int res = 1;
    if ( isRefToDataDir(fld,true) && !isRefToDataDir(fld,false) )
    {
	uiString msg =
	    od_static_tr( "passSurveyCheck" ,
			  "%1 - path is referring to previous survey!" )
			.arg( fld.titleText().getFullString() );
	res = uiMSG().question(msg, uiStrings::sContinue(), uiStrings::sReset(),
				  uiStrings::sCancel(), uiStrings::sWarning() );
	surveycheck = res<0;

	if ( !res )
	    fld.setFileName( "" );
    }
    return res>0;
}


void uiCmdDriverDlg::selectGoCB( CallBacker* )
{
    if ( !passSurveyCheck(*inpfld_,inpfldsurveycheck_) )
	return;

    if ( !passSurveyCheck(*logfld_,logfldsurveycheck_) )
    {
	if ( *logfld_->text() )
	    return;

	setDefaultLogFile();
    }

    BufferString fnm = inpfld_->fileName();
    if ( File::isDirectory(fnm) || File::isEmpty(fnm) )
    {
	uiMSG().error( tr("Invalid command file selected") );
	return;
    }

    FilePath fp( logfld_->fileName() );
    if ( !File::isWritable(fp.pathOnly()) ||
	 (File::exists(fp.fullPath()) && !File::isWritable(fp.fullPath())) )
    {
	uiMSG().error( tr("Log file cannot be written") );
	return;
    }

    drv_.setOutputDir( fp.pathOnly() );
    drv_.setLogFileName( fp.fileName() );
    drv_.clearLog();

    if ( !drv_.getActionsFromFile(fnm) )
    {
	uiMSG().error( mToUiStringTodo(drv_.errMsg()) );
	return;
    }

    logfld_->setFileName( fp.fullPath() );
    inpfld_->setFileName( fnm );

    refreshDisplay( true, false );
    drv_.execute();
}


void uiCmdDriverDlg::selectPauseCB( CallBacker* )
{
    BufferString buttext = pausebut_->text().getFullString();
    if ( buttext == uiStrings::sResume().getFullString()  )
    {
	pausebut_->setText( uiStrings::sPause() );
	drv_.pause( false );
    }
    else
    {
	pausebut_->setText( sInterrupting() );
	drv_.pause( true );
    }
}


void uiCmdDriverDlg::interactCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const InteractSpec*, ispec, cb );

    if ( !ispec )
    {
	interactClosedCB( 0 );
	mDeleteInteractDlg();
	return;
    }

    BufferString buttext = pausebut_->text().getFullString();
    if ( buttext==sInterrupting().getFullString() && ispec->dlgtitle_.isEmpty())
    {
	pausebut_->setText( uiStrings::sResume() );
	return;
    }

    mDeleteInteractDlg();
    popUp();

    uiCursorManager::setPriorityCursor( ispec->cursorshape_ );
    interactdlg_ = new uiCmdInteractDlg( this, *ispec );
    if ( ispec->wait_ )
    {
	pausebut_->setText( uiStrings::sPause() );
	pausebut_->setSensitive( false );
	mAttachCB( interactdlg_->windowClosed,
		   uiCmdDriverDlg::interactClosedCB );
    }

    button(CANCEL)->setSensitive( false );
    interactdlg_->go();
}


void uiCmdDriverDlg::selectAbortCB( CallBacker* )
{
    drv_.abort();
    abortbut_->setText( sInterrupting() );

    pausebut_->setText( uiStrings::sPause() );
    pausebut_->setSensitive( false );
}


void uiCmdDriverDlg::executeFinished()
{
    if ( rec_.isRecording() )
    {
	refreshDisplay( false, false );
	Timer::setUserWaitFlag( false );
    }
    else
    {
	mDeleteInteractDlg();
	refreshDisplay( true, true );
    }

    logproposal_.setEmpty();
}


void uiCmdDriverDlg::selectStartRecordCB( CallBacker* )
{
    if ( !passSurveyCheck(*outfld_, outfldsurveycheck_) )
	return;

    FilePath fp( outfld_->fileName() );
    fp.setExtension( ".odcmd" );

    if ( !File::isWritable(fp.pathOnly()) ||
	 (File::exists(fp.fullPath()) && !File::isWritable(fp.fullPath())) )
    {
	uiMSG().error( tr("Command file cannot be written") );
	return;
    }

    if ( File::exists(fp.fullPath()) )
    {
	if ( !uiMSG().askOverwrite(tr("Overwrite existing command file?")) )
	    return;
    }

    rec_.setOutputFile( fp.fullPath() );
    outfld_->setFileName( fp.fullPath() );
    refreshDisplay( false, false );
    rec_.start();
}


void uiCmdDriverDlg::selectStopRecordCB( CallBacker* )
{
    rec_.stop();
    inpfld_->setFileName( outfld_->fileName() );
    refreshDisplay( true, true );
}


void uiCmdDriverDlg::beforeSurveyChg()
{
    inpfldsurveycheck_ = inpfldsurveycheck_ || isRefToDataDir( *inpfld_ );
    logfldsurveycheck_ = logfldsurveycheck_ || isRefToDataDir( *logfld_ );
    outfldsurveycheck_ = outfldsurveycheck_ || isRefToDataDir( *outfld_ );
}


void uiCmdDriverDlg::afterSurveyChg()
{
    setDefaultSelDirs();

    const bool untouchedlog = logproposal_==logfld_->fileName();
    if ( logfldsurveycheck_ && !drv_.nowExecuting() && untouchedlog )
	setDefaultLogFile();
}


void uiCmdDriverDlg::setDefaultSelDirs()
{
    const char* dir = defaultscriptsdir_.isEmpty() ? GetScriptsDir()
				: defaultscriptsdir_.buf();
    inpfld_->setDefaultSelectionDir( dir );
    outfld_->setDefaultSelectionDir( dir );

    dir = defaultlogdir_.isEmpty() ? GetProcFileName(0)
		      : defaultlogdir_.buf();
    logfld_->setDefaultSelectionDir( dir );
}


void uiCmdDriverDlg::setDefaultLogFile()
{
    const char* dir = defaultlogdir_.isEmpty() ? GetScriptsLogDir()
					       : defaultlogdir_.buf();

    BufferString deflogfnm = CmdDriver::defaultLogFilename();
    const FilePath inpfp( inpfld_->fileName() );
    if ( !inpfp.isEmpty() )
	deflogfnm = inpfp.fileName();

    FilePath logfp( dir, deflogfnm );
    logfp.setExtension( "log" );
    logproposal_ = logfp.fullPath();
    logfld_->setFileName( logproposal_ );
    logfldsurveycheck_ = false;
}


void uiCmdDriverDlg::autoStartGo( const char* fnm )
{
    if ( drv_.nowExecuting() )
    {
	if ( drv_.insertActionsFromFile(fnm) )
	{
	    inpfld_->setFileName( fnm );
	    refreshDisplay( true, false );
	}
    }
    else if ( drv_.getActionsFromFile(fnm) )
    {
	inpfld_->setFileName( fnm );
	refreshDisplay( true, false );
	drv_.execute();
    }
    else
	drv_.executeFinished.trigger();
}



// uiScriptRunnerDlg
class uiScriptRunnerSettings : public uiDialog
{
mODTextTranslationClass(uiScriptRunnerSettings)
public:

uiScriptRunnerSettings( uiParent* p )
    : uiDialog(p,Setup(tr("Command Driver Settings"),mNoDlgTitle,mNoHelpKey))
{
    scriptsdirfld_ = new uiFileInput( this,
			tr("Scripts folder"),
			uiFileInput::Setup()
			    .directories(true)
			    .forread(false)
			    .defseldir(GetScriptsDir()) );
    scriptsdirfld_->setText( GetScriptsDir() );

    logdirfld_ = new uiFileInput( this,
			tr("Logs folder"),
			uiFileInput::Setup()
			    .directories(true)
			    .forread(false)
			    .defseldir(GetScriptsLogDir()) );
    logdirfld_->setText( GetScriptsLogDir() );
    logdirfld_->attach( alignedBelow, scriptsdirfld_ );

    picturesdirfld_ = new uiFileInput( this,
			tr("Pictures folder"),
			uiFileInput::Setup()
			    .directories(true)
			    .forread(false)
			    .defseldir(GetScriptsPicturesDir()) );
    picturesdirfld_->setText( GetScriptsPicturesDir() );
    picturesdirfld_->attach( alignedBelow, logdirfld_ );
}

~uiScriptRunnerSettings()
{}


bool acceptOK( CallBacker* )
{
    FilePath fp = scriptsdirfld_->fileName();
    if ( !fp.exists() )
    {
	uiMSG().error( tr("Scritps folder does not exist.") );
	return false;
    }

    fp = logdirfld_->fileName();
    if ( !fp.exists() )
    {
	uiMSG().error( tr("Scritps folder does not exist.") );
	return false;
    }

    fp = picturesdirfld_->fileName();
    if ( !fp.exists() )
    {
	uiMSG().error( tr("Scritps folder does not exist.") );
	return false;
    }

    SetEnvVar( "DTECT_SCRIPTS_DIR", scriptsdirfld_->fileName() );
    SetEnvVar( "DTECT_SCRIPTS_LOG_DIR", logdirfld_->fileName() );
    SetEnvVar( "DTECT_SCRIPTS_PICTURES_DIR", picturesdirfld_->fileName() );

    return true;
}


    uiFileInput*	scriptsdirfld_;
    uiFileInput*	logdirfld_;
    uiFileInput*	picturesdirfld_;

}; // class uiScriptRunnerSettings



class ScriptItem : public uiTreeViewItem
{
public:
ScriptItem( uiTreeViewItem* parent, const char* fnm, CmdDriver& drv )
    : uiTreeViewItem(parent,Setup())
    , fnm_(fnm)
    , driver_(drv)
{
    const FilePath fp( fnm );
    setText( toUiString(fp.fileName()), 0 );

    logfnm_ = getLogFilename().fullPath();
}


ScriptItem( uiTreeView* parent, const char* fnm, CmdDriver& drv )
    : uiTreeViewItem(parent,Setup())
    , fnm_(fnm)
    , driver_(drv)
{
    const FilePath fp( fnm );
    setText( toUiString(fp.fileName()), 0 );
    setIcon( 1, "empty" );

    logfnm_ = getLogFilename().fullPath();
}


bool execute()
{
    setIcon( 1, "inprogress" );
    setSelected( true );
    mAttachCB( driver_.executeFinished, ScriptItem::executeFinishedCB );

    const FilePath logfp = getLogFilename();
    logfnm_ = logfp.fullPath();
    if ( File::exists(logfnm_) && File::isFile(logfnm_) )
	File::remove( logfnm_ );

    driver_.setLogFileName( logfp.fileName() );
    driver_.getActionsFromFile( fnm_ );
    return driver_.execute();
}


void executeFinishedCB( CallBacker* )
{
    const bool haserror = driver_.scriptAborted() || driver_.scriptFailed();
    setIcon( 1, haserror ? "abort" : "checkgreen" );
    setSelected( false );
    mDetachCB( driver_.executeFinished, ScriptItem::executeFinishedCB );
}


FilePath getLogFilename() const
{
    const FilePath scriptfp( fnm_ );
    FilePath logfp( driver_.outputDir() );
    logfp.add( scriptfp.fileName() );
    logfp.setExtension( "log" );
    return logfp;
}

    BufferString		fnm_;
    BufferString		logfnm_;

    enum Status			{ Pending, Started, FinishedOK, FinishedError };
    Status			status_;
    CmdDriver&			driver_;

}; // class ScriptItem



uiScriptRunnerDlg::uiScriptRunnerDlg( uiParent* p, CmdDriver& driver )
    : uiDialog(p,Setup(tr("Run Command Driver Script"),
		       mNoDlgTitle,mTODOHelpKey))
    , drv_(driver)
{
    setCtrlStyle( CloseOnly );
    drv_.setOutputDir( GetScriptsLogDir() );

    mAttachCB( drv_.executeFinished, uiScriptRunnerDlg::executeFinishedCB );

    scriptfld_ = new uiFileInput( this,
			uiStrings::phrInput(tr("Command Driver Script")),
			uiFileInput::Setup(uiFileDialog::Gen)
			    .filter("Script files (*.odcmd)")
			    .forread(true)
			    .withexamine(false)
			    .defseldir(GetScriptsDir())
			    .displaylocalpath(true) );
    mAttachCB( scriptfld_->valueChanged, uiScriptRunnerDlg::inpSelCB );

    auto* settingsbut = new uiToolButton( this, "settings",
					uiStrings::sSettings(),
					mCB(this,uiScriptRunnerDlg,settingsCB));
    settingsbut->attach( rightOf, scriptfld_ );

    scriptlistfld_ = new uiTreeView( this, "Script Tree" );
    scriptlistfld_->attach( ensureBelow, scriptfld_ );
    uiStringSet lbls;
    lbls.add( tr("Script") ).add( uiStrings::sStatus() );
    scriptlistfld_->setSelectionMode( uiTreeView::Single );
    scriptlistfld_->addColumns( lbls );
    scriptlistfld_->setColumnWidthMode( 0, uiTreeView::Stretch );
    scriptlistfld_->setColumnWidth( 0, 350 );
    scriptlistfld_->setFixedColumnWidth( 1, 50 );
    scriptlistfld_->setPrefWidth( 600 );
    scriptlistfld_->setPrefHeight( 400 );
    scriptlistfld_->setStretch( 2, 2 );
    mAttachCB( scriptlistfld_->doubleClicked, uiScriptRunnerDlg::doubleClickCB);
    mAttachCB( scriptlistfld_->rightButtonPressed,
	       uiScriptRunnerDlg::rightClickCB );
    mAttachCB( scriptlistfld_->selectionChanged, uiScriptRunnerDlg::selChgCB );

    auto* itemgrp = new uiButtonGroup( this, "Item group", OD::Vertical );
    editbut_ = new uiToolButton( itemgrp, "edit", tr("Edit script"),
				mCB(this,uiScriptRunnerDlg,editCB) );
    editbut_->setSensitive( false );
    logbut_ = new uiToolButton( itemgrp, "logfile", tr("View log"),
				mCB(this,uiScriptRunnerDlg,logCB) );
    logbut_->setSensitive( false );
    itemgrp->attach( centeredRightOf, scriptlistfld_ );

    auto* grp = new uiButtonGroup( this, "", OD::Horizontal );
    grp->attach( centeredBelow, scriptlistfld_ );
    gobut_ = new uiPushButton( grp, uiStrings::sGo(), "resume",
			       mCB(this,uiScriptRunnerDlg,goCB), true );
    stopbut_ = new uiPushButton( grp, uiStrings::sStop(), "stop",
				 mCB(this,uiScriptRunnerDlg,stopCB), true );
    stopbut_->setSensitive( false );
}


uiScriptRunnerDlg::~uiScriptRunnerDlg()
{
    detachAllNotifiers();
}


void uiScriptRunnerDlg::inpSelCB( CallBacker* )
{
    scriptlistfld_->clear();

    const FilePath inpfp( scriptfld_->fileName() );
    if ( inpfp.isEmpty() )
	return;

    auto* item = new ScriptItem( scriptlistfld_, inpfp.fullPath(), drv_ );
    addChildren( *item );
    scriptlistfld_->expandAll();
}


void uiScriptRunnerDlg::settingsCB( CallBacker* )
{
    const BufferString scriptsdir = GetScriptsDir();
    const BufferString scriptslogdir = GetScriptsLogDir();
    uiScriptRunnerSettings dlg( this );
    if ( !dlg.go() )
	return;

    const BufferString newscriptsdir = GetScriptsDir();
    if ( scriptsdir != newscriptsdir )
    {
	scriptfld_->setEmpty();
	scriptfld_->setDefaultSelectionDir( newscriptsdir );
	scriptlistfld_->clear();
    }

    const BufferString newscriptslogdir = GetScriptsLogDir();
    if ( scriptslogdir != newscriptslogdir )
    {
	drv_.setOutputDir( newscriptslogdir );
    }
}


void uiScriptRunnerDlg::addChildren( ScriptItem& parent )
{
    BufferStringSet includes;
    drv_.getIncludedScripts( parent.fnm_, includes );
    for ( auto* fnm : includes )
    {
	auto* item = new ScriptItem( &parent, fnm->buf(), drv_ );
	addChildren( *item );
    }
}


void uiScriptRunnerDlg::goCB( CallBacker* )
{
    abort_ = false;
    gobut_->setSensitive( false );
    stopbut_->setSensitive( true );

    drv_.setOutputDir( GetScriptsLogDir() );

    deleteAndNullPtr( iter_ );

    auto* curitm = scriptlistfld_->currentItem();
    if ( curitm )
	iter_ = new uiTreeViewItemIterator( *curitm );
    else
	iter_ = new uiTreeViewItemIterator( *scriptlistfld_ );

    executeNext();
}


void uiScriptRunnerDlg::stopCB( CallBacker* )
{
    abort_ = true;
    drv_.abort();
}


bool uiScriptRunnerDlg::executeNext()
{
    uiTreeViewItem* item = iter_ ? iter_->next() : nullptr;
    if ( !item || abort_ )
    {
	gobut_->setSensitive( true );
	stopbut_->setSensitive( false );
	return false;
    }

    if ( item->nrChildren() > 0 )
	return executeNext();

    auto* scriptitem = sCast(ScriptItem*,item);
    return scriptitem->execute();
}


void uiScriptRunnerDlg::executeFinishedCB( CallBacker* )
{
    executeNext();
}


void uiScriptRunnerDlg::doubleClickCB( CallBacker* )
{
    uiTreeViewItem* item = scriptlistfld_->itemNotified();
    auto* scriptitem = sCast(ScriptItem*,item);
    if ( !scriptitem )
	return;

    const int column = scriptlistfld_->columnNotified();
    const BufferString fnm = column==0 ? scriptitem->fnm_ : scriptitem->logfnm_;
    if ( !File::exists(fnm) )
    {
	uiMSG().error( tr("File does not exist:\n%1").arg(fnm) );
	return;
    }

    File::launchViewer( column==0 ? scriptitem->fnm_ : scriptitem->logfnm_ );
}


void uiScriptRunnerDlg::rightClickCB( CallBacker* )
{
    const int column = scriptlistfld_->columnNotified();
    if ( column != 0 )
	return;

    uiTreeViewItem* item = scriptlistfld_->itemNotified();
    auto* scriptitem = sCast(ScriptItem*,item);
    if ( !scriptitem )
	return;

    uiMenu menu( uiStrings::sAction() );
    menu.insertAction( new uiAction(uiStrings::sEdit()), 0 );
    menu.insertAction( new uiAction(tr("Run this script only")), 1 );
    const int res = menu.exec();
    if ( res < 0 )
	return;

    if ( res==0 )
	uiDesktopServices::openUrl( scriptitem->fnm_ );
    else if ( res==1 )
	scriptitem->execute();
}


void uiScriptRunnerDlg::selChgCB( CallBacker* )
{
    uiTreeViewItem* item = scriptlistfld_->selectedItem();
    auto* scriptitem = sCast(ScriptItem*,item);

    const bool enabedit = scriptitem && File::exists(scriptitem->fnm_);
    const bool enablog = scriptitem && File::exists(scriptitem->logfnm_);

    editbut_->setSensitive( enabedit );
    logbut_->setSensitive( enablog );
}


void uiScriptRunnerDlg::editCB( CallBacker* )
{
    uiTreeViewItem* item = scriptlistfld_->selectedItem();
    auto* scriptitem = sCast(ScriptItem*,item);
    if ( !scriptitem )
	return;

    uiDesktopServices::openUrl( scriptitem->fnm_ );
}


void uiScriptRunnerDlg::logCB( CallBacker* )
{

    uiTreeViewItem* item = scriptlistfld_->selectedItem();
    auto* scriptitem = sCast(ScriptItem*,item);
    if ( !scriptitem )
	return;

    File::launchViewer( scriptitem->logfnm_ );
}


bool uiScriptRunnerDlg::acceptOK( CallBacker* )
{
    return false;
}

} // namespace CmdDrive

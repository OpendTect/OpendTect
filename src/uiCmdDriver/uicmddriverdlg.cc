/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uicmddriverdlg.cc,v 1.1 2012-09-17 12:37:42 cvsjaap Exp $";

#include "uicmddriverdlg.h"

#include "cmddriverbasics.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uicursor.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitextedit.h"

#include "cmddriver.h"
#include "cmdrecorder.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "oddirs.h"
#include "timer.h"

namespace CmdDrive
{


uiCmdInteractDlg::uiCmdInteractDlg( uiParent* p, const InteractSpec& ispec )
	: uiDialog( p, Setup("Command interaction", ispec.dlgtitle_, mNoHelpID)
		       .modal(false) )
	, unhide_( !ispec.wait_ )
{
    showAlwaysOnTop();
    setOkText( ispec.okbuttext_ );
    setCancelText( ispec.cancelbuttext_ );

    const char* ptr = ispec.infotext_;
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

uiCmdDriverDlg::uiCmdDriverDlg( uiParent* p, CmdDriver& d, CmdRecorder& r )
        : uiDialog( 0, Setup( controllerTitle(), "Specify your command script",
			      "0.4.3").modal(false))
	, drv_(d), rec_(r)
	, inpfldsurveycheck_(false)
	, outfldsurveycheck_(false)
	, logfldsurveycheck_(false)
	, interactdlg_(0)
{
    setCtrlStyle( LeaveOnly );
    setCancelText( "&Hide" );

    cmdoptionfld_ = new uiLabeledComboBox( this, optstrs, "Select script to" );
    cmdoptionfld_->box()->selectionChanged.notify(
	    				  mCB(this,uiCmdDriverDlg,selChgCB) );

    tooltipfld_ = new uiCheckBox( this, "ToolTipNameGuide",
	    			  mCB(this,uiCmdDriverDlg,toolTipChangeCB) );
    tooltipfld_->attach( rightOf, cmdoptionfld_ );
    tooltipfld_->setChecked( GetEnvVarYN("DTECT_USE_TOOLTIP_NAMEGUIDE") );
    toolTipChangeCB(0);

    inpfld_ = new uiFileInput( this, "Input command file",
			uiFileInput::Setup(uiFileDialog::Gen)
			.filter("Script files (*.odcmd *.cmd)")
			.forread(true)
			.withexamine(true)
			.examstyle(uiFileInput::Setup::Edit)
			.displaylocalpath(true) );
    inpfld_->attach( alignedBelow, cmdoptionfld_ );

    logfld_ = new uiFileInput( this, "Output log file",
			uiFileInput::Setup()
			.forread(false)
			.withexamine(true)
			.examstyle(uiFileInput::Setup::Log)
			.displaylocalpath(true) );
    logfld_->attach( alignedBelow, inpfld_ );

    outfld_ = new uiFileInput( this, "Output command file",
			uiFileInput::Setup(uiFileDialog::Gen)
			.filter("Script files (*.odcmd)")
			.forread(false)
			.confirmoverwrite(false)
			.withexamine(true)
	   		.examstyle(uiFileInput::Setup::Log)
			.displaylocalpath(true) );
    outfld_->attach( alignedBelow, cmdoptionfld_ );

    gobut_ = new uiPushButton( this, "&Go",
			mCB(this,uiCmdDriverDlg,selectGoCB), true );
    gobut_->attach( alignedBelow, logfld_ );

    pausebut_ = new uiPushButton( this, "&Pause",
			mCB(this,uiCmdDriverDlg,selectPauseCB), true );
    pausebut_->attach( alignedBelow, logfld_ );

    abortbut_ = new uiPushButton( this, "&Abort",
			mCB(this,uiCmdDriverDlg,selectAbortCB), true );
    abortbut_->attach( rightOf, pausebut_ );

    startbut_ = new uiPushButton( this, "S&tart",
			mCB(this,uiCmdDriverDlg,selectStartRecordCB), true );
    startbut_->attach( alignedBelow, logfld_ );

    stopbut_ = new uiPushButton( this, "S&top",
			mCB(this,uiCmdDriverDlg,selectStopRecordCB), true );
    stopbut_->attach( alignedBelow, logfld_ );

    uiLabel* cmddriverhackdummy mUnusedVar = new uiLabel( this, "" );

    drv_.interactRequest.notify( mCB(this,uiCmdDriverDlg,interactCB) );

    setDefaultSelDirs();
    setDefaultLogFile();
    const FilePath fp( drv_.outputDir(), drv_.logFileName() );
    logfld_->setFileName( fp.fullPath() );

    selChgCB(0);

    mDynamicCastGet( const uiMainWin*, parentwin, p );
    if ( parentwin )
    {
	uiRect rect = parentwin->geometry( false );
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
    drv_.interactRequest.remove( mCB(this,uiCmdDriverDlg,interactCB) );
    mDeleteInteractDlg();
}


#define mPopUp( dlg, unhide ) \
    if ( dlg && (!dlg->isHidden() || unhide) ) \
    { \
	dlg->showNormal(); \
	dlg->raise(); \
    } \

void uiCmdDriverDlg::popUp()
{
     mPopUp( this, true );
     mPopUp( interactdlg_, interactdlg_->unHide() );
}


void uiCmdDriverDlg::refreshDisplay( bool runmode, bool idle )
{
    cmdoptionfld_->box()->setCurrentItem( runmode ? "Run" : "Record" );
    logfld_->displayField( runmode );
    inpfld_->displayField( runmode );
    outfld_->displayField( !runmode );

    gobut_->display( runmode && idle );
    abortbut_->setText( "&Abort" );
    abortbut_->display( runmode && !idle );
    startbut_->display( !runmode && idle );
    stopbut_->display( !runmode && !idle );

    pausebut_->setText( "&Pause" );
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
    uiObject::useNameToolTip( tooltipfld_->isChecked() );
}


void uiCmdDriverDlg::selChgCB( CallBacker* )
{
    refreshDisplay( !cmdoptionfld_->box()->currentItem(), true );
}


static bool isRefToDataDir( uiFileInput& fld, bool base=false )
{
    FilePath fp( fld.fileName() );
    BufferString dir = base ? GetBaseDataDir() : GetDataDir();
    dir += FilePath::dirSep(FilePath::Local);
    return matchString( dir.buf(), fp.fullPath() );
}


static bool passSurveyCheck( uiFileInput& fld, bool& surveycheck )
{
    if ( !surveycheck ) return true;

    int res = 1;
    if ( isRefToDataDir(fld,true) && !isRefToDataDir(fld,false) )
    {
	BufferString msg = fld.titleText();
	msg += "-path is referring to previous survey!";
	res = uiMSG().question( msg, "Continue", "Reset", "Cancel", "Warning" );
	surveycheck = res<0;

	if ( !res )
	    fld.setFileName( "" );
    }
    return res>0;
}


bool uiCmdDriverDlg::selectGoCB( CallBacker* )
{
    if ( !passSurveyCheck(*inpfld_, inpfldsurveycheck_) )
	return false;

    if ( !passSurveyCheck(*logfld_, logfldsurveycheck_) )
    {
	if ( *logfld_->text() )
	    return false;

	setDefaultLogFile();
    }

    BufferString fnm = inpfld_->fileName();
    if ( File::isDirectory(fnm) || File::isEmpty(fnm) )
    {
	uiMSG().error( "Invalid command file selected" );
	return false;
    }

    FilePath fp( logfld_->fileName() );
    if ( !File::isWritable(fp.pathOnly()) ||
	 (File::exists(fp.fullPath()) && !File::isWritable(fp.fullPath())) )
    {
	uiMSG().error( "Log file cannot be written" );
	return false;
    }

    drv_.setOutputDir( fp.pathOnly() );
    drv_.setLogFileName( fp.fileName() );
    drv_.clearLog();

    if ( !drv_.getActionsFromFile(fnm) )
    {
	uiMSG().error( drv_.errMsg() );
	return false;
    }

    logfld_->setFileName( fp.fullPath() );
    inpfld_->setFileName( fnm );

    refreshDisplay( true, false );
    drv_.execute();

    return true;
}


void uiCmdDriverDlg::selectPauseCB( CallBacker* )
{
    BufferString buttext = pausebut_->text();
    if ( strcmp(buttext,"&Resume") )
    {
	pausebut_->setText( "-Interrupting-" );
	drv_.pause( true );
    }
    else
    {
	pausebut_->setText( "&Pause" );
	drv_.pause( false );
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

    BufferString buttext = pausebut_->text();
    if ( !strcmp(buttext,"-Interrupting-") && ispec->dlgtitle_.isEmpty() )
    {
	pausebut_->setText( "&Resume" );
	return;
    }

    mDeleteInteractDlg();
    popUp();

    uiCursorManager::setPriorityCursor( ispec->cursorshape_ );
    interactdlg_ = new uiCmdInteractDlg( this, *ispec );
    if ( ispec->wait_ )
    {
	pausebut_->setText( "&Pause" );
	pausebut_->setSensitive( false );
	interactdlg_->windowClosed.notify(
				mCB(this,uiCmdDriverDlg,interactClosedCB) );
    }
    button(CANCEL)->setSensitive( false );
    interactdlg_->go();
}


void uiCmdDriverDlg::selectAbortCB( CallBacker* )
{
    drv_.abort();
    abortbut_->setText( "-Interrupting-" );

    pausebut_->setText( "&Pause" );
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


bool uiCmdDriverDlg::selectStartRecordCB( CallBacker* )
{
    if ( !passSurveyCheck(*outfld_, outfldsurveycheck_) )
	return false;

    FilePath fp( outfld_->fileName() );
    fp.setExtension( ".odcmd" );

    if ( !File::isWritable(fp.pathOnly()) ||
	 (File::exists(fp.fullPath()) && !File::isWritable(fp.fullPath())) )
    {
	uiMSG().error( "Command file cannot be written" );
	return false;
    }

    if ( File::exists(fp.fullPath()) )
    {
	if ( !uiMSG().askOverwrite("Overwrite existing command file?") )
	    return false;
    }

    rec_.setOutputFile( fp.fullPath() );
    outfld_->setFileName( fp.fullPath() );
    refreshDisplay( false, false );
    rec_.start();

    return true;
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
    inpfld_->setDefaultSelectionDir( GetScriptsDir(0) );
    logfld_->setDefaultSelectionDir( GetProcFileName(0) );
    outfld_->setDefaultSelectionDir( GetScriptsDir(0) );
}


void uiCmdDriverDlg::setDefaultLogFile()
{
    logproposal_ = GetProcFileName( CmdDriver::defaultLogFilename() );
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


}; // namespace CmdDrive

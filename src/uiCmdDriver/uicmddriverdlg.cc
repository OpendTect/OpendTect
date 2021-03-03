/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Sep 2009
________________________________________________________________________

-*/

#include "uicmddriverdlg.h"

#include "cmddriverbasics.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uicursor.h"
#include "uifilesel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitextedit.h"
#include "uimain.h"

#include "cmddriver.h"
#include "cmdrecorder.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "timer.h"
#include "od_helpids.h"

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

    const BufferString bufstr = toString( ispec.infotext_ );
    const char* ptr = bufstr.str();
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


bool uiCmdInteractDlg::rejectOK()
{ return button( CANCEL ); }


//=========================================================================


uiCmdDriverDlg::uiCmdDriverDlg( uiParent* p, CmdDriver& d, CmdRecorder& r,
			    const char* defscriptsdir, const char* deflogdir )
        : uiDialog( p, Setup( controllerUiTitle(),
			      tr("Specify your command script"),
			      mODHelpKey(mmcmddriverimpsHelpID) ).modal(false))
	, drv_(d), rec_(r)
	, inpfldsurveycheck_(false)
	, outfldsurveycheck_(false)
	, logfldsurveycheck_(false)
	, interactdlg_(0)
	, defaultscriptsdir_(defscriptsdir)
	, defaultlogdir_(deflogdir)
{
    setCtrlStyle( CloseOnly );
    setCancelText( uiStrings::sHide() );

    uiStringSet opts;
    opts.add( uiStrings::sRun() ).add( tr("Record") );
    cmdoptionfld_ = new uiLabeledComboBox( this, opts, tr("Select script to") );
    cmdoptionfld_->box()->selectionChanged.notify(
					  mCB(this,uiCmdDriverDlg,selChgCB) );

    tooltipfld_ = new uiCheckBox( this, tr("Show Tooltips"),
				  mCB(this,uiCmdDriverDlg,toolTipChangeCB) );
    tooltipfld_->attach( rightOf, cmdoptionfld_ );
    tooltipfld_->setChecked( GetEnvVarYN("DTECT_USE_TOOLTIP_NAMEGUIDE") );
    toolTipChangeCB(0);

    const uiString commandfile = tr("command file");

    const File::Format fmt( tr("Script files"), "odcmd", "cmd" );
    uiFileSel::Setup fssu( OD::GeneralContent );
    fssu.withexamine(true).exameditable(true)
	.displaylocalpath(true).formats(fmt);
    inpfld_ = new uiFileSel( this, uiStrings::phrInput(commandfile), fssu );
    inpfld_->attach( alignedBelow, cmdoptionfld_ );

    fssu.examstyle(File::Log).setForWrite();
    logfld_ = new uiFileSel( this,
			uiStrings::phrOutput(uiStrings::sLogFile()), fssu );
    logfld_->attach( alignedBelow, inpfld_ );

    fssu.formats( File::Format( tr("Script files"), "odcmd" ) )
	.confirmoverwrite(false).contenttype( OD::GeneralContent );
    outfld_ = new uiFileSel( this, uiStrings::phrOutput(commandfile), fssu );
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
					     uiString::empty() );

    drv_.interactRequest.notify( mCB(this,uiCmdDriverDlg,interactCB) );

    setDefaultSelDirs();
    setDefaultLogFile();
    const File::Path fp( drv_.outputDir(), drv_.logFileName() );
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
    logfld_->display( runmode );
    inpfld_->display( runmode );
    outfld_->display( !runmode );

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

    logfld_->setSensitive( idle );
    outfld_->setSensitive( idle );

    // inpfld_->enableExamine( true );
    // logfld_->enableExamine( true );
    // outfld_->enableExamine( true );
}


bool uiCmdDriverDlg::rejectOK()
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


static bool isRefToDataDir( uiFileSel& fld, bool base=false )
{
    File::Path fp( fld.fileName() );
    BufferString dir = base ? GetBaseDataDir() : GetDataDir();
    dir += File::Path::dirSep(File::Path::Local);
    return dir.isStartOf( fp.fullPath() );
}


static bool passSurveyCheck( uiFileSel& fld, bool& surveycheck )
{
    if ( !surveycheck ) return true;

    int res = 1;
    if ( isRefToDataDir(fld,true) && !isRefToDataDir(fld,false) )
    {
	uiString msg =
	    od_static_tr( "passSurveyCheck" ,
			  "%1 - path is referring to previous survey!" )
			.arg( fld.labelText() );
	res = gUiMsg(&fld).question( msg, uiStrings::sContinue(),
			uiStrings::sReset(), uiStrings::sCancel(),
			uiStrings::sWarning() );
	surveycheck = res<0;

	if ( !res )
	    fld.setFileName( "" );
    }
    return res>0;
}


void uiCmdDriverDlg::selectGoCB( CallBacker* )
{
    if ( !passSurveyCheck(*inpfld_, inpfldsurveycheck_) )
	return;

    if ( !passSurveyCheck(*logfld_, logfldsurveycheck_) )
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

    File::Path fp( logfld_->fileName() );
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
	uiMSG().error( drv_.errMsg() );
	return;
    }

    logfld_->setFileName( fp.fullPath() );
    inpfld_->setFileName( fnm );

    refreshDisplay( true, false );
    drv_.execute();
}


void uiCmdDriverDlg::selectPauseCB( CallBacker* )
{
    BufferString buttext = toString( pausebut_->text() );
    if ( buttext == toString(uiStrings::sResume())  )
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

    BufferString buttext = toString( pausebut_->text() );
    if ( buttext==toString(sInterrupting()) && ispec->dlgtitle_.isEmpty())
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
	interactdlg_->windowClosed.notify(
				mCB(this,uiCmdDriverDlg,interactClosedCB) );
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

    File::Path fp( outfld_->fileName() );
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
    BufferString dir = defaultscriptsdir_.isEmpty() ? GetCmdDriverScript(0)
						    : defaultscriptsdir_.buf();
    inpfld_->setup().initialselectiondir( dir );
    outfld_->setup().initialselectiondir( dir );

    dir = defaultlogdir_.isEmpty() ? GetProcFileName(0) : defaultlogdir_.buf();
    logfld_->setup().initialselectiondir( dir );
}


void uiCmdDriverDlg::setDefaultLogFile()
{
    const char* dir = defaultlogdir_.isEmpty() ? GetProcFileName(0)
					       : defaultlogdir_.buf();

    logproposal_ = File::Path(dir, CmdDriver::defaultLogFilename()).fullPath();
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

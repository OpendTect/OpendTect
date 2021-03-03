/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Sep 2009
________________________________________________________________________

-*/

#include "uicmddrivermgr.h"


#include "cmddriver.h"
#include "cmdrecorder.h"
#include "commandlineparser.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "dbman.h"
#include "keyboardevent.h"
#include "keyenum.h"
#include "oddirs.h"
#include "settings.h"
#include "sighndl.h"
#include "timer.h"

#include "uicmddriverdlg.h"
#include "uimain.h"
#include "uimenu.h"
#include "uimsg.h"



namespace CmdDrive
{


static const char* autoexecfnm = "autoexec.odcmd";


uiCmdDriverMgr::uiCmdDriverMgr( bool fullodmode )
	: applwin_(*uiMain::theMain().topLevel())
	, settingsautoexec_(fullodmode)
	, surveyautoexec_(fullodmode)
	, scriptidx_(-3)
	, cmdlineparsing_(fullodmode)
	, defaultscriptsdir_(fullodmode ? "" : GetPersonalDir())
	, defaultlogdir_(fullodmode ? "" : GetPersonalDir())
{
    tim_ = new Timer();
    rec_ = new CmdRecorder( applwin_ );
    drv_ = new CmdDriver( applwin_ );

    commandLineParsing();

    mAttachCB( DBM().surveyToBeChanged, uiCmdDriverMgr::beforeSurveyChg );
    mAttachCB( DBM().afterSurveyChange, uiCmdDriverMgr::afterSurveyChg );
    mAttachCB( DBM().applicationClosing, uiCmdDriverMgr::stopRecordingCB );
    mAttachCB( applwin_.runScriptRequest, uiCmdDriverMgr::runScriptCB );
    mAttachCB( uiMain::keyboardEventHandler().keyPressed,
	       uiCmdDriverMgr::keyPressedCB );
    mAttachCB( drv_->executeFinished, uiCmdDriverMgr::executeFinishedCB );

    if ( !applwin_.finalised() )
    {
	mAttachCB( applwin_.postFinalise(), uiCmdDriverMgr::delayedStartCB );
	mAttachCB( tim_->tick, uiCmdDriverMgr::timerCB );
    }
}


uiCmdDriverMgr::~uiCmdDriverMgr()
{
    detachAllNotifiers();

    delete tim_;
    delete rec_;
    delete drv_;

    if ( historec_ )
    {
	delete historec_;
	SignalHandling::stopNotify( SignalHandling::Kill,
				    mCB(this,uiCmdDriverMgr,stopRecordingCB) );
    }
}


uiCmdDriverMgr& uiCmdDriverMgr::getMgr( bool fullodmode )
{
    mDefineStaticLocalObject( PtrMan<CmdDrive::uiCmdDriverMgr>, cmdmmgr,
				= new CmdDrive::uiCmdDriverMgr(fullodmode) )
    return *(cmdmmgr.ptr());
}


uiCmdDriverDlg* uiCmdDriverMgr::getCmdDlg()
{
    if ( !cmddlg_ )
    {
	initCmdLog( cmdlogname_ );
	cmddlg_ = new uiCmdDriverDlg( &applwin_, *drv_, *rec_,
				      defaultscriptsdir_, defaultlogdir_ );
	cmddlg_->go();
    }

    return cmddlg_;
}


void uiCmdDriverMgr::keyPressedCB( CallBacker* )
{
    if ( !uiMain::keyboardEventHandler().hasEvent() )
	return;

    const KeyboardEvent& kbe = uiMain::keyboardEventHandler().event();
    const OD::ButtonState bs =
			  OD::ButtonState( kbe.modifier_ & OD::KeyButtonMask );

    if ( bs==OD::ControlButton && kbe.key_==OD::KB_R && !kbe.isrepeat_ )
    {
	uiMain::keyboardEventHandler().setHandled( true );
	showDlgCB( 0 );
    }
}


void uiCmdDriverMgr::showDlgCB( CallBacker* )
{
    getCmdDlg()->popUp();
}


void uiCmdDriverMgr::enableCmdLineParsing( bool yn )
{
    cmdlineparsing_ = yn;
}


void uiCmdDriverMgr::setDefaultScriptsDir( const char* dirnm )
{
    if ( File::isDirectory(dirnm) )
	defaultscriptsdir_ = dirnm;
}


void uiCmdDriverMgr::setDefaultLogDir( const char* dirnm )
{
    if ( File::isDirectory(dirnm) )
	defaultlogdir_ = dirnm;
}


void uiCmdDriverMgr::setLogFileName( const char* fnm )
{
    cmdlogname_ = fnm;
}


#define mCheckFilePath( fnm, defaultdir, altdir ) \
{ \
    File::Path fp( fnm ); \
    if ( !File::exists(fnm) && !fp.isAbsolute() ) \
    { \
	fp.setPath( defaultdir.isEmpty() ? altdir : defaultdir.buf() ); \
	fnm = fp.fullPath(); \
    } \
}

void uiCmdDriverMgr::addCmdLineScript( const char* fnm )
{
    BufferString filenm( fnm );
    mCheckFilePath( filenm, defaultscriptsdir_, GetCmdDriverScript(0) );

    if ( !File::exists(filenm) )
    {
	File::Path fp( filenm );
	fp.setExtension( "odcmd", true );
	filenm = fp.fullPath();
    }

    if ( File::exists(filenm) )
	cmdlinescripts_.add( filenm );
}


void uiCmdDriverMgr::commandLineParsing()
{
    if ( !cmdlineparsing_ )
	return;

    const CommandLineParser clp;
    cmdlogname_ = clp.keyedString( "cmdlog" );
    const BufferStringSet cmdscripts = clp.keyedList( "cmd" );
    for ( auto cmdscript : cmdscripts )
	addCmdLineScript( cmdscript->str() );

    const bool noautoexec = clp.hasKey( "noautoexec" );
    if ( noautoexec || clp.hasKey("nosettingsautoexec") )
	settingsautoexec_ = false;
    if ( noautoexec || clp.hasKey("nosurveyautoexec") )
	surveyautoexec_ = false;
}


void uiCmdDriverMgr::initCmdLog( const char* cmdlognm )
{
    BufferString fnm( cmdlognm );
    if ( fnm.isEmpty() )
	fnm = drv_->defaultLogFilename();

    mCheckFilePath( fnm, defaultlogdir_, GetProcFileName(0) );

    const File::Path fp( fnm );
    if ( File::isWritable(fp.pathOnly()) &&
	 (!File::exists(fp.fullPath()) || File::isWritable(fp.fullPath())) )
    {
	drv_->setOutputDir( fp.pathOnly() );
	drv_->setLogFileName( fp.fileName() );
    }
    else if ( *cmdlognm )
	initCmdLog( "" );
}


void uiCmdDriverMgr::delayedStartCB( CallBacker* )
{
    tim_->start( 500, true );
}


void uiCmdDriverMgr::timerCB( CallBacker* )
{
    int bufsize = 0;
    mSettUse( get, "dTect", "User history buffer", bufsize );
    if ( bufsize )
    {
	historec_ = new CmdRecorder( applwin_ );
	historec_->setOutputFile( GetProcFileName("userhistory.odcmd") );
	historec_->writeTailOnly( bufsize<0 );
	historec_->setBufferSize( abs(bufsize) );
	historec_->ignoreCmdDriverWindows( false );

	if ( !GetOSEnvVar("DTECT_HANDLE_FATAL") )
	{
	    SetEnvVar( "DTECT_HANDLE_FATAL", "1" );
	    SignalHandling::initFatalSignalHandling();
	}
	SignalHandling::startNotify( SignalHandling::Kill,
				     mCB(this,uiCmdDriverMgr,stopRecordingCB) );
	historec_->start();
    }

    autoStart();
}


void uiCmdDriverMgr::executeFinishedCB( CallBacker* )
{
    if ( cmddlg_ )
	cmddlg_->executeFinished();

    if ( historec_ && historec_->isRecording() )
	historec_->updateCmdComposers();

    autoStart();
}


void uiCmdDriverMgr::autoStart()
{
    if ( scriptidx_ >= cmdlinescripts_.size() )
	return;

    scriptidx_++;

    if ( scriptidx_ == -2 )
	handleSettingsAutoExec();
    else if ( scriptidx_ == -1 )
	afterSurveyChg( 0 );
    else if ( scriptidx_ < cmdlinescripts_.size() )
	getCmdDlg()->autoStartGo( cmdlinescripts_.get(scriptidx_) );
    else if ( cmddlg_ )
	cmddlg_->reject();
}


void uiCmdDriverMgr::handleSettingsAutoExec()
{
    BufferString fnm( File::Path(GetSettingsDir(),autoexecfnm).fullPath() );
    if ( settingsautoexec_ && File::exists(fnm) )
	getCmdDlg()->autoStartGo( fnm );
    else
	autoStart();
}


void uiCmdDriverMgr::beforeSurveyChg( CallBacker* cb )
{
    if ( !applwin_.finalised() )
	return;
    if ( cmddlg_ )
	cmddlg_->beforeSurveyChg();
}


void uiCmdDriverMgr::afterSurveyChg( CallBacker* cb )
{
    if ( !applwin_.finalised() )
	return;

    if ( cmddlg_ )
	cmddlg_->afterSurveyChg();

    const BufferString fnm( GetProcFileName(autoexecfnm) );
    if ( surveyautoexec_ && File::exists(fnm) )
    {
	if ( cb && !drv_->nowExecuting() )
	{
	    if ( !cmddlg_ || cmddlg_->isHidden() )
		scriptidx_--;		// Hide afterwards if not shown now.

	    getCmdDlg()->popUp();
	}

	getCmdDlg()->autoStartGo( fnm );

	if ( scriptidx_ == -2 )		// If called from settings-autoexec,
	    scriptidx_++;		// do not run survey-autoexec again.
    }
    else
	autoStart();
}


void uiCmdDriverMgr::stopRecordingCB( CallBacker* cb )
{
    if ( rec_ )
	rec_->stop( cb );
    if ( historec_ )
	historec_->stop( cb );
}


void uiCmdDriverMgr::runScriptCB( CallBacker* )
{
    if ( !File::exists(applwin_.getScriptToRun()) )
	return;

    if ( !drv_->nowExecuting() )
    {
	if ( !cmddlg_ || cmddlg_->isHidden() )
	    scriptidx_--;		// Hide afterwards if not shown now.

	getCmdDlg()->popUp();
    }

    getCmdDlg()->autoStartGo( applwin_.getScriptToRun() );
}


}; // namespace CmdDrive

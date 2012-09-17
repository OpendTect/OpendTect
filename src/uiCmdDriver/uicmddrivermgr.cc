/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uicmddrivermgr.cc,v 1.1 2012-09-17 12:37:42 cvsjaap Exp $";

#include "uicmddrivermgr.h"

#include "cmdrecorder.h"
#include "envvars.h"
#include "filepath.h"
#include "ioman.h"
#include "keyboardevent.h"
#include "keyenum.h"
#include "oddirs.h"
#include "settings.h"

#include "uimain.h"
#include "uimenu.h"
#include "uimsg.h"

#include "cmddriver.h"
#include "cmdrecorder.h"
#include "uicmddriverdlg.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "oddirs.h"
#include "timer.h"
#include "sighndl.h" 

namespace CmdDrive
{


static const char* autoexecfnm = "autoexec.odcmd";


uiCmdDriverMgr::uiCmdDriverMgr( uiMainWin& aw )
    	: applwin_(aw)
	, cmddlg_(0)
	, settingsautoexec_(true)
	, surveyautoexec_(true)
	, scriptidx_(-3)
        , historec_(0)
{
    tim_ = new Timer();
    rec_ = new CmdRecorder( aw );
    drv_ = new CmdDriver( aw );

    commandLineParsing();

    IOM().surveyToBeChanged.notify( mCB(this,uiCmdDriverMgr,beforeSurveyChg) );
    IOM().afterSurveyChange.notify( mCB(this,uiCmdDriverMgr,afterSurveyChg) );
    applwin_.windowClosed.notify( mCB(this,uiCmdDriverMgr,closeDlg) );
    uiMain::keyboardEventHandler().keyPressed.notify(
				mCB(this,uiCmdDriverMgr,keyPressedCB) );
    drv_->executeFinished.notify( mCB(this,uiCmdDriverMgr,executeFinishedCB) );

    if ( !applwin_.finalised() )
    {
	applwin_.postFinalise().notify( mCB(this,uiCmdDriverMgr,delayedStartCB) );
	tim_->tick.notify( mCB(this,uiCmdDriverMgr,timerCB) );
    }
}


uiCmdDriverMgr::~uiCmdDriverMgr()
{
    closeDlg(0);
    stopRecordingCB(0);

    IOM().surveyToBeChanged.remove( mCB(this,uiCmdDriverMgr,beforeSurveyChg) );
    IOM().afterSurveyChange.remove( mCB(this,uiCmdDriverMgr,afterSurveyChg) );
    applwin_.windowClosed.remove( mCB(this,uiCmdDriverMgr,closeDlg) );
    uiMain::keyboardEventHandler().keyPressed.remove(
				mCB(this,uiCmdDriverMgr,keyPressedCB) );

    applwin_.postFinalise().remove( mCB(this,uiCmdDriverMgr,delayedStartCB) );

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


uiCmdDriverDlg* uiCmdDriverMgr::getCmdDlg()
{
    if ( !cmddlg_ )
    {
	initCmdLog( cmdlogname_ );
	cmddlg_ = new uiCmdDriverDlg( &applwin_, *drv_, *rec_ );
	cmddlg_->go();
    }

    return cmddlg_;
}


void uiCmdDriverMgr::closeDlg( CallBacker* )
{
    if ( cmddlg_ )
    {
	delete cmddlg_;
	cmddlg_ = 0;
    }
}


void uiCmdDriverMgr::keyPressedCB( CallBacker* )
{
    const KeyboardEvent& kbe = uiMain::keyboardEventHandler().event();
    const OD::ButtonState bs =
			  OD::ButtonState( kbe.modifier_ & OD::KeyButtonMask );

    if ( bs==OD::ControlButton && kbe.key_==OD::R && !kbe.isrepeat_ )
    {
	uiMain::keyboardEventHandler().setHandled( true );
	showDlgCB( 0 );
    }
}


void uiCmdDriverMgr::showDlgCB( CallBacker* )
{
    getCmdDlg()->popUp();
}


void uiCmdDriverMgr::commandLineParsing()
{
    BufferStringSet cmdline;
    uiMain::theMain().getCmdLineArgs( cmdline );
    for ( int idx=1; idx<cmdline.size(); idx++ )
    {
	char* str = cmdline.get(idx).buf();
	char* ptr = strchr( str, '=' );
	if ( !ptr )
	    continue;

	*ptr++ = '\0';
	if ( !strcmp(str,"cmd") )
	{

	    BufferString fnm( ptr );
	    if ( fnm == "noautoexec" )
	    {
		settingsautoexec_ = false;
		surveyautoexec_ = false;
		continue;
	    }
	    if ( fnm == "nosettingsautoexec" )
	    {
		settingsautoexec_ = false;
		continue;
	    }
	    if ( fnm == "nosurveyautoexec" )
	    {
		surveyautoexec_ = false;
		continue;
	    }

	    if ( !File::exists(fnm) && !FilePath(fnm).isAbsolute() )
		fnm = GetScriptsDir( fnm );
	    if ( !File::exists(fnm) )
	    {
		FilePath fp( fnm );
		fp.setExtension( "odcmd", true );
		fnm = fp.fullPath();
	    }
	    if ( File::exists(fnm) )
		cmdlinescripts_.add( fnm );
	}

	if ( !strcmp(str,"cmdlog") )
	    cmdlogname_ = ptr;
    }
}


void uiCmdDriverMgr::initCmdLog( const char* cmdlognm )
{
    BufferString fnm( cmdlognm );
    if ( fnm.isEmpty() )
	fnm = drv_->defaultLogFilename();

    if ( !File::exists(fnm) && !FilePath(fnm).isAbsolute() )
	fnm = GetProcFileName( fnm );

    FilePath fp( fnm );
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
    BufferString fnm( FilePath(GetSettingsDir(),autoexecfnm).fullPath() );
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


}; // namespace CmdDrive

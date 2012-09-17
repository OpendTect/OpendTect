/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: cmddrivermgr.cc,v 1.21 2011/12/14 13:16:41 cvsbert Exp $";

#include "cmddrivermgr.h"

#include "cmdrecorder.h"
#include "envvars.h"
#include "filepath.h"
#include "ioman.h"
#include "keyboardevent.h"
#include "keyenum.h"
#include "oddirs.h"

#include "uimain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"

#include "cmddriver.h"
#include "cmddriverdlg.h"
#include "cmdrecorder.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "oddirs.h"
#include "timer.h"


namespace CmdDrive
{


static const char* autoexecfnm = "autoexec.odcmd";


// Remove in OD 4.4
static const char* oldautoexecfnm = "autoexec.cmd";
#define mOldAutoExecExtWarn( oldfullpath ) \
    uiMSG().warning( oldfullpath, \
		    " must have the new extension '.odcmd' to run at startup" );


uiCmdDriverMgr::uiCmdDriverMgr( uiODMain& a )
    	: appl_(a)
	, cmddlg_(0)
	, settingsautoexec_(true)
	, surveyautoexec_(true)
	, scriptidx_(-3)
{
    uiODMenuMgr& mnumgr = appl_.menuMgr();
  
    cmdmnuitm_ = new uiMenuItem( "Command &Driver ..." );
    mnumgr.toolsMnu()->insertItem( cmdmnuitm_ );
    cmdmnuitm_->activated.notify( mCB(this,uiCmdDriverMgr,dlgCB) );

    tim_ = new Timer();
    rec_ = new CmdRecorder();
    drv_ = new CmdDriver();

    commandLineParsing();

    IOM().surveyToBeChanged.notify( mCB(this,uiCmdDriverMgr,beforeSurveyChg) );
    IOM().afterSurveyChange.notify( mCB(this,uiCmdDriverMgr,afterSurveyChg) );
    appl_.windowClosed.notify( mCB(this,uiCmdDriverMgr,closeDlg) );
    uiMain::keyboardEventHandler().keyPressed.notify(
				mCB(this,uiCmdDriverMgr,keyPressedCB) );
    drv_->executeFinished.notify( mCB(this,uiCmdDriverMgr,executeFinishedCB) );

    if ( !appl_.finalised() )
    {
	appl_.postFinalise().notify( mCB(this,uiCmdDriverMgr,delayedStartCB) );
	tim_->tick.notify( mCB(this,uiCmdDriverMgr,autoStartCB) );
    }
}


uiCmdDriverMgr::~uiCmdDriverMgr()
{
    closeDlg(0);

    IOM().surveyToBeChanged.remove( mCB(this,uiCmdDriverMgr,beforeSurveyChg) );
    IOM().afterSurveyChange.remove( mCB(this,uiCmdDriverMgr,afterSurveyChg) );
    appl_.windowClosed.remove( mCB(this,uiCmdDriverMgr,closeDlg) );
    uiMain::keyboardEventHandler().keyPressed.remove(
				mCB(this,uiCmdDriverMgr,keyPressedCB) );

    appl_.postFinalise().remove( mCB(this,uiCmdDriverMgr,delayedStartCB) );

    delete tim_;
    delete rec_;
    delete drv_;
}


uiCmdDriverDlg* uiCmdDriverMgr::getCmdDlg()
{
    if ( !cmddlg_ )
    {
	initCmdLog( cmdlogname_ );
	cmddlg_ = new uiCmdDriverDlg( &appl_, *drv_, *rec_ );
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
	dlgCB( 0 );
    }
}


void uiCmdDriverMgr::dlgCB( CallBacker* )
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


void uiCmdDriverMgr::executeFinishedCB( CallBacker* )
{
    if ( cmddlg_ )
	cmddlg_->executeFinished();

    autoStartCB( 0 );
}


void uiCmdDriverMgr::autoStartCB( CallBacker* )
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
    if ( File::exists(fnm) )
    {
	if ( settingsautoexec_ )
	    { getCmdDlg()->autoStartGo( fnm ); return; }
    }
    else
    {
	// Remove in OD4.4
	fnm = FilePath( GetSettingsDir(), oldautoexecfnm ).fullPath();
	if ( File::exists(fnm) )
	    mOldAutoExecExtWarn( fnm )
    }

    autoStartCB( 0 );
}


void uiCmdDriverMgr::beforeSurveyChg( CallBacker* cb )
{
    if ( !appl_.finalised() )
	return;
    if ( cmddlg_ )
	cmddlg_->beforeSurveyChg();
}


void uiCmdDriverMgr::afterSurveyChg( CallBacker* cb )
{
    if ( !appl_.finalised() )
	return;

    if ( cmddlg_ )
	cmddlg_->afterSurveyChg();

    const BufferString fnm( GetProcFileName(autoexecfnm) );
    if ( File::exists(fnm) )
    {
	if ( surveyautoexec_ )
	{
	    if ( cb && !drv_->nowExecuting() )
	    {
		if ( !cmddlg_ || cmddlg_->isHidden() )
		    scriptidx_--;	// Hide afterwards if not shown now.

		getCmdDlg()->popUp();
	    }

	    getCmdDlg()->autoStartGo( fnm );

	    if ( scriptidx_ == -2 )	// If called from settings-autoexec,
		scriptidx_++;		// do not run survey-autoexec again.

	    return;
	}
    }
    else
    {
	// Remove in OD4.4
	const BufferString oldfnm( GetProcFileName(oldautoexecfnm) );
	if ( File::exists(oldfnm) )
	    mOldAutoExecExtWarn( oldfnm );
    }

    autoStartCB( 0 );
}


}; // namespace CmdDrive

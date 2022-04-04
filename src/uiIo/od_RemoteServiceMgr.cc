/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Mar 2022
________________________________________________________________________

-*/

#include "applicationdata.h"
#include "file.h"
#include "filepath.h"
#include "moddepmgr.h"
#include "netservice.h"
#include "networkcommon.h"
#include "prog.h"
#include "oddirs.h"
#include "oscommand.h"
#include "remjobexec.h"
#include "sighndl.h"
#include "systeminfo.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uimain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uitraydialog.h"
#include "uistrings.h"
#include "uitextfile.h"


class uiRemoteServiceMgr : public uiTrayDialog
{ mODTextTranslationClass(uiRemoteServiceMgr)
public:
			uiRemoteServiceMgr(uiParent*);
			~uiRemoteServiceMgr();

protected:
    uiTextFile*		textbox_;
    uiToolBar*		tb_;
    uiPushButton*	startbut_;
    uiPushButton*	stopbut_;
    uiPushButton*	reloadlogbut_;

    Network::Service	nwservice_;

    void		startCB(CallBacker*);
    void		stopCB(CallBacker*);
    void		reloadlogCB(CallBacker*);
    void		updateCB(CallBacker*);
    void		exitCB(CallBacker*) override;
    bool		rejectOK(CallBacker*) override;

    BufferString	getLogFileName() const;
    void		addApplicationTrayMenuItems(uiMenu*) override;
};


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain app( argc, argv );
    OD::ModDeps().ensureLoaded( "uiIo" );
    ApplicationData::setApplicationName( "OpendTect RemoteServiceMgr" );
    PtrMan<uiDialog> mw = new uiRemoteServiceMgr( nullptr );
    app.setTopLevel( mw );
    mw->show();

    return app.exec();
}


uiRemoteServiceMgr::uiRemoteServiceMgr( uiParent* p )
    : uiTrayDialog(p,Setup(tr("Remote Service Manager"), uiString::empty(),
			   mNoHelpKey))
    , nwservice_(0)
{
    setCtrlStyle( CloseOnly );
    setTrayToolTip( tr("OpendTect Remote Service Manager") );


    auto* buttons =  new uiButtonGroup( this, "buttons", OD::Horizontal );
    startbut_ = new uiPushButton( buttons, uiStrings::sStart(),
				  mCB(this,uiRemoteServiceMgr,startCB), false );
    stopbut_ = new uiPushButton( buttons, uiStrings::sStop(),
				 mCB(this,uiRemoteServiceMgr,stopCB), false );
    reloadlogbut_ = new uiPushButton( buttons, uiStrings::sReload(),
			     mCB(this,uiRemoteServiceMgr,reloadlogCB), false );


    File::ViewPars su( File::Log );
    textbox_ = new uiTextFile( this, nullptr, su );
    textbox_->uiObj()->attach( centeredBelow, buttons );

    nwservice_.setPID( 0 );
    updateCB( nullptr );
}


uiRemoteServiceMgr::~uiRemoteServiceMgr()
{
    detachAllNotifiers();
}


void uiRemoteServiceMgr::startCB( CallBacker* )
{
    if ( nwservice_.isAlive() )
    {
	uiMSG().error( tr("Remote service is already running") );
	return;
    }

    uiString errmsg;
    const PortNr_Type rsport = RemoteJobExec::remoteHandlerPort();
    if ( !Network::isPortFree(rsport, &errmsg) )
    {
	uiMSG().errorWithDetails( uiStringSet(errmsg),
				    tr("Port %1 required by the OpendTect"
				     " multi-machine processing system is "
				     "already in use").arg(rsport) );
	return;
    }

    const char* rsexecnm = RemoteJobExec::remoteHandlerName();
    OS::MachineCommand mc( FilePath(GetExecPlfDir(), rsexecnm).fullPath() );
    const OS::CommandExecPars pars( OS::RunInBG );
    OS::CommandLauncher cl( mc );
    if ( cl.execute(pars) )
    {
	nwservice_.setPID( cl );
	nwservice_.setPort( rsport );
	nwservice_.setType( Network::Service::ODBatch );
	nwservice_.setLogFile( getLogFileName() );
	nwservice_.setHostName( System::localAddress() );
	nwservice_.setName( rsexecnm );

	textbox_->open( getLogFileName() );

    }
    else
	uiMSG().errorWithDetails( uiStringSet(cl.errorMsg(),
				  tr("Unable to start %1").arg(rsexecnm)) );

    updateCB( nullptr );
}


void uiRemoteServiceMgr::stopCB( CallBacker* )
{
    if ( !nwservice_.isAlive() )
    {
	uiMSG().error( tr("Remote service is not running") );
	return;
    }

    nwservice_.stop( false );

    updateCB( nullptr );
}


void uiRemoteServiceMgr::reloadlogCB( CallBacker* )
{
    if ( nwservice_.isAlive() )
	textbox_->reLoad();
}


void uiRemoteServiceMgr::updateCB( CallBacker*)
{
    const bool isalive = nwservice_.isAlive();
    startbut_->setSensitive( !isalive );
    stopbut_->setSensitive( isalive );
    reloadlogbut_->setSensitive( isalive );
    if ( isalive )
	setTrayIcon("reload");
    else
	setTrayIcon("stop");
}


void uiRemoteServiceMgr::exitCB( CallBacker* )
{
    showCB( nullptr );
    if ( nwservice_.isAlive() )
    {
	if ( uiMSG().askGoOn(tr("Confirm Exit of %1").arg(caption())) )
	    stopCB( nullptr );
	else
	    return;
    }

    setInTray( false );
    close();
}


bool uiRemoteServiceMgr::rejectOK( CallBacker* )
{
    return true;
}


BufferString uiRemoteServiceMgr::getLogFileName() const
{
    FilePath logfp( GetBaseDataDir(), "LogFiles" );
    BufferString lhname = System::localAddress();
    lhname.replace( '.',  '_' );
    logfp.add( lhname );
    logfp.setExtension( ".log" );
    return logfp.fullPath();
}


void uiRemoteServiceMgr::addApplicationTrayMenuItems( uiMenu* mnu )
{
    mnu->insertAction( new uiAction(uiStrings::sStart(),
				    mCB(this, uiRemoteServiceMgr, startCB)) );
    mnu->insertAction( new uiAction(uiStrings::sStop(),
				    mCB(this, uiRemoteServiceMgr, stopCB)) );
}

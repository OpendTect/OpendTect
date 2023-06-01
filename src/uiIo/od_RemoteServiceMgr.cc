/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "applicationdata.h"
#include "commandlineparser.h"
#include "file.h"
#include "filepath.h"
#include "filesystemwatcher.h"
#include "ioman.h"
#include "mmpserverclient.h"
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
#include "uidatarootsel.h"
#include "uimain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uitraydialog.h"
#include "uistrings.h"
#include "uitextfile.h"



class uiRemoteServiceMgr : public uiTrayDialog
{ mODTextTranslationClass(uiRemoteServiceMgr)
public:
			uiRemoteServiceMgr(uiParent*);
			~uiRemoteServiceMgr();

    void		processCommandLine();

    static const char*	sAutostart()		{ return "autostart"; }
    static const char*	sMinimize()		{ return "minimize"; }
    static BufferString getHelp();

protected:
    uiDataRootSel*	datarootsel_;
    uiTextFile*		textbox_;
    uiLineEdit*		datarootlbl_;
    uiToolBar*		tb_;
    uiPushButton*	startbut_;
    uiPushButton*	stopbut_;
    uiPushButton*	reloadlogbut_;

    MMPServerClient	mmpclient_;
    FileSystemWatcher*	logfsw_		= nullptr;
    BufferString	logfilewatched_;

    void		startCB(CallBacker*);
    void		stopCB(CallBacker*);
    void		reloadlogCB(CallBacker*);
    void		showErrorCB(CallBacker*);
    void		logFileChgCB(CallBacker*);
    void		logFileUpdCB(CallBacker*);
    void		changeDataRootCB(CallBacker*);
    void		updateCB(CallBacker*);
    void		exitCB(CallBacker*) override;
    bool		rejectOK(CallBacker*) override;

    BufferString	getLogFileName() const;
    void		addApplicationTrayMenuItems(uiMenu*) override;
};


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv );
    uiMain app( argc, argv );
    app.setIcon( "rsm" );
    ApplicationData::setApplicationName( "OpendTect RemoteServiceMgr" );

    OD::ModDeps().ensureLoaded( "uiTools" );

    const CommandLineParser clp( argc, argv );
    if ( clp.hasKey("help") )
    {
	od_cout() << uiRemoteServiceMgr::getHelp() << od_endl;
	return 0;
    }

    const uiRetVal uirv = IOMan::setDataSource( clp );
    if ( !uirv.isOK() )
	return 1;

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiIo" );
    PtrMan<uiRemoteServiceMgr> topdlg = new uiRemoteServiceMgr( nullptr );
    app.setTopLevel( topdlg );
    PIM().loadAuto( true );
    topdlg->show();
    topdlg->processCommandLine();

    return app.exec();
}


uiRemoteServiceMgr::uiRemoteServiceMgr( uiParent* p )
    : uiTrayDialog(p,Setup(tr("Remote Service Manager"), uiString::empty(),
			   mNoHelpKey))
    , mmpclient_(RemoteJobExec::remoteHandlerPort())
{
    setCtrlStyle( CloseOnly );
    setTrayToolTip( tr("OpendTect Remote Service Manager") );

    datarootsel_ = new uiDataRootSel( this );
    auto* buttons =  new uiButtonGroup( this, "buttons", OD::Horizontal );
    startbut_ = new uiPushButton( buttons, uiStrings::sStart(),
				  mCB(this,uiRemoteServiceMgr,startCB), false );
    stopbut_ = new uiPushButton( buttons, uiStrings::sStop(),
				 mCB(this,uiRemoteServiceMgr,stopCB), false );
    reloadlogbut_ = new uiPushButton( buttons, uiStrings::sReload(),
			     mCB(this,uiRemoteServiceMgr,reloadlogCB), false );
    buttons->attach( centeredBelow, datarootsel_ );

    File::ViewPars su( File::Log );
    textbox_ = new uiTextFile( this, nullptr, su );
    textbox_->uiObj()->attach( centeredBelow, buttons );

    mAttachCB( mmpclient_.errorNotice, uiRemoteServiceMgr::showErrorCB );
    mAttachCB( mmpclient_.logFileChg, uiRemoteServiceMgr::logFileChgCB );
    mAttachCB( datarootsel_->selectionChanged,
	       uiRemoteServiceMgr::changeDataRootCB );
    logFileChgCB( nullptr );
    updateCB( nullptr );
}


uiRemoteServiceMgr::~uiRemoteServiceMgr()
{
    detachAllNotifiers();
    delete logfsw_;
}


void uiRemoteServiceMgr::startCB( CallBacker* )
{
    if ( mmpclient_.serverService().isAlive() )
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
    mc.addKeyedArg( CommandLineParser::sDataRootArg(), GetBaseDataDir() );
    const OS::CommandExecPars pars( OS::RunInBG );
    OS::CommandLauncher cl( mc );
    if ( cl.execute(pars) )
    {
	sleepSeconds( 1 );
	mmpclient_.refresh();
	logFileChgCB( nullptr );
    }
    else
	uiMSG().errorWithDetails( uiStringSet(cl.errorMsg(),
				  tr("Unable to start %1").arg(rsexecnm)) );

    updateCB( nullptr );
}


void uiRemoteServiceMgr::stopCB( CallBacker* )
{
    if ( !mmpclient_.serverService().isAlive() )
    {
	uiMSG().error( tr("Remote service is not running") );
	return;
    }

    mmpclient_.stopServer( true );
    if ( logfsw_ )
	mDetachCB( logfsw_->fileChanged, uiRemoteServiceMgr::logFileUpdCB );

    deleteAndNullPtr( logfsw_ );
    logfilewatched_.setEmpty();
    updateCB( nullptr );
    textbox_->open( nullptr );
}


void uiRemoteServiceMgr::reloadlogCB( CallBacker* )
{
    if ( mmpclient_.serverService().isAlive() )
	textbox_->reLoad();
}


void uiRemoteServiceMgr::showErrorCB( CallBacker* )
{
    uiMSG().errorWithDetails( mmpclient_.errMsg().messages() );
}


void uiRemoteServiceMgr::logFileChgCB( CallBacker* )
{
    const BufferString logfnm( mmpclient_.serverService().logFnm() );
    textbox_->open( logfnm.buf() );
    if ( !File::exists(logfnm.buf()) || logfnm == logfilewatched_ )
	return;

    delete logfsw_;
    logfsw_ = new FileSystemWatcher();
    logfilewatched_ = logfnm.buf();
    logfsw_->addFile( logfilewatched_.buf() );
    mAttachCB( logfsw_->fileChanged, uiRemoteServiceMgr::logFileUpdCB );
}


void uiRemoteServiceMgr::logFileUpdCB( CallBacker* )
{
    const BufferString logfnm( mmpclient_.serverService().logFnm() );
    if ( File::exists(logfnm.buf()) )
	return;

    FilePath olddrfp( logfnm );
    olddrfp.setFileName( nullptr ).setFileName( nullptr );
    const BufferString olddr = olddrfp.fullPath();
    mmpclient_.refresh();
    const BufferString newdr = mmpclient_.serverDataRoot();
    if ( newdr == olddr || !File::isDirectory(newdr) )
	return;

    if ( logfsw_ )
	mDetachCB( logfsw_->fileChanged, uiRemoteServiceMgr::logFileUpdCB );

    delete logfsw_;
    logfsw_ = new FileSystemWatcher();
    logfilewatched_ = mmpclient_.serverService().logFnm();
    logfsw_->addFile( logfilewatched_.buf() );
    mAttachCB( logfsw_->fileChanged, uiRemoteServiceMgr::logFileUpdCB );

    NotifyStopper ns( datarootsel_->selectionChanged );
    datarootsel_->setDataRoot( newdr.buf() );
    logFileChgCB( nullptr );
}


void uiRemoteServiceMgr::changeDataRootCB( CallBacker* )
{
    if ( !mmpclient_.serverService().isAlive() )
	return;

    mmpclient_.setServerDataRoot( datarootsel_->getDataRoot() );
    if ( logfsw_ )
	mDetachCB( logfsw_->fileChanged, uiRemoteServiceMgr::logFileUpdCB );

    delete logfsw_;
    logfsw_ = new FileSystemWatcher();
    logfilewatched_ = mmpclient_.serverService().logFnm();
    logfsw_->addFile( logfilewatched_.buf() );
    mAttachCB( logfsw_->fileChanged, uiRemoteServiceMgr::logFileUpdCB );
}


void uiRemoteServiceMgr::updateCB( CallBacker*)
{
    const bool isalive = mmpclient_.serverService().isAlive();
    startbut_->setSensitive( !isalive );
    stopbut_->setSensitive( isalive );
    reloadlogbut_->setSensitive( isalive );
    if ( isalive )
	setTrayIcon("tray-rsm-active");
    else
	setTrayIcon("tray-rsm-inactive");
}


void uiRemoteServiceMgr::exitCB( CallBacker* )
{
    showCB( nullptr );
    if ( mmpclient_.serverService().isAlive() )
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


void uiRemoteServiceMgr::processCommandLine()
{
    CommandLineParser clp;
    if ( clp.hasKey(sAutostart()) )
	startCB( nullptr );

    if ( clp.hasKey(sMinimize()) )
	close();

    updateCB( nullptr );
}


BufferString uiRemoteServiceMgr::getHelp()
{
    BufferString msg( "Usage: ", GetExecutableName(), "\n" );
    msg.add("\t--").add(CommandLineParser::sDataRootArg()).
				add("\t'data_root_dir' (optional)\n");
    msg.add("\t--").add(uiRemoteServiceMgr::sAutostart()).
				add("\tautostart remote service (optional)\n");
    msg.add("\t--").add(uiRemoteServiceMgr::sMinimize()).
				add("\thide manager at start (optional)\n");
    return msg;
}

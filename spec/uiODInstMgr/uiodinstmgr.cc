	/*+
	________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uiodinstmgr.cc 7996 2013-06-13 12:13:14Z ranojay.sen@dgbes.com $";

#include "uiodinstmgr.h"
#include "uiodinstpkgmgr.h"
#include "odinstdlhandler.h"
#include "odinstlogger.h"
#include "odinstappdata.h"
#include "odinstpkgprops.h"
#include "odinstpkgselmgr.h"
#include "odinst.h"
#include "odinstwinutils.h"


#include "settings.h"
#include "file.h"
#include "filepath.h"
#include "databuf.h"
#include "datainpspec.h"
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "oddirs.h"
#include "strmdata.h"
#include "strmoper.h"
#include "strmprov.h"
#include "timer.h"
#include "ascstream.h"
#include "fixedstreambuf.h"
#include "ziputils.h"

#include "uicombobox.h"
#include "uichecklist.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uiproxydlg.h"
#include "uihandledlsitefail.h"
#include "uiselsimple.h"
#include "uitaskrunner.h"



#ifdef __win__
#include "winutils.h"
#endif

static const char* sKeyBaseDir = "Base directory";
static const char* getTitleWithVersion()
{
    static BufferString title( "OpendTect Installation Manager V" );
    title += ODInst::getPkgVersion( "instmgr" );
    return title.buf();
}


uiODInstMgr::uiODInstMgr( uiParent* p, ODInst::AppData& a, const char* sd,
       			 const char* fs )
    : uiODInstDlg(p,uiDialog::Setup(getTitleWithVersion(),
			     " ","0.5.1").nrstatusflds(1),0)
    , appdata_(a)
    , pkggrps_(new ODInst::PkgGroupSet)
    , forcedsite_(fs)
    , subdir_(sd)
    , timer_(0)
    , platform_(ODInst::Platform::thisPlatform())
    , startupmode_(false)
    , isonlinemode_(true)
{
    uiDialog::setTitlePos( -1 );
    setCancelText( "&Exit" );
    dlFailHandler().isfatal_ = true;

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
	    			ODInst::RelTypeNames(), "Release Type" );
    reltypefld_ = lcb->box();
    reltypefld_->setHSzPol( uiObject::MedMax );

    mODInstLogStartParagraph(uiODInstMgr);
    mODInstLog() << "Application: " << appdata_.getReport() << std::endl;
        
    CallBack cb( mCB(this,uiODInstMgr,onlineCB) );
    onlinebut_ = new uiRadioButton( this, "Install/Update Online", cb );
    onlinebut_->attach( leftAlignedBelow, lcb );
    onlinebut_->setChecked();
    offlinebut_ = new uiRadioButton( this,
				"Prepare offline installation packages", cb );
    offlinebut_->attach( leftAlignedBelow, onlinebut_ );
    
    BufferString basedir = getBaseDirFromSettings();
    uiFileInput::Setup su( basedir ); su.forread( false ).directories( true );
    basedirfld_ = new uiFileInput( this, "Installation base directory", su );
    basedirfld_->attach( leftAlignedBelow, offlinebut_ );

    plffld_ = new uiLabeledComboBox( this, OD::Platform::TypeNames(),
	    			     "Platform" );
    plffld_->box()->setPrefWidthInChar( 30 );
    plffld_->box()->setCurrentItem( OD::Platform::local().longName() );
    plffld_->box()->selectionChanged.notify( mCB(this,uiODInstMgr,plfSelCB) );
    plffld_->attach( alignedBelow, basedirfld_ );
    plffld_->display( false );

    settingsbut_ = new uiPushButton( topGroup(), "&Proxy settings", false );
    settingsbut_->setPixmap( "proxysettings" );
    settingsbut_->setPrefWidthInChar( 21 );
    settingsbut_->activated.notify( mCB(this,uiODInstMgr,networkSettingsCB) );

    postFinalise().notify( mCB(this,uiODInstMgr,startAction) );
}


uiODInstMgr::~uiODInstMgr()
{
    deepErase( reldata_ );
    delete timer_;
    delete dlhndlr_;
    delete pkggrps_;
}


void uiODInstMgr::startAction( CallBacker* )
{
    toStatusBar( "Connecting to Internet ..." );
    timer_ = new Timer;
    timer_->tick.notify( mCB(this,uiODInstMgr,startInternet) );
    timer_->start( 250, true );
}


void uiODInstMgr::networkSettingsCB( CallBacker* )
{
    uiProxyDlg dlg( this );
    dlg.setHelpID( mNoHelpID );
    if ( dlg.go() )
	startInternet( 0 );
}


void uiODInstMgr::plfSelCB( CallBacker* cb )
{
    if ( isonlinemode_ )
	return;

    selectPlatform();
    BufferString offlinedirnm( "od_offline_", platform_.shortName() );
    FilePath fp( File::getTempPath(), offlinedirnm );
    basedirfld_->setText( fp.fullPath() );
    basedirfld_->setTitleText( "Download directory" );
}


void uiODInstMgr::onlineCB( CallBacker* cb )
{
    if ( cb == onlinebut_ )
    {
	BufferString basdir = getBaseDirFromSettings();
	basedirfld_->setText( basdir );
	basedirfld_->setTitleText( "Installation base directory" );
	plffld_->display( false );
	isonlinemode_ = true;
    }
    else if ( cb == offlinebut_ )
    {
	plffld_->display( true );
	isonlinemode_ = false;
	plfSelCB( 0 );
    }
}


class uiODInstMgrSeedSiteFailHndlr : public uiODInstDLFailHndlr
{
public:

uiODInstMgrSeedSiteFailHndlr( uiODInstMgr* p )
    : uiODInstDLFailHndlr(p)
{
    isfatal_ = true;
}

bool handle( ODDLSite& dlsite, BufferString& newsite, float& newtmout )
{
    mODInstToLog( "Cannot get seed site" );
    uiHandleDLSiteFail dlg( par_, dlsite, true,
	    		    &ODInst::DLHandler::seedSites() );
    dlg.go();
    newsite = dlg.site(); newtmout = dlg.timeout();
    mODInstLog() << "Trying seed site: " << newsite <<
		    " with timeout " << newtmout << std::endl;
    return true;
}

};


void uiODInstMgr::startInternet( CallBacker* )
{
    DataBuffer* dbuf; float tmout = 0;
    if ( forcedsite_.isEmpty() )
    {
	uiODInstMgrSeedSiteFailHndlr fh( this );
	dbuf = ODInst::DLHandler::getSites( ODInst::DLHandler::seedSites(),
						fh, tmout );
    }
    else
    {
	mODInstLog() << "Forced download site: " << forcedsite_ << std::endl;
	dbuf = new DataBuffer( forcedsite_.size()+1, 1 );
        strcpy( (char*)dbuf->data(), forcedsite_.buf() );
    }

    if ( dlhndlr_ )
    {
	delete dlhndlr_;
	dlhndlr_= 0;
    }
	    					       
    dlhndlr_ = new ODInst::DLHandler( *dbuf, subdir_, tmout );
    dlHandler().setFailHandler( &dlFailHandler() );
    delete dbuf;

    initWin();
}


#define mErrUpdRet { \
    reportError( failmsg ); \
    ExitProgram(0); \
    return; }
void uiODInstMgr::initWin()
{
    if ( !dlHandler().isOK() )
    {
	const char* errmsg = dlHandler().errMsg();
	mODInstLog() << "DLHandler not OK: " << errmsg << std::endl;
	toStatusBar( errmsg );
	mEnableProceed( false );
	return;
    }

    if ( hasUpdateForInstMgr() )
    {
	BufferString msg( "There is an update available for the Installation "
			  "Manager. The new Installation Manager will be "
			  "downloaded and run now. Do you want to continue?" );
	const bool getupdate = uiMSG().askGoOn( msg );
	if ( getupdate )
	{
	    BufferString failmsg( "Failed to update the Installation Manager."
		    "\nPlease download the latest Installation Manager from "
		    "\nhttp://opendtect.org/index.php/download.html" );
	    BufferString tempdirnm( GetUserNm(), "_od_instmgr_exec" );
	    FilePath tempdirfp( FilePath::getTempDir(), tempdirnm );
	    if ( !File::isDirectory(tempdirfp.fullPath()) &&
		!File::createDir(tempdirfp.fullPath()) )
		mErrUpdRet
	    BufferString cmd( FilePath(tempdirfp,InstMgrPkg()).fullPath() );
	    uiTaskRunner uitr( this );
	    if ( !dlHandler().fetchFile( InstMgrPkg(),cmd,&uitr) )
		mErrUpdRet

	    failmsg = "Could not launch the new Installation Manager.";
	    failmsg += "\nPlease launch it manually from ";
	    failmsg += cmd;
	    File::makeExecutable( cmd, true );
#ifdef __win__
	    if ( !executeWinProg(cmd,args_,tempdirfp.fullPath()) )
#else
	    BufferString bgcmd( cmd, "&" );
	    if ( system(bgcmd) )
#endif
		mErrUpdRet
	}
	
	ExitProgram(0);
    }

    const BufferString dlurl( dlHandler().fullURL("") );
    mODInstToLog2( "DLHandler OK, URL=", dlurl );
    toStatusBar( BufferString("Connected to ",dlurl) );

    dlHandler().fetchFile( "rels.txt" );
    reltypefld_->setEmpty();
    BufferStringSet relstrs;
    ODInst::DLHandler::getFileData( dlHandler().fileData(), relstrs );
    if ( relstrs.isEmpty() )
    {
	mODInstToLog( "Got rels.txt, but no releases found." );
	uiMSG().error( "No releases found on server." );
	mEnableProceed( false );
    }
    else
    {
	reldata_.set( relstrs );
	mODInstToLog( "Got rels.txt, releases available:" );
	for ( int idx=0; idx<reldata_.size(); idx++ )
	{
	    const ODInst::RelData& rd = *reldata_[idx];
	    reltypefld_->addItem( rd.name_ );
	    mODInstToLog( rd.prettyName() );
	}
	if ( appdata_.exists() )
	{
	    const ODInst::RelData* rd
		= reldata_.get( ODInst::Version(appdata_.dirName()) );
	    if ( rd )
		reltypefld_->setCurrentItem( rd->name_ );
	}
	
    }
    
    if ( startupmode_ )
    {
	if ( !isOK() || acceptOK(0) )
	    done();
    }
}


bool uiODInstMgr::isOK() const
{
    return !reltypefld_->isEmpty();
}


ODInst::RelType uiODInstMgr::relType() const
{
    ODInst::RelType ret = ODInst::Stable;
    if ( !isOK() ) return ret;

    parseEnumRelType( reltypefld_->text(), ret );
    return ret;
}


const ODInst::RelData& uiODInstMgr::relData() const
{
    return *reldata_.get( reltypefld_->text() );
}


const char* uiODInstMgr::instDir() const
{
    return basedirfld_->fileName();
}


bool uiODInstMgr::getPkgGroups( const ODInst::Version& ver )
{
    mODInstToLog( "Fetching package definitions" );
    toStatusBar( "Fetching package definitions" );

    BufferStringSet defsfilenms;
    dlFailHandler().isfatal_ = false;
    dlHandler().fetchFile( "defs/list.txt" );
    const DataBuffer& listdbuf = dlHandler().fileData();
    std::fixedstreambuf listfsb( (char*)listdbuf.data(), listdbuf.size() );
    std::istream liststrm( &listfsb );
    ascistream listastrm( liststrm );
    IOPar listpar( listastrm );
    int idx = 0;
    while ( true )
    {
	PtrMan<IOPar> subpar = listpar.subselect( idx++ );
	if ( !subpar )
	    break;

	for ( int fdx=0; fdx<subpar->size(); fdx++ )
	    defsfilenms.add( subpar->getValue(fdx) );
    }

    delete pkggrps_;
    pkggrps_ = 0;
    ObjectSet<IOPar> iops;
    for ( idx=0; idx<defsfilenms.size(); idx++ )
    {
	BufferString fnm( "defs/" );
	fnm += defsfilenms.get( idx );
	if ( !dlHandler().fetchFile(fnm.buf()) )
	    continue;

	const DataBuffer& defsdbuf = dlHandler().fileData();
	std::fixedstreambuf fsb( (char*)defsdbuf.data(), defsdbuf.size() );
	std::istream strm( &fsb );
	ascistream astrm( strm );
	if ( !pkggrps_ )
	    pkggrps_ = new ODInst::PkgGroupSet( IOPar(astrm) );

	while ( astrm.type() != ascistream::EndOfFile )
	{
	    IOPar* iop = new IOPar( astrm );
	    if ( iop->isEmpty() )
		delete iop;
	    else
		iops += iop;
	}
    }

    const ODInst::RelType rt = relType();
    const bool comm = rt != ODInst::PreStable && rt != ODInst::PreDevelopment;
    pkggrps_->set( iops, comm );

    toStatusBar( "" );

    if ( pkggrps_->isEmpty() )
    {
	mODInstToLog( "Got defs file(s), but no packages found." );
	uiMSG().error( "No available packages found on server." );
	return false;
    }

    mODInstToLog2( "Got defs file(s), nr pkgs available:", pkggrps_->size() );
    BufferString fnm( ver.dirName(false) );
    fnm.add( "/" ).add( "versions.txt" );
    mODInstToLog2( "Reading versions from", fnm );
    dlFailHandler().isfatal_ = false;
    const bool res = dlHandler().fetchFile( fnm );
    dlFailHandler().isfatal_ = true;
    if ( !res )
	return false;

    BufferStringSet verlines;
    ODInst::DLHandler::getFileData( dlHandler().fileData(), verlines );
    if ( verlines.isEmpty() )
    {
	mODInstToLog( "Got versions.txt, but none found." );
	uiMSG().error("No version info for packages found on server");
    }

    dlFailHandler().isfatal_ = true;
    pkggrps_->setVersions( verlines, platform_ );
    return true;
}


void uiODInstMgr::getImages()
{
    BufferString imagefile( "images.zip" );
    FilePath imagefilepath( ODInst::sTempDataDir() );
    imagefilepath.add( imagefile );
    uiTaskRunner uitr( this );
    if ( dlHandler().fetchFile(imagefile,imagefilepath.fullPath(),&uitr) )
    {
	BufferString errmsg;
    	if( !ZipUtils::unZipArchive(imagefilepath.fullPath(),
	    imagefilepath.pathOnly(),errmsg) )
	{   
	    mODInstLog() << errmsg;
	    uiMSG().error( "Failed to fetch icons" );
	}
    }
}


int uiODInstMgr::getPackageChoice() const
{
    BufferStringSet opt;
    opt.add( "[&GPL] Typical GPL-only (free) installation" );
    opt.add( "[&Academic] Typical academic installation" );
    opt.add( "[&Commercial] Typical commercial installation" );
    opt.add( "[&Pick] I want to pick each package by hand" );
    uiDialog::Setup su( "Package Selection",
			"What packages do you want to select, initially?",
			mNoHelpID );
    su.canceltext_ = "<<Back";
    su.okcancelrev( true );
    su.oktext_ = "Proceed>>";
    uiODInstMgr* _this = const_cast<uiODInstMgr*>( this );
    uiGetChoice uich( _this, su, opt, true );
    uich.setDefaultChoice( 2 );
    uich.go();
    return uich.choice();
}


int uiODInstMgr::checkInstDir( FilePath& instdir )
{
    BufferString fnm = instdir.fileName();
    if ( fnm.isEqual("OpendTect",true) || fnm.isEqual("dGB",true)
	    || fnm.isEqual("OD",true) || fnm.isEqual("dTect",true) )
	return 0;

    DirList dl( instdir.fullPath() );
    if ( !dl.size() )
	return 0;

    FilePath fp( instdir );
    fp.add( "OpendTect" );
    BufferStringSet opt;
    opt.add( "Yes, I know what I am doing" );

    BufferString str( "No, " );
    str += File::isDirectory(fp.fullPath()) ? "use" : "create";
    str += " sub-directory "; str += fp.fullPath();
    opt.add( str );

    str = instdir.fullPath(); str += " contains other files/directories. ";
    str += "Are you sure you want to install OpendTect here?";
    uiDialog::Setup su( "Installation Base Directory", str.buf(), mNoHelpID );
    su.canceltext_ = "<<Back";
    su.okcancelrev( true );
    su.oktext_ = "Proceed>>";
    uiGetChoice uich( this, su, opt, true );
    uich.go();
    const int choice = uich.choice();
    if ( choice == 1 )
    {
	instdir = fp;
	basedirfld_->setFileName( instdir.fullPath() );
	if ( !File::exists(instdir.fullPath()) )
	    if ( !File::createDir(instdir.fullPath()) )
	    { 
		BufferString errmsg( instdir.fullPath(), " is not writable");
		mODInstLog() << errmsg;
		uiMSG().error( appdata_.errMsg() ); 
		return false; 
	    }
    }

    return choice;
}


bool uiODInstMgr::acceptOK( CallBacker* cb )
{
    if ( !isOK() )
	return rejectOK( cb );

    if ( !reldata_.size() )
	return false;
    const ODInst::RelData& rd = relData();
    FilePath basedir( instDir() );
	if ( !appdata_.set(basedir.fullPath(),rd.version_.dirName(true)) )
	    { uiMSG().error( appdata_.errMsg() ); return false; }
    appdata_.setRelType( rd.reltype_ );
    if ( isonlinemode_ && !appdata_.exists() )
    {
	const int res = checkInstDir( basedir );
	if ( res < 0 )
	    return false;
	else if ( res == 1
		&& !appdata_.set(basedir.fullPath(),rd.version_.dirName(true)) )
	{ uiMSG().error( appdata_.errMsg() ); return false; }
    }

    if ( isonlinemode_ )
    {
	Settings& setts = ODInst::userSettings();
	setts.set( sKeyBaseDir, basedir.fullPath() );
	setts.write();
    }

    if ( !dlHandler().remoteStatusOK(false) )
	{ uiMSG().error( "The site is currently unavailable."
			"\nPlease try again later" ); return false; }
    selectPlatform();
    if ( !getPkgGroups(rd.version_) )
	return false;

    mODInstToLog( "Proceeding to package selection" );
    mODInstLogEndParagraph(uiODInstMgr);
    getImages();
    const int ch = !appdata_.exists() ? getPackageChoice() : 0;
    if ( ch < 0 )
	return false;
   
    uiODInstPkgMgr pkgmgr( this, appdata_, *dlhndlr_, *pkggrps_, platform_, 
	    		   rd, ch, isonlinemode_ );
    bool retval = false;
    if ( startupmode_ )
    {
	retval = true;
	const ODInst::AutoInstType ait = ODInst::getAutoInstType();
	if ( ait == ODInst::FullAuto )
	    pkgmgr.setAutoUpdate( true );
    }

    if ( pkgmgr.go() )
	retval = true;

    return retval;
}


bool uiODInstMgr::rejectOK( CallBacker* )
{
    mODInstToLog( "Exiting on user request" );
    mODInstLogEndParagraph(uiODInstMgr);
    ExitProgram(0);
    return true;
}


bool uiODInstMgr::hasUpdateForInstMgr()
{
    mODInstToLog( "Checking for update for Installation Manager" );
    dlHandler().fetchFile( BufferString("Installer/versions.txt") );
    BufferStringSet verlines;
    ODInst::DLHandler::getFileData( dlHandler().fileData(), verlines );
    if ( verlines.isEmpty() )
    {
	mODInstToLog( "No versions found for InstMgr: no updates available" );
	return false;
    }

    ODInst::PkgKey* pk = 0;
    for ( int idx=0; idx<verlines.size(); idx++ )
    {
	ODInst::PkgKey* curpk = new ODInst::PkgKey( verlines.get(idx) );
	if ( curpk->plf_ == ODInst::Platform::thisPlatform() )
	{
	    pk = curpk;
	    break;
	}

	delete curpk;
    }

    if ( !pk )
    {
	mODInstToLog( "No versions found for InstMgr: no updates available" );
	return false;
    }

    FilePath verfile( GetSoftwareDir(0), ODInst::AppData::sKeyRelInfoSubDir(),
	    	      BufferString("ver.",pk->fileNameBase(),".txt") );
    StreamData sd = StreamProvider(verfile.fullPath()).makeIStream();
    if ( !sd.usable() )
    {
	mODInstToLog( "No versions found for InstMgr: no updates available" );
	return false;
    }

    BufferString verstr;
    StrmOper::readLine( *sd.istrm, &verstr );
    ODInst::Version instver( verstr );
    const bool ret = pk->ver_ > instver;
    if ( ret )
	mODInstToLog( ret ? "Updates are available" : "No updates available" );

    return ret;
}


void uiODInstMgr::setArgs( int argc, char** argv )
{
    args_.setEmpty();
    for ( int idx=1; idx<argc; idx++ )
	args_.add( argv[idx] ).add( " " );
}


void uiODInstMgr::selectPlatform()
{
    if ( isonlinemode_ )
    {
	platform_ = ODInst::Platform::thisPlatform();
	return;
    }
    
    OD::Platform plf( (OD::Platform::Type) plffld_->box()->getIntValue() );
    platform_.setType( plf.type() );
}


BufferString uiODInstMgr::InstMgrPkg() const
{
    BufferString ret( "OpendTect_" );
    ret += "Installer_";
    switch ( OD::Platform::local().type() )
    {
	case OD::Platform::Lin32 : ret += "lux32.sh"; break;
	case OD::Platform::Lin64 : ret += "lux64.sh"; break;
	case OD::Platform::Win32 : ret += "win32.exe"; break;
	case OD::Platform::Win64 : ret += "win64.exe"; break;
	case OD::Platform::Mac : ret += "mac.dmg"; break;
	default: ;
    }

    return ret;
}


BufferString uiODInstMgr::getBaseDirFromSettings() const
{
    BufferString basedir;
    Settings& setts = ODInst::userSettings();
    setts.get( sKeyBaseDir, basedir );
    if ( basedir.isEmpty() )
    {
	basedir = appdata_.baseDirName();
	if ( !basedir.isEmpty() )
	    mODInstLog() << "Default basedir from user settings";
	else
	{
#ifdef __win__
	    FilePath fp( GetSpecialFolderLocation( CSIDL_PROGRAM_FILES ) );
#else
# ifdef __mac__
	    FilePath fp( "/Applications" );
# else
	    FilePath fp( GetPersonalDir() );
# endif
#endif
	    fp.add( "OpendTect" );
	    basedir = fp.fullPath();
	    mODInstLog() << "Default basedir generated";
	}
	mODInstLog() << ": " << basedir << std::endl;
    }

    return basedir;
}

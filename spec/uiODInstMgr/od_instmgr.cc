/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: od_instmgr.cc 7594 2013-04-02 09:20:24Z raman.singh@dgbes.com $";

#include "uiodinstmgr.h"

#include "databuf.h"
#include "file.h"
#include "filepath.h"
#include "envvars.h"
#include "moddepmgr.h"
#include "oddlsite.h"
#include "odinst.h"
#include "odinstappdata.h"
#include "odinstlogger.h"
#include "odinstver.h"
#include "prog.h"
#include "settings.h"
#include "strmprov.h"

#include "uimain.h"
#include "uimsg.h"
#include "uicmddrivermgr.h"

#ifdef __mac__
# include "od_instmgr_128.xpm"
#else
# include "od_instmgr.xpm"
#endif

static const char* stdsitesubdir = "relman";
static const int cUpdChkReport = 1;
static const int cUpdChkList = 2;
static const int cUpdChkLaunch = 3;
static const int cUpdChkStartup = 4;

static const char* usage_str =
"[  --help]              show this message\n"
"[  --updcheck_report    exit with status 1 if updates are available\n"
"|| --updcheck_list      provide a list with files that need download\n"
"|| --updcheck_launch    launch only if updates are available\n"
"|| --updcheck_startup]	 do the actions required for OpendTect startup\n"
"[  --site site]         alternative download site\n"
"[  --sitesubdir subdir] alternative download site subdir (default is relman)\n"
"[  --instdir inst_dir   installation directory (basedir/reldir)\n"
"|| --basedir basedir]   basedir of OpendTect installations\n"
"[  --cmdscript script]  Command Driver script to execute (full path)\n"
"[  --cmdlogfile file]   Command Driver log file (full path)\n"
"&& --reldir rel_dir]    subdirectory from the basedir\n";

static bool haveUpdates(const ODInst::AppData&,const char*,const char*,
			BufferStringSet&,BufferString&);
static void notifyOD(bool);

int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "uiCmdDriver" );
    if ( argc >= 2 && !strcmp(argv[1],"--help") )
    {
	std::cerr << "Usage:" << argv[0] << '\n' << usage_str << std::endl;
	return 0;
    }

    BufferString site, basedir, reldir, sitesubdir( stdsitesubdir );
    BufferString cmddriverscript, cmddriverlogfile;
    int updcheck = 0;
    for ( int iarg=1; iarg<argc; iarg++ )
    {
	if ( !strcmp(argv[iarg],"--site") )
	    { iarg++; if ( iarg<argc ) site = argv[iarg]; }
	if ( !strcmp(argv[iarg],"--sitesubdir") )
	    { iarg++; if ( iarg<argc ) sitesubdir = argv[iarg]; }
	if ( !strcmp(argv[iarg],"--basedir") )
	    { iarg++; if ( iarg<argc ) basedir = argv[iarg]; }
	if ( !strcmp(argv[iarg],"--reldir") )
	    { iarg++; if ( iarg<argc ) reldir = argv[iarg]; }
	if ( !strcmp(argv[iarg],"--cmdscript") )
	    { iarg++; if ( iarg<argc ) cmddriverscript = argv[iarg]; }
	if ( !strcmp(argv[iarg],"--cmdlogfile") )
	    { iarg++; if ( iarg<argc ) cmddriverlogfile = argv[iarg]; }
	if ( !strcmp(argv[iarg],"--instdir") )
	{
	    iarg++;
	    if ( iarg<argc )
	    {
		FilePath fp( argv[iarg] );
		basedir = fp.pathOnly(); reldir = fp.fileName();
	    }
	}
	if ( matchString("--updcheck",argv[iarg]) )
	{
	    updcheck = cUpdChkReport;
	    const char* rest = argv[iarg] + 10;
	    if ( *rest == '_' ) rest++;
	    if ( !strcmp(rest,"launch") )
		updcheck = cUpdChkLaunch;
	    else if ( !strcmp(rest,"startup") )
		updcheck = cUpdChkStartup;
	    else if ( !strcmp(rest,"list") )
		updcheck = cUpdChkList;
	}
    }

    const bool havebd = !basedir.isEmpty();
    const bool haverd = !reldir.isEmpty();

    BufferString subdirfrominst = GetEnvVar("OD_INST_SITE_SUBDIR");
    if ( !subdirfrominst.isEmpty() )
	sitesubdir = subdirfrominst;

    BufferString sitefrominst = GetEnvVar("OD_INST_SITE");
    if ( !sitefrominst.isEmpty() )
	site = sitefrominst;

    if ( !updcheck )
	(void)ODInst::Logger::theInst( "_install" );
    else
    {
	const ODInst::AutoInstType ait = ODInst::getAutoInstType();
	if ( updcheck == cUpdChkStartup
	 && (ait == ODInst::NoAuto || ait == ODInst::InformOnly) )
	    return 0;

	if ( !havebd || !haverd )
	{
	    std::cerr << "--updcheck requires --instdir" << std::endl;
	    return 1;
	}
	(void)ODInst::Logger::theInst( "_updcheck" );
    }

    mODInstLog() << "Command= " << argv[0];
    if ( argc < 2 )
	mODInstToLog( ", no arguments. " );
    else
    {
	mODInstToLog( ", arguments:" );
	for ( int iarg=1; iarg<argc; iarg++ )
	    mODInstToLogQuoted(iarg,argv[iarg]) << std::endl;
	mODInstLog() << std::endl;
    }

    uiMain::setXpmIconData( od_instmgr_xpm_data );
    uiMain app( argc, argv );

    ODInst::AppData ad; BufferString errmsg;
    if ( !havebd && !haverd )
	mODInstToLog("No installation paths passed to program.");
    else
    {
	mODInstToLogQuoted("Base directory specified:",basedir) << '\n';
	mODInstToLogQuoted("Sub-directory name:",reldir) << std::endl;

	ad.set( basedir, reldir );
	if ( havebd && haverd && !ad.exists() )
	{
	    errmsg = "Cannot read installed application data.\n";
	    errmsg.add( "Directory: " ).add( basedir ).add( "\n" )
		  .add( "Release: " ).add( reldir );
	    BufferString msg( ad.errMsg() );
	    if ( !msg.isEmpty() )
		errmsg.add( "\n\nError:\n" ).add( msg );
	    mODInstToLog(errmsg);
	}
	if ( updcheck )
	{
	    BufferStringSet nms; BufferString dlurl;
	    const bool haveupd = haveUpdates( ad, sitesubdir, site,
		    				nms, dlurl );
	    mODInstToLog( haveupd ? "[Y] Update(s) available"
				  : "[N] No updates available" );
	    if ( updcheck == cUpdChkReport )
	    {
		notifyOD( haveupd );
		return haveupd ? 1 : 0;
	    }
	    
	    if ( !haveupd )
		return 0;

	    if ( updcheck == cUpdChkList )
	    {
		std::cerr << dlurl;
		for ( int idx=0; idx<nms.size(); idx++ )
		    std::cerr << ' ' << nms.get(idx);
		std::cerr << std::endl;
		return 0;
	    }

	    if ( !File::isWritable(basedir) )
	    {
		mODInstToLog( "User does not have write permission in " );
		mODInstToLog( basedir.buf() );
		return 0;
	    }
	}
    }

    if ( !errmsg.isEmpty() )
    {
	mODInstToLog("Exiting program.");
	uiMSG().error( errmsg );
	ExitProgram( 1 );
    }

    mODInstToLog("-----");

    uiODInstMgr* inst = new uiODInstMgr( 0, ad, sitesubdir, site );
    inst->setArgs( argc, argv );
    if ( updcheck )
    {
	inst->setTitleText( "New updates available" );
	if ( updcheck == cUpdChkStartup )
	    inst->setStartupMode( true );
    }

    app.setTopLevel( inst );

    if ( !cmddriverscript.isEmpty() )
    {
	CmdDrive::uiCmdDriverMgr* mgr = new CmdDrive::uiCmdDriverMgr();
	mgr->addCmdLineScript( cmddriverscript );
	mgr->setLogFileName( cmddriverlogfile );
    }

//    Optional:
//    mgr->setDefaultScripsDir( ? );
//    mgr->setDefaultLogDir( ? );

    inst->go();
    ExitProgram( app.exec() );

    return 0;
}


struct od_instmgr_DLFailHandler : public ODInst::DLHandler::FailHndlr
{
od_instmgr_DLFailHandler() { isfatal_ = true; }
bool handle( ODDLSite& dls, BufferString&, float& )
{
    mODInstToLog2( "Cannot download from", dls.host() );
    mODInstToLog( "Download check will report no updates available" );
    return false;
}

};


static bool haveUpdates( const ODInst::AppData& ad, const char* subd,
			 const char* site, BufferStringSet& nms,
			 BufferString& dlurl )
{
    mODInstToLog( "Checking for updates" );
    od_instmgr_DLFailHandler fh; DataBuffer* dbuf; float tmout = 0;
    if ( !site || !*site )
	dbuf = ODInst::DLHandler::getSites(
		ODInst::DLHandler::seedSites(), fh, tmout );
    else
    {
	dbuf = new DataBuffer( strlen(site)+1, 1 );
	strcpy( (char*)dbuf->data(), site );
    }

    if ( !dbuf )
    {
	mODInstToLog( "Failed to connect to the download site" );
	return false;
    }

    ODInst::DLHandler dlh( *dbuf, subd, tmout );
    if ( !dlh.isOK() )
    {
	mODInstToLog( "Failed to connect to the download site" );
	return false;
    }

    dlh.setFailHandler( &fh );
    delete dbuf;

    dlh.fetchFile( BufferString(ad.dirName(),"/","versions.txt") );
    BufferStringSet verlines;
    ODInst::DLHandler::getFileData( dlh.fileData(), verlines );
    if ( verlines.isEmpty() )
    {
	mODInstToLog( "No versions found: no updates available" );
	return false;
    }

    const bool ret = ad.needUpdate( verlines, ODInst::Platform::thisPlatform(),
	    			    &nms );
    if ( ret )
	mODInstToLog( ret ? "Updates are available" : "No updates available" );

    dlurl = dlh.site().fullURL(0);
    return ret;
}


void notifyOD( bool hasupd )
{
#ifdef __win__
    BufferString notifile( FilePath(File::getTempPath(),"od_updt").fullPath() );
    if ( hasupd )
    {
	StreamData sd = StreamProvider( notifile ).makeOStream();
	*sd.ostrm  << "1" << std::endl;
	sd.close();
    }
    else if ( File::exists(notifile) )
	File::remove( notifile );
#endif
    //TODO: Handle multiple versions (e.g. 4.4.0, 4.5.0)
}


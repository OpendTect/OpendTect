/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: odinst.cc,v 1.18 2012-07-05 02:36:30 cvsraman Exp $";

#include "odinst.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "odplatform.h"
#include "envvars.h"
#include "strmoper.h"
#include "strmprov.h"
#include "settings.h"
#include "bufstringset.h"

#define mDeclEnvVarVal const char* envvarval = GetEnvVar("OD_INSTALLER_POLICY")
#define mRelRootDir GetSoftwareDir(1)

#ifdef __win__
#include <Windows.h>
#include <direct.h>
static BufferString getInstDir()
{
    BufferString dirnm( _getcwd(NULL,0) );
    const int len = dirnm.size() - 10;
    if ( len > 0 )
	dirnm[len] = '\0';
    return dirnm;
}
#undef mRelRootDir
#define mRelRootDir getInstDir()

unsigned int getWinVersion()
{
    DWORD dwVersion = 0; 
    DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0; 
    dwVersion = GetVersion();
    dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
    return dwMajorVersion;
}


bool ExecShellCmd( const char* comm, const char* parm, const char* runin )
{
   int res = (int) ShellExecute( NULL, "runas",
				  comm,
				  parm,    // params
				  runin, // directory
				  SW_SHOW );
    return res > 32;
}


bool ExecProc( const char* comm, bool inconsole, bool inbg,
			    const char* runin )
{
    if ( !comm || !*comm ) return false;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(STARTUPINFO));
    ZeroMemory( &pi, sizeof(pi) );
    si.cb = sizeof(STARTUPINFO);

    if ( !inconsole )
    {
	si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);	
	si.wShowWindow = SW_HIDE;
    }
    
   //Start the child process. 
    int res = CreateProcess( NULL,	// No module name (use command line). 
        const_cast<char*>( comm ),
        NULL,				// Process handle not inheritable. 
        NULL,				// Thread handle not inheritable. 
        FALSE,				// Set handle inheritance to FALSE. 
        0,				// Creation flags. 
        NULL,				// Use parent's environment block. 
        const_cast<char*>( runin ), 	// Use parent's starting directory if runin is NULL. 
        &si, &pi );
	
    if ( res )
    {
	if ( !inbg )  WaitForSingleObject( pi.hProcess, INFINITE );
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
    }
    else
    {
	char *ptr = NULL;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM,
	    0, GetLastError(), 0, (char *)&ptr, 1024, NULL);

	fprintf(stderr, "\nError: %s\n", ptr);
	LocalFree(ptr);
    }

    return res;
}


bool ExecuteProg( const char* comm, const char* parm, const char* runin )
{
	 if ( !comm || !*comm ) return false;
	 unsigned int winversion = getWinVersion();
	 if ( winversion < 6 )
		 return ExecProc( comm, true, true, runin );
	 return ExecShellCmd( comm, parm, runin );
}

#endif

DefineNameSpaceEnumNames(ODInst,AutoInstType,1,"Auto update")
{ "Manager", "Inform", "Full", "None", 0 };


DefineNameSpaceEnumNames(ODInst,RelType,0,"Release type")
{
	"Stable",
	"Development",
	"Pre-Release Stable",
	"Pre-Release Development",
	"Old Version",
	"Other",
	0
};


ODInst::RelType ODInst::getRelType()
{
    FilePath relinfofp( GetSoftwareDir(true), "relinfo", "README.txt" );
    const BufferString reltxtfnm( relinfofp.fullPath() );
    if ( !File::exists(reltxtfnm) )
	return ODInst::Other;
    StreamData sd( StreamProvider(reltxtfnm).makeIStream() );
    if ( !sd.usable() )
	return ODInst::Other;

    char appnm[64], relstr[64];
    appnm[0] = relstr[0] = '\0';
    StrmOper::wordFromLine(*sd.istrm,appnm,64);
    StrmOper::wordFromLine(*sd.istrm,relstr,64);
    sd.close();
    int relstrlen = strlen( relstr );
    if ( appnm[0] != '[' || relstrlen < 4 || relstr[0] != '('
	|| relstr[relstrlen-1] != ']' || relstr[relstrlen-2] != ')' )
	return ODInst::Other;

    relstr[relstrlen-2] = '\0';
    return ODInst::parseEnumRelType( relstr+1 );
}


const BufferStringSet& ODInst::autoInstTypeUserMsgs()
{
    static BufferStringSet* ret = 0;
    if ( !ret )
    {
	ret = new BufferStringSet;
	ret->add( "[&Manager] Start the Installation Manager "
		    "when updates are available" );
	ret->add( "[&Inform] When new updates are present, "
		    "show this in OpendTect's title bar" );
	ret->add( "[&Auto] Automatically download and install new updates "
		    "(requires sufficient administrator rights)" );
	ret->add( "[&None] Never check for updates" );
    };
    return *ret;
}
const char* ODInst::sKeyAutoInst() { return ODInst::AutoInstTypeDef().name(); }


bool ODInst::canInstall()
{
    return File::isWritable( mRelRootDir );
}


#define mDefCmd(errretval) \
    FilePath installerdir( GetSoftwareDir(0) ); \
    installerdir.setFileName( "Installer" ); \
    if ( !File::isDirectory(installerdir.fullPath()) ) \
	return errretval; \
    installerdir.add( __iswin__ ? "od_instmgr" : "run_installer" ); \
    BufferString cmd( __iswin__ ? "" : "@", installerdir.fullPath() ); \
    cmd.add( " --instdir " ).add( "\"" ).add( mRelRootDir ).add( "\"" ); \
   


void ODInst::startInstManagement()
{
#ifndef __win__
    mDefCmd();
    chdir( installerdir.pathOnly() );
    StreamProvider( cmd ).executeCommand( true, true );
    chdir( GetSoftwareDir(0) );
#else
    FilePath installerdir( GetSoftwareDir(0) ); 
    BufferString dir = installerdir.fullPath();
    installerdir.setFileName( "Installer" );
    dir = installerdir.fullPath();
    if ( !File::isDirectory(installerdir.fullPath()) )
	return;
    installerdir.add( "od_instmgr" );
    BufferString cmd( installerdir.fullPath() ); 
    BufferString parm( " --instdir "  );
    parm.add( "\"" ).add( mRelRootDir ).add( "\"" );
    
    ExecuteProg( cmd, parm, installerdir.pathOnly() );
#endif
}

bool ODInst::updatesAvailable()
{

    mDefCmd(false); cmd.add( " --updcheck_report" );
#ifndef __win__

    chdir( installerdir.pathOnly() );
    const bool ret = !StreamProvider( cmd ).executeCommand( false );
    chdir( GetSoftwareDir(0) );
    return ret;
#else
    ExecOSCmd( cmd, false, true );
    FilePath tmp( File::getTempPath(), "od_updt" );
    bool ret = File::exists( tmp.fullPath() );
    if ( ret )
	File::remove( tmp.fullPath() );
    return ret;
#endif
}


const char* ODInst::getPkgVersion( const char* file_pkg_basenm )
{
    static BufferString ret;
    const BufferString part1( "ver.", file_pkg_basenm );
    BufferString fnm = part1;
    fnm.add( "_" ).add( OD::Platform::local().shortName() );
    FilePath fp( GetSoftwareDir(1), "relinfo", fnm );
    fp.setExtension( "txt", false );

    fnm = fp.fullPath();
    if ( !File::exists(fnm) )
    {
	fp.setFileName( part1 ); fp.setExtension( "txt", false );
	fnm = fp.fullPath();
	if ( !File::exists(fnm) )
	    { ret = "[error: version file not found]"; return ret.buf(); }
    }

    File::getContent( fnm, ret );
    if ( ret.isEmpty() )
	ret = "[error: empty version file]";
    return ret.buf();
}


bool ODInst::autoInstTypeIsFixed()
{
    mDeclEnvVarVal;
    return envvarval && *envvarval;
}


ODInst::AutoInstType ODInst::getAutoInstType()
{
    mDeclEnvVarVal;
    const char* res = envvarval && *envvarval ? envvarval
			: userSettings().find( sKeyAutoInst() ).str();
    return res && *res ? parseEnumAutoInstType( res ) : ODInst::InformOnly;
}


void ODInst::setAutoInstType( ODInst::AutoInstType ait )
{
    userSettings().set( sKeyAutoInst(), ODInst::toString(ait) );
    userSettings().write();
}


Settings& ODInst::userSettings()
{
    return Settings::fetch( "instmgr" );
}

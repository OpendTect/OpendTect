
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odinstwinutils.cc 7930 2013-05-31 11:40:13Z ranojay.sen@dgbes.com $";

#include "odinstwinutils.h"

#include "bufstring.h"
#include "file.h"
#include "filepath.h"
#include "odinstappdata.h"
#include "odinstlogger.h"
#include "odinstpkgprops.h"
#include "strmprov.h"
#include "winutils.h"


#ifdef __win__
#include <shlobj.h>

bool CreateLink( LPCSTR lpszPathObj, LPCSTR lpszworkdir, LPCSTR lpszPathLink, 
		 LPCSTR lpszDesc )
{
    HRESULT hres;
    IShellLink *psl;
    CoInitialize (NULL);

    hres = CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
			     IID_IShellLink, (LPVOID*)&psl );
    if ( SUCCEEDED(hres) )
    {
	IPersistFile* ppf;
	hres = psl->SetPath( lpszPathObj );
	hres = psl->SetDescription( lpszDesc );
	hres = psl->SetWorkingDirectory( lpszworkdir );
	hres = psl->QueryInterface( IID_IPersistFile, (LPVOID*)&ppf );

	if ( SUCCEEDED(hres) )
	{
	    WCHAR wsz[MAX_PATH];
	    hres = MultiByteToWideChar( CP_ACP, 0, lpszPathLink,
					-1, wsz, MAX_PATH);
	    hres = ppf->Save( wsz, true );
	    ppf->Release();
	}

	psl->Release();
    }

    return SUCCEEDED(hres);
}

#endif

ODInst::WinUtils::WinUtils(const AppData& appdata, const Platform& plf )
    : appdata_(appdata)
    , platform_(plf)
{}


ODInst::WinUtils::~WinUtils()
{}


#define mErrorRet(msg) \
{ mODInstToLog(msg);  errmsg_ = msg; return false; } \

bool ODInst::WinUtils::createDeskTopLinks( const BufferString& pckgnm )
{
    FilePath startmenu( GetSpecialFolderLocation(CSIDL_COMMON_PROGRAMS),
			"OpendTect" );
    if ( !File::exists(startmenu.fullPath()) )
    {
	if ( !File::createDir(startmenu.fullPath()) )
	    mErrorRet( "Failed to make \"Start menu\" entry" );
    }

    const BufferString version = appdata_.dirName();
    startmenu.add( version.buf() ); 

    if ( !File::exists(startmenu.fullPath()) )
    {
	if ( !File::createDir(startmenu.fullPath()) )
	    mErrorRet( "Failed to make \"Start menu-OpendTect entry\"" );
    }

    BufferString desc( "OpendTect ", version.buf() );
    BufferString linkname( "OpendTect ", version.buf() );
    linkname += ".lnk";
    FilePath link( GetSpecialFolderLocation(CSIDL_COMMON_DESKTOPDIRECTORY),
	linkname.buf() );
    
    FilePath work( appdata_.binPlfDirName() );
    FilePath bin( work.fullPath(), pckgnm );
    startmenu.add( linkname.buf() );
    bool res = true;
    res &= makeUninstallScripts( appdata_.fullDirName(),
	startmenu.dirUpTo(startmenu.nrLevels()-2),
			    link.fullPath(), version.buf() );
    res &= CreateLink( bin.fullPath(), work.fullPath(), startmenu.fullPath(),
		    desc.buf() );
    res &= CreateLink( bin.fullPath(), work.fullPath(), link.fullPath(), desc.buf() );
    if ( !res )
	mErrorRet( "failed to create desktop link" );

    res &= createStartMenuLinks( "od_remote_service_manager.exe",
				 "Remote Processing Service", platform_.is32Bits(), false );
    if ( !res )
	return false;
    BufferString uninstlnk( startmenu.pathOnly(), "\\Uninstall OpendTect " );
    uninstlnk += version.buf();
    uninstlnk += " (Run as administrator)";
    uninstlnk += ".lnk";
    res &= CreateLink( uninstscript_.buf(), uninstscript_.buf(),
	uninstlnk.buf(), "Uninstall OpendTect" );
    return res;
}


bool ODInst::WinUtils::makeUninstallScripts( const char* path,
		const char* startlnk, const char* desktoplnk,
		const char* ver )
{
    BufferString filenm( "uninst", ver );
    filenm += ".bat";
    FilePath batchpath( appdata_.baseDirName(), filenm.buf() );
    uninstscript_ = batchpath.fullPath();
    StreamData sd = StreamProvider( uninstscript_ ).makeOStream();
    BufferString batchheader( "@echo off\necho You need sufficient"
			      " administrative rights"
			      " to perform this operation. Please run this as"
			      " \"Run as Adminstrator\" to completely remove"
			      " the installation\n\n"
	"SET /p param=Are you sure ? y/n:\nif %param% == y "
	"goto REMOVE\ngoto END\n:REMOVE" );
    *sd.ostrm << batchheader << std::endl;
    *sd.ostrm << "rd /S /Q " << "\"" << path << "\"" << std::endl;
    *sd.ostrm << "rd /S /Q " << "\"" << startlnk << "\"" << std::endl;
    *sd.ostrm << "del " << "\"" << desktoplnk << "\"" << std::endl;
    *sd.ostrm << "del " << "\"" << uninstscript_ << "\"" << std::endl;
    *sd.ostrm << ":END" << std::endl;
    sd.close();
    return true;
}


bool ODInst::WinUtils::createStartMenuLinks( const BufferString& fnm,
					     const char* desc, bool isw32,
					     bool inrel )
{
    FilePath startmenu( GetSpecialFolderLocation(CSIDL_COMMON_PROGRAMS),
			"OpendTect" );
    if ( !File::exists(startmenu.fullPath()) )
    {
	if ( !File::createDir(startmenu.fullPath()) )
	    mErrorRet( "Failed to make \"Start menu\" entry" );
    }

    const BufferString version = appdata_.dirName();
    startmenu.add( version.buf() ); 

    if ( !File::exists(startmenu.fullPath()) )
    {
	if ( !File::createDir(startmenu.fullPath()) )
	    mErrorRet( "Failed to make \"Start menu-OpendTect entry\"" );
    }

    BufferString linkname( desc );
    linkname += ".lnk";
    
    FilePath bin( inrel ? appdata_.binPlfDirName() 
			: appdata_.binPlfBaseDirName()  );
    if ( fnm == "lmtools.exe" )
	bin.add( "lm.dgb" );

    if ( fnm == "od_remote_service_manager.exe" )
	bin.add( "rsm" );

    bin.add( fnm );
    startmenu.add( linkname.buf() );
    if ( !File::exists(bin.fullPath()) )
	mErrorRet( "File not found" );

    if ( !CreateLink(bin.fullPath(),bin.pathOnly(),startmenu.fullPath(),desc) )
	mErrorRet( "failed to create start menu link" );
    return true;
}


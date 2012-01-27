/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          December 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: ziputils.cc,v 1.5 2012-01-27 12:03:41 cvsranojay Exp $";

#include "ziputils.h"

#include "file.h"
#ifdef __win__
#import "c:\windows\system32\shell32.dll"
#endif

#define mDirCheck( dir ) \
    if ( !File::exists(dir) ) \
    { \
	errmsg_ = dir; \
	errmsg_ += " does not exist"; \
	return false; \
    } \

bool ZipUtils::Zip( const char* src, const char* dest )
{
   mDirCheck(src);
   mDirCheck(dest);

#ifdef __win__
    return doZip( src, dest );
#else
    // TODO on Linux
    return false;
#endif
}


bool ZipUtils::UnZip( const char* src, const char* dest )
{
    mDirCheck(src);
    mDirCheck(dest);

#ifdef __win__
    return doUnZip( src, dest );
#else
    // TODO on Linux
    return false;
#endif
}


bool ZipUtils::doZip( const char* src, const char* dest )
{
    return true;
}


void ZipUtils::makeFileList( const char* src )
{
#ifdef __win__
  HZIP hz = OpenZip( src, 0 );
  ZIPENTRY ze; GetZipItem( hz, -1, &ze );
  const int numitems = ze.index;
  filelist_.erase();
  for ( int zi=0; zi<numitems; zi++ )
  { 
     ZIPENTRY ze; GetZipItem( hz, zi, &ze );
     BufferString name = ze.name;
     filelist_.add( name );
  }
  CloseZip( hz );
#endif
}


bool ZipUtils::doUnZip( const char* src, const char* dest )
{
#ifdef __win__
    makeFileList( src );
    if ( filelist_.isEmpty() )
    {
	errmsg_ = "Error: ";
	errmsg_ += BufferString( src, " is empty." );
	return false;
    }

    CoInitialize(0); 
    Shell32::IShellDispatch2Ptr shell;
    shell.CreateInstance(__uuidof(Shell32::Shell));
    _bstr_t bs( src );
    _variant_t varsrc ( bs );
    Shell32::FolderPtr srcfolder = shell->NameSpace( varsrc );
    _bstr_t bd( dest );
    _variant_t vardest( bd );
    Shell32::FolderPtr destfolder = shell->NameSpace( vardest );
    Shell32::FolderItemsPtr items = srcfolder->Items();
    long flags = FOF_NOCONFIRMATION | FOF_NOERRORUI;
    HRESULT hres = destfolder->CopyHere( 
	_variant_t((IDispatch*)items,true), flags );
    ::Sleep(1000);
    CoUninitialize(); 
    if ( SUCCEEDED(hres) )
	return true;
    else
    {
	errmsg_ = "Unzip Failed";
	return false;
    }
    
#else
    return false;
#endif
}

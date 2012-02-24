/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          December 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: ziputils.cc,v 1.12 2012-02-24 03:52:19 cvsraman Exp $";

#include "ziputils.h"

#include "file.h"
#include "filepath.h"
#include "strmprov.h"

#define mDirCheck( dir ) \
    if ( !File::exists(dir) ) \
    { \
	errmsg_ = dir; \
	errmsg_ += " does not exist"; \
	return false; \
    } \


ZipUtils::ZipUtils( const char* filelistnm )
    : filelistname_(filelistnm)
    , needfilelist_(!filelistname_.isEmpty())
{}

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
    return doUnZip( src, dest );
}


bool ZipUtils::doZip( const char* src, const char* dest )
{
    return true;
}


bool ZipUtils::doUnZip( const char* src, const char* dest )
{
    bool tempfile = false;
    BufferString tmpfnm( filelistname_ );
    if ( needfilelist_ )
    {
	FilePath listfp( filelistname_ );
	if ( !File::exists( listfp.pathOnly() ) )
	{
	    tempfile = true;
	    FilePath tempfp( File::getTempPath() );
	    tempfp.add( listfp.fileName() );
	    filelistname_ = tempfp.fullPath();
	}
    }

    bool res = false;
#ifdef __win__
    BufferString cmd( "cmd /c unzip -o \"", src );
    cmd.add( "\" -d \"" ).add( dest ).add( "\"");
    if ( needfilelist_ )
	cmd.add( " > " ).add( "\"" ).add( filelistname_ ).add( "\"" );
    res = ExecOSCmd( cmd );
    if ( res && tempfile )
    {
	BufferString cpcmd( "copy \"" );
	cpcmd.add( filelistname_ ) .add( "\" \"" ).add( tmpfnm ).add("\"");
	system( cpcmd );
    }
#else
    BufferString cmd( "unzip -o ", src );
    cmd.add( " -d " ).add( dest ).add( " > " )
	.add( needfilelist_ ? filelistname_ : "/dev/null" );
    res = !system( cmd );
#endif

    if ( res ) return true;

    errmsg_ = "Unzip failed";
    return false;
}

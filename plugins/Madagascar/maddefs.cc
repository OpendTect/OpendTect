
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
-*/

static const char* rcsID = "$Id: maddefs.cc,v 1.2 2007-06-27 16:41:59 cvsbert Exp $";

#include "maddefs.h"
#include "envvars.h"
#include "filegen.h"
#include "filepath.h"
#include "dirlist.h"


ODMad::ProgInfo::ProgInfo( const char* rsfroot )
	: rsfroot_(rsfroot)
{
    if ( rsfroot_.isEmpty() )
	rsfroot_ = GetEnvVar( "RSFROOT" );

    doPreScanCheck();
}


ODMad::ProgInfo::~ProgInfo()
{
    deepErase( defs_ );
}


void ODMad::ProgInfo::doPreScanCheck()
{
    if ( rsfroot_.isEmpty() )
    {
	errmsg_ = "RSFROOT not set - cannot use Madagascar tools";
	return;
    }
    if ( !File_isDirectory(rsfroot_) )
    {
	errmsg_ = "$RSFROOT (Madagascar) is invalid:\n";
	errmsg_ += rsfroot_;
	errmsg_ += "\nThe directory does not exist or cannot be read";
	return;
    }

    FilePath fp( rsfroot_ ); fp.add( "defs" );
    defdir_ = fp.fullPath();

    if ( !File_isDirectory(defdir_) )
    {
	errmsg_ = "Madagascar installation not prepared. Directory:\n";
	errmsg_ += defdir_;
	errmsg_ += "\ndoes not exist. You need to issue the command:\n"
		   "$RSFROOT/bin/sfdoc -t $RSFROOT/defs\n";
    }
}


void ODMad::ProgInfo::doScan()
{
    deepErase( defs_ );
    doPreScanCheck();

    DirList dl( defdir_, DirList::FilesOnly, "*.txt" );
    const int sz = dl.size();
    if ( sz < 1 )
    {
	errmsg_ = "Madagascar Definition directory:\n";
	errmsg_ += defdir_; errmsg_ += "\ncontains no definition files";
	return;
    }

    for ( int idx=0; idx<sz; idx++ )
	addEntry( dl.fullPath(idx) );
}


void ODMad::ProgInfo::addEntry( const char* )
{
}

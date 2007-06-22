
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
-*/

static const char* rcsID = "$Id: maddefs.cc,v 1.1 2007-06-22 12:07:19 cvsbert Exp $";

#include "maddefs.h"


MadagascarDefs::MadagascarDefs( const char* rsfroot )
	: rsfroot_(rsfroot)
{
    if ( rsfroot_.isEmpty() )
	rsfroot_ = GetEnvVar( "RSFROOT" );
    if ( rsfroot_.isEmpty() )
	ErrMsg( "RSFROOT not set - cannot use Madagascar tools" );

    FilePath fp( rsfroot_ ); fp.add( "defs" );
    defdir_ = fp.fullPath();
    reScan();
}


MadagascarDefs::~MadagascarDefs()
{
    deepErase( defs_ );
}


bool MadagascarDefs::reScan()
{
    deepErase( defs_ );

    if ( !File_isDirectory(defdir_) )
    {
	BufferString msg( "Madagascar installation not prepared. Directory:\n");
	msg += defdir_; msg += "\ndoes not exist";
	ErrMsg( msg );
	return false;
    }


    DirList dl( defdir_, DirList::FilesOnly, "*.txt" );
    const int sz = dl.size();
    if ( sz < 1 )
    {
	BufferString msg( "Madagascar Definition directory:\n");
	msg += defdir_; msg += "\ncontains no definition files";
	ErrMsg( msg );
	return false;
    }

    for ( int idx=0; idx<sz; idx++ )
	addEntry( dl.fullPath(idx) );
}

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: od_process_segyio.cc,v 1.8 2012/05/07 11:33:46 cvskris Exp $";

#include "batchprog.h"

#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "oddirs.h"
#include "segybatchio.h"
#include "segydirectdef.h"
#include "segyscanner.h"
#include "segyfiledef.h"
#include "keystrs.h"

#include "prog.h"

bool BatchProgram::go( std::ostream& strm )
{ 
    OD::ModDeps().ensureLoaded("Seis");

    const FixedString task = pars().find( SEGY::IO::sKeyTask() );
    const char* parseerror =  "Cannot parse parameters";
    const bool isps = task == SEGY::IO::sKeyIndexPS();
    const bool isvol = task == SEGY::IO::sKeyIndex3DVol();
    bool is2d = !isvol; MultiID mid;
    pars().getYN( SEGY::IO::sKeyIs2D(), is2d );
    pars().get( sKey::Output, mid );

    if ( isps || isvol )
    {
	if ( mid.isEmpty() )
	{
	    strm << "Parameter file lacks the '" << sKey::Output << " key."
		 << std::endl;
	    return false;
	}

	SEGY::FileSpec filespec;
	if ( !filespec.usePar( pars() ) )
	{
	    strm << "Missing or invalid file name in parameter file"
		 << std::endl;
	    return false;
	}

	FilePath fp ( filespec.fname_ );
	if ( fp.isSubDirOf(GetDataDir()) )
	{
	    BufferString relpath = File::getRelativePath( GetDataDir(),
							  fp.pathOnly() );
	    if ( !relpath.isEmpty() )
	    {
		relpath += "/";
		relpath += fp.fileName();
#ifdef __win__
		replaceCharacter( relpath.buf(), '/', '\\' );  
#endif
		if ( relpath != filespec.fname_ )
		{
		    replaceCharacter( relpath.buf(), '\\', '/' );  
		    filespec.fname_ = relpath;
		}
		pars().set( sKey::FileName, filespec.fname_ );
	    }
	}

	SEGY::FileIndexer indexer( mid, isvol, filespec, is2d, pars() );
	if ( !indexer.execute( &strm ) )
	{
	    strm << indexer.message();
	    IOM().permRemove( mid );
	    return false;
	}

	IOPar report;
	indexer.scanner()->getReport( report );

	if ( indexer.scanner()->warnings().size() == 1 )
	    report.add( "Warning", indexer.scanner()->warnings().get(0) );
	else
	{
	    for ( int idx=0; idx<indexer.scanner()->warnings().size(); idx++ )
	    {
		if ( !idx ) report.add( IOPar::sKeyHdr(), "Warnings" );
		report.add( toString(idx+1 ),
			    indexer.scanner()->warnings().get(idx) );
	    }
	}

	report.write( strm, IOPar::sKeyDumpPretty() );

	return true;
    }

    strm << "Unknown task: " << (task.isEmpty() ? "<empty>" : task)
			     << std::endl;
    return false;
}

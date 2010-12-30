/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: od_process_segyio.cc,v 1.2 2010-12-30 19:31:47 cvskris Exp $";

#include "batchprog.h"

#include "ioman.h"
#include "multiid.h"
#include "segybatchio.h"
#include "segydirectdef.h"
#include "segyscanner.h"
#include "segyfiledef.h"
#include "keystrs.h"

#include "prog.h"

bool BatchProgram::go( std::ostream& strm )
{ 
    const char* parseerror =  "Cannot parse parameters";

    const FixedString task = pars().find( SEGY::IO::sKeyTask() );
    if ( task==SEGY::IO::sKeyIndexPS() )
    {
	bool is2d;
	MultiID mid;
	if ( !pars().getYN( SEGY::IO::sKeyIs2D(), is2d )  ||
	     !pars().get( sKey::Output, mid ) )
	{
	    strm << parseerror;
	    return false;
	}

	BufferString linename;
	if ( is2d && !pars().get( sKey::LineName, linename ) )
	{
	    strm << parseerror;
	    return false;
	}

	SEGY::FileSpec filespec;
	if ( !filespec.usePar( pars() ) )
	{
	    strm << parseerror;
	    return false;
	}

	SEGY::PreStackIndexer indexer( mid, linename, filespec, is2d, pars() );
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
	    char buf[10];
	    for ( int idx=0; idx<indexer.scanner()->warnings().size(); idx++ )
	    {
		if ( !idx ) report.add( IOPar::sKeyHdr(), "Warnings" );
		report.add( toString(idx+1, buf ),
			    indexer.scanner()->warnings().get(idx) );
	    }
	}

	report.write( strm, IOPar::sKeyDumpPretty() );

	return true;
    }

    strm << "Unknown task";
    return false;
}

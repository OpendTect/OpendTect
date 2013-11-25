/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"

#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "multiid.h"
#include "oddirs.h"
#include "segybatchio.h"
#include "segydirectdef.h"
#include "segyscanner.h"
#include "segyfiledef.h"
#include "scaler.h"
#include "seisresampler.h"
#include "seissingtrcproc.h"
#include "keystrs.h"
#include "moddepmgr.h"

#include "prog.h"

static bool doImport( od_ostream& strm, IOPar& iop, bool is2d )
{
    IOPar* outpar = iop.subselect( sKey::Output() );
    if ( !outpar || outpar->isEmpty() )
	{ strm << "Batch parameters 'Ouput' empty" << od_endl; return false; }

    SEGY::FileSpec fs; fs.usePar( iop );
    IOObj* inioobj = fs.getIOObj( true );
    if ( !inioobj )
	{ strm << "Input file spec is not OK" << od_endl; return false; }
    IOObj* outioobj = IOM().get( outpar->find(sKey::ID()) );
    if ( !outioobj )
	{ strm << "Output object spec is not OK" << od_endl; return false; }

    outpar->removeWithKey( sKey::ID() );
	// important! otherwise reader will try to read output ID ...

    Scaler* sclr = Scaler::get( outpar->find(sKey::Scale()) );
    const int nulltrcpol = toInt( outpar->find("Null trace policy") );
    const bool exttrcs = outpar->isTrue( "Extend Traces To Survey Z Range" );
    CubeSampling cs; cs.usePar( *outpar );
    SeisResampler* resmplr = new SeisResampler( cs, is2d );

    SeisSingleTraceProc* stp = new SeisSingleTraceProc( inioobj, outioobj,
				"SEG-Y importer", outpar, "Importing traces" );
    stp->setScaler( sclr );
    stp->skipNullTraces( nulltrcpol < 1 );
    stp->fillNullTraces( nulltrcpol == 2 );
    stp->setExtTrcToSI( exttrcs );
    stp->setResampler( resmplr );
	//TODO resampler stuff
    return stp->go( strm );
}


static bool doScan( od_ostream& strm, IOPar& iop, bool isps, bool is2d )
{
    MultiID mid;
    iop.get( sKey::Output(), mid );
    if ( mid.isEmpty() )
    {
	strm << "Parameter file lacks key '" << sKey::Output() << od_endl;
	return false;
    }

    SEGY::FileSpec filespec;
    if ( !filespec.usePar( iop ) )
    {
	strm << "Missing or invalid file name in parameter file\n";
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
	    relpath.replace( '/', '\\' );
#endif
	    if ( relpath != filespec.fname_ )
	    {
		relpath.replace( '\\', '/' );
		filespec.fname_ = relpath;
	    }
	    iop.set( sKey::FileName(), filespec.fname_ );
	}
    }

    SEGY::FileIndexer indexer( mid, !isps, filespec, is2d, iop );
    if ( !indexer.go(strm) )
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


bool BatchProgram::go( od_ostream& strm )
{
    OD::ModDeps().ensureLoaded("Seis");

    const FixedString task = pars().find( SEGY::IO::sKeyTask() );
    const bool isimport = task == SEGY::IO::sKeyImport();
    bool is2d = false; pars().getYN( SEGY::IO::sKeyIs2D(), is2d );

    if ( isimport )
	return doImport( strm, pars(), is2d );

    const bool ispsindex = task == SEGY::IO::sKeyIndexPS();
    const bool isvolindex = task == SEGY::IO::sKeyIndex3DVol();

    if ( ispsindex || isvolindex )
	return doScan( strm, pars(), ispsindex, is2d );


    strm << "Unknown task: " << (task.isEmpty() ? "<empty>" : task)
			     << od_newline;
    return false;
}

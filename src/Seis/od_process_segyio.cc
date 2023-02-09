/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/


#include "batchprog.h"

#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "multiid.h"
#include "oddirs.h"
#include "ptrman.h"
#include "segybatchio.h"
#include "segydirectdef.h"
#include "segyhdr.h"
#include "segyscanner.h"
#include "segyfiledef.h"
#include "scaler.h"
#include "seisresampler.h"
#include "seissingtrcproc.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "zdomain.h"

#include "prog.h"

#include "seistrc.h"
#include "segytr.h"
#include "seiswrite.h"
#include "seisread.h"

static void splitIOPars( const IOPar& base, ObjectSet<IOPar>& pars, bool is2d )
{
    if ( !is2d ) // Splitting only needed for multi 2D line I/O
    {
	pars += new IOPar( base );
	return;
    }

    BufferStringSet fnms;
    TypeSet<Pos::GeomID> geomids;
    BufferString fnm;
    Pos::GeomID geomid;
    for ( int idx=0; ; idx++ )
    {
	const BufferString key = idx==0 ? "" : toString(idx);
	const bool res =
		base.get( IOPar::compKey(sKey::FileName(),key), fnm ) &&
		base.get( IOPar::compKey( sKey::GeomID(),key), geomid );
	if ( !res )
	    break;

	fnms.add( fnm );
	geomids.add( geomid );
    }

    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	IOPar* par = new IOPar( base );
	par->removeSubSelection( sKey::FileName() );
	par->removeSubSelection( sKey::GeomID() );
	par->set( sKey::FileName(), fnms.get(idx) );
	par->set( sKey::GeomID(), geomids[idx] );
	pars += par;
    }
}


static void writeSEGYHeader( const char* hdr, const IOObj& outioobj )
{
    FilePath sgyhdr = outioobj.fullUserExpr();
    sgyhdr.setExtension( "sgyhdr" );
    od_ostream strm( sgyhdr.fullPath() );
    if ( strm.isOK() )
	strm << hdr << od_endl;
}


static bool doImport( od_ostream& strm, IOPar& iop, bool is2d )
{
    PtrMan<IOPar> outpar = iop.subselect( sKey::Output() );
    if ( !outpar || outpar->isEmpty() )
	{ strm << "Batch parameters 'Ouput' empty" << od_endl; return false; }

    SEGY::FileSpec fs; fs.usePar( iop );
    IOObj* inioobj = fs.getIOObj( true );
    if ( !inioobj )
	{ strm << "Input file spec is not OK" << od_endl; return false; }
    PtrMan<IOObj> outioobj = IOM().get( outpar->find(sKey::ID()) );
    if ( !outioobj )
	{ strm << "Output object spec is not OK" << od_endl; return false; }

    outpar->removeWithKey( sKey::ID() );
	// important! otherwise reader will try to read output ID ...

    RefMan<Coords::CoordSystem> crs = Coords::CoordSystem::createSystem(
								    *outpar );

    PtrMan<SeisSingleTraceProc> stp =
	new SeisSingleTraceProc( *inioobj, *outioobj, "SEG-Y importer", &iop,
				 toUiString("Importing traces") );

    const SeisTrcReader* rdr = stp->reader();
    SeisTrcTranslator* transl =
		    const_cast<SeisTrcTranslator*>(rdr->seisTranslator());
    mDynamicCastGet(SEGYSeisTrcTranslator*,segytr,transl)
    if ( segytr )
	segytr->setCoordSys( crs );

    stp->setProcPars( *outpar, is2d );
    const bool res = stp->go( strm );
    if ( res && segytr && !is2d )
    {
	const SEGY::TxtHeader& th = *segytr->txtHeader();
	BufferString buf; th.getText( buf );
	writeSEGYHeader( buf, *outioobj );
    }

    return res;
}


static bool doExport( od_ostream& strm, IOPar& iop, bool is2d )
{
    PtrMan<IOPar> inppar = iop.subselect( sKey::Input() );
    if ( !inppar || inppar->isEmpty() )
	{ strm << "Batch parameters 'Input' empty" << od_endl; return false; }

    PtrMan<IOPar> outpar = iop.subselect( sKey::Output() );
    if ( !outpar || outpar->isEmpty() )
	{ strm << "Batch parameters 'Ouput' empty" << od_endl; return false; }

    PtrMan<IOObj> inioobj = IOM().get( inppar->find(sKey::ID()) );
    if ( !inioobj )
	{ strm << "Input seismics is not OK" << od_endl; return false; }

    SEGY::FileSpec fs; fs.usePar( *outpar );
    IOObj* outioobj = fs.getIOObj( true );
    if ( !outioobj )
	{ strm << "Output SEG-Y file is not OK" << od_endl; return false; }

    int compnr;
    if ( !inppar->get( sKey::Component(), compnr ) )
	compnr = 0;

    if ( !ZDomain::isSI(inioobj->pars()) )
	ZDomain::Def::get(inioobj->pars()).set( outioobj->pars() );

    SEGY::FilePars fp; fp.usePar( *outpar );
    fp.fillPar( outioobj->pars() );

    RefMan<Coords::CoordSystem> crs = Coords::CoordSystem::createSystem(
								    *outpar );
    PtrMan<SeisSingleTraceProc> stp =
	new SeisSingleTraceProc( *inioobj, *outioobj, "SEG-Y exporter", outpar,
				 mToUiStringTodo("Exporting traces"), compnr );
    const SeisTrcWriter& wrr = stp->writer();
    SeisTrcTranslator* transl =
		    const_cast<SeisTrcTranslator*>(wrr.seisTranslator());
    mDynamicCastGet(SEGYSeisTrcTranslator*,segytr,transl)
    if ( segytr )
	segytr->setCoordSys( crs );
    stp->setProcPars( *outpar, is2d );
    return stp->go( strm );
}


static bool doScan( od_ostream& strm, IOPar& iop, bool isps, bool is2d )
{
    MultiID mid;
    iop.get( sKey::Output(), mid );
    if ( mid.isEmpty() )
    {
	iop.get( IOPar::compKey(sKey::Output(),sKey::ID()), mid );
	if ( mid.isEmpty() )
	{
	    strm << "Parameter file lacks key 'Output[.ID]'" << od_endl;
	    return false;
	}
    }

    SEGY::FileSpec filespec;
    if ( !filespec.usePar(iop) )
    {
	strm << "Missing or invalid file name in parameter file\n";
	return false;
    }
    SEGY::FileSpec::makePathsRelative( iop );

    SEGY::FileIndexer indexer( mid, !isps, filespec, is2d, iop );
    if ( !indexer.go(strm) )
    {
	strm << indexer.uiMessage().getFullString();
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


mLoad1Module("Seis")

bool BatchProgram::doWork( od_ostream& strm )
{
    const FixedString task = pars().find( SEGY::IO::sKeyTask() );
    const bool isimport = task == SEGY::IO::sKeyImport();
    const bool isexport = task == SEGY::IO::sKeyExport();
    const bool ispsindex = task == SEGY::IO::sKeyIndexPS();
    const bool isvolindex = task == SEGY::IO::sKeyIndex3DVol();
    bool is2d = false; pars().getYN( SEGY::IO::sKeyIs2D(), is2d );

    ManagedObjectSet<IOPar> allpars;
    splitIOPars( pars(), allpars, is2d );

    const bool ismultiline = allpars.size() > 1;
    for ( int idx=0; idx<allpars.size(); idx++ )
    {
	IOPar& curpars = *allpars[idx];
	if ( ismultiline )
	{
	    strm << "Line " << idx+1 << " of " << allpars.size() << od_endl;
	    Pos::GeomID geomid;
	    BufferString linename;
	    if ( curpars.get(sKey::GeomID(),geomid) )
		linename = Survey::GM().getName( geomid );

	    if ( !linename.isEmpty() )
		strm << "Line name: " << linename << od_endl;

	    const BufferString filename = curpars.find( sKey::FileName() );
	    if ( !filename.isEmpty() )
		strm << "File name: " << filename << od_endl;
	}

	if ( isimport )
	    doImport( strm, curpars, is2d );
	else if ( isexport )
	    doExport( strm, curpars, is2d );
	else if ( ispsindex || isvolindex )
	    doScan( strm, curpars, ispsindex, is2d );
	else
	    strm << "Unknown task: " << (task.isEmpty() ? "<empty>" : task)
		 << od_newline;
    }

    return true;
}

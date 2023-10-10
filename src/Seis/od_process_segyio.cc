/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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

static IOPar* cleanInputPar( const IOPar& iop )
{
    auto* ret = new IOPar( iop );
    ret->removeSubSelection( sKey::Output() );
    OS::CommandExecPars execpars;
    execpars.usePar( *ret );
    execpars.removeFromPar( *ret );
    ret->removeWithKey( sKey::LogFile() );
    ret->removeSubSelection( "Program");
    ret->removeWithKey( sKey::DataRoot() );
    ret->removeWithKey( sKey::Survey() );

    return ret;
}


static void splitIOPars( const IOPar& base, ObjectSet<IOPar>& pars )
{
    PtrMan<IOPar> inppar = base.subselect( sKey::Input() );
    if ( !inppar )
	inppar = cleanInputPar( base );

    BufferStringSet fnms;
    TypeSet<Pos::GeomID> geomids;
    BufferString fnm;
    Pos::GeomID geomid;
    for ( int idx=0; ; idx++ )
    {
	const BufferString key = idx==0 ? "" : toString(idx);
	const bool res =
		inppar->get( IOPar::compKey(sKey::FileName(),key), fnm ) &&
		inppar->get( IOPar::compKey( sKey::GeomID(),key), geomid );
	if ( !res )
	    break;

	fnms.add( fnm );
	geomids.add( geomid );
    }

    PtrMan<IOPar> outpar = base.subselect( sKey::Output() );

    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	IOPar inpiop( *inppar.ptr() );
	inpiop.removeSubSelection( sKey::FileName() );
	inpiop.removeSubSelection( sKey::GeomID() );
	inpiop.set( sKey::FileName(), fnms.get(idx) );
	inpiop.set( sKey::GeomID(), geomids[idx] );

	auto* par = new IOPar( base.name() );
	par->mergeComp( inpiop, sKey::Input() );
	if ( outpar && !outpar->isEmpty() )
	    par->mergeComp( *outpar.ptr(), sKey::Output() );
	pars += par;
    }
}


static bool doImport( od_ostream& strm, const IOPar& iop, Seis::GeomType gt )
{
    PtrMan<IOPar> inppar = iop.subselect( sKey::Input() );
    if ( !inppar )
	inppar = cleanInputPar( iop );

    PtrMan<IOPar> outpar = iop.subselect( sKey::Output() );
    if ( !outpar || outpar->isEmpty() )
    {
	strm << "Batch parameters 'Output' empty" << od_endl;
	return false;
    }

    SEGY::FileSpec fs;
    fs.usePar( *inppar.ptr() );
    PtrMan<IOObj> inioobj = fs.getIOObj( true );
    if ( !inioobj )
    {
	strm << "Input file spec is not OK" << od_endl;
	return false;
    }

    MultiID mid;
    outpar->get( sKey::ID(), mid );
    if ( mid.isUdf() )
    {
	strm << "MultiID is not defined" << od_endl;
	return false;
    }

    PtrMan<IOObj> outioobj = IOM().get( mid );
    if ( !outioobj )
    {
	strm << "Output object spec is not OK" << od_endl;
	return false;
    }

    Pos::GeomID gid = Seis::is3D(gt) ? Survey::default3DGeomID()
				     : mUdf(Pos::GeomID);
    if ( Seis::is2D(gt) )
	inppar->get( sKey::GeomID(), gid );

    SeisStoreAccess::Setup ssasuin( *inioobj, gid, &gt );
    ssasuin.usePar( *inppar );
    SeisStoreAccess::Setup ssasuout( *outioobj, gid, &gt );
    ssasuout.usePar( *outpar );
    if ( ssasuout.seldata_ )
	ssasuin.seldata( ssasuout.seldata_ );

    PtrMan<SeisSingleTraceProc> stp = new SeisSingleTraceProc( ssasuin,
					ssasuout, "SEG-Y importer",
					toUiString("Importing traces") );
    if ( !stp->isOK() )
    {
	strm.add( stp->errMsg() );
	return false;
    }

    const bool is2d = Seis::is2D( gt );
    stp->setProcPars( *outpar, is2d );
    const bool res = stp->go( strm );
    if ( !res )
	strm << stp->errMsg() << od_endl;

    if ( gt != Seis::Vol )
	return res;

    //Reader is gone after stp->go, need a new one
    SeisTrcReader rdr( ssasuin );
    if ( !rdr.prepareWork(Seis::PreScan) )
	return res;

    SEGYSeisTrcTranslator::writeSEGYHeader( rdr,
			   outioobj->fullUserExpr() );
    return res;
}


static bool doExport( od_ostream& strm, const IOPar& iop,
		      Seis::GeomType gt )
{
    PtrMan<IOPar> inppar = iop.subselect( sKey::Input() );
    if ( !inppar )
	inppar = cleanInputPar( iop );

    PtrMan<IOPar> outpar = iop.subselect( sKey::Output() );
    if ( !outpar || outpar->isEmpty() )
    {
	strm << "Batch parameters 'Output' empty" << od_endl;
	return false;
    }

    MultiID mid;
    inppar->get( sKey::ID(), mid );
    if ( mid.isUdf() )
    {
	strm << "MultiID is not defined" << od_endl;
	return false;
    }

    PtrMan<IOObj> inioobj = IOM().get( mid );
    if ( !inioobj )
    {
	strm << "Input seismics is not OK" << od_endl;
	return false;
    }

    SEGY::FileSpec fs;
    fs.usePar( *outpar );
    PtrMan<IOObj> outioobj = fs.getIOObj( true );
    if ( !outioobj )
    {
	strm << "Output SEG-Y file is not OK" << od_endl;
	return false;
    }

    int compnr;
    if ( !inppar->get(sKey::Component(),compnr) )
	compnr = 0;

    SeisStoreAccess::zDomain( inioobj ).fillPar( outioobj->pars() );

    Pos::GeomID gid = Seis::is3D(gt) ? Survey::default3DGeomID()
				     : mUdf(Pos::GeomID);
    if ( Seis::is2D(gt) )
	inppar->get( sKey::GeomID(), gid );

    SeisStoreAccess::Setup ssasuin( *inioobj, gid, &gt );
    ssasuin.usePar( *inppar );
    ssasuin.compnr( compnr );
    SeisStoreAccess::Setup ssasuout( *outioobj, gid, &gt );
    ssasuout.usePar( *outpar );
    ssasuout.compnr( compnr );
    if ( ssasuout.seldata_ )
	ssasuin.seldata( ssasuout.seldata_ );

    PtrMan<SeisSingleTraceProc> stp = new SeisSingleTraceProc( ssasuin,
			    ssasuout, "SEG-Y exporter",
			    toUiString("Exporting traces") );
    if ( !stp->isOK() )
    {
	strm.add( stp->errMsg() );
	return false;
    }

    stp->setProcPars( *outpar, Seis::is2D(gt) );
    const bool res = stp->go( strm );
    if ( !res )
	strm << stp->errMsg() << od_endl;

    return res;
}


static bool doScan( od_ostream& strm, const IOPar& iop, bool isps,
		    Seis::GeomType gt )
{
    PtrMan<IOPar> inppar = iop.subselect( sKey::Input() );
    if ( !inppar )
	inppar = cleanInputPar( iop );

    MultiID mid;
    PtrMan<IOPar> outpar = iop.subselect( sKey::Output() );
    if ( !outpar || outpar->isEmpty() )
	iop.get( sKey::Output(), mid );
    else
	outpar->get( sKey::ID(), mid );

    if ( mid.isUdf() )
    {
	strm << "Parameter file lacks key 'Output[.ID]'" << od_endl;
	return false;
    }

    SEGY::FileSpec filespec;
    if ( !filespec.usePar(*inppar) )
    {
	strm << "Missing or invalid file name in parameter file\n";
	return false;
    }

    SEGY::FileIndexer indexer( mid, !isps, filespec, Seis::is2D(gt), *inppar );
    if ( !indexer.go(strm) )
    {
	strm << indexer.uiMessage() << od_endl;
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
    const IOPar& iop = pars();
    const BufferString task = iop.find( SEGY::IO::sKeyTask() );
    const bool isimport = task.isEqual( SEGY::IO::sKeyImport() );
    const bool isexport = task.isEqual( SEGY::IO::sKeyExport() );
    const bool ispsindex = task.isEqual( SEGY::IO::sKeyIndexPS() );
    const bool isvolindex = task.isEqual( SEGY::IO::sKeyIndex3DVol() );

    Seis::GeomType gt;
    if ( !Seis::getFromPar(iop,gt) )
    {
	bool is2d = false;
	if ( iop.getYN(SEGY::IO::sKeyIs2D(),is2d) )
	    gt = Seis::geomTypeOf( is2d, ispsindex );
	else
	{
	    strm << "Cannot determine geometry type" << od_endl;
	    return false;
	}
    }

    ManagedObjectSet<IOPar> allpars;
    if ( Seis::is2D(gt) ) // Splitting only needed for multi 2D line I/O
	splitIOPars( iop, allpars );
    else
	allpars += new IOPar( iop );

    const bool ismultiline = allpars.size() > 1;
    for ( int idx=0; idx<allpars.size(); idx++ )
    {
	const IOPar& curpars = *allpars[idx];
	if ( ismultiline )
	{
	    strm << "Line " << idx+1 << " of " << allpars.size() << od_endl;
	    Pos::GeomID geomid;
	    BufferString linename;
	    if ( curpars.get(IOPar::compKey(sKey::Input(),sKey::GeomID()),
			      geomid) )
		linename = Survey::GM().getName( geomid );

	    if ( !linename.isEmpty() )
		strm << "Line name: " << linename << od_endl;

	    const BufferString filename =
		curpars.find( IOPar::compKey(sKey::Input(),sKey::FileName()) );
	    if ( !filename.isEmpty() )
		strm << "File name: " << filename << od_endl;
	}

	if ( isimport )
	    doImport( strm, curpars, gt );
	else if ( isexport )
	    doExport( strm, curpars, gt );
	else if ( ispsindex || isvolindex )
	    doScan( strm, curpars, ispsindex, gt );
	else
	    strm << "Unknown task: " << (task.isEmpty() ? "<empty>"
							: task.buf() )
		 << od_newline;
    }

    return true;
}

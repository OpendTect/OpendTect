/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: seisjobexecprov.cc,v 1.5 2004-11-10 14:19:13 bert Exp $";

#include "seisjobexecprov.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "seissingtrcproc.h"
#include "jobdescprov.h"
#include "jobrunner.h"
#include "ctxtioobj.h"
#include "cbvsreader.h"
#include "ioman.h"
#include "iostrm.h"
#include "iopar.h"
#include "iodirentry.h"
#include "hostdata.h"
#include "filegen.h"
#include "filepath.h"
#include "keystrs.h"
#include "strmprov.h"
#include "ptrman.h"
#include <iostream>

const char* SeisJobExecProv::sKeyTmpStor = "Temporary storage location";
const char* SeisJobExecProv::sKeySeisOutIDKey = "Output Seismics Key";


SeisJobExecProv::SeisJobExecProv( const char* prognm, const IOPar& iniop )
	: progname_(prognm)
    	, iopar_(*new IOPar(iniop))
    	, outioobjpars_(*new IOPar)
    	, ctio_(*new CtxtIOObj(SeisTrcTranslatorGroup::ioContext()) )
    	, nrrunners_(0)
    	, is2d_(false)
{
    ctio_.ctxt.trglobexpr = "CBVS";
    seisoutkey_ = outputKey( iopar_ );

    const char* res = iopar_.find( seisoutkey_ );
    IOObj* outioobj = IOM().get( res );
    if ( !outioobj )
	errmsg_ = "Cannot find specified output seismic ID";
    else
    {
	seisoutid_ = outioobj->key();
	outioobjpars_ = outioobj->pars();
	is2d_ = SeisTrcTranslator::is2D(*outioobj);
	delete outioobj;
    }
}


const char* SeisJobExecProv::outputKey( const IOPar& iopar )
{
    const char* res = iopar.find( sKeySeisOutIDKey );
    if ( !res ) res = "Output.1.Seismic ID";
    return res;
}


JobDescProv* SeisJobExecProv::mk2DJobProv()
{
    BufferStringSet nms;
    const char* lskey = iopar_.find( "LineSet Key" );
    if ( !lskey ) lskey = "Input Seismics.ID";
    lskey = iopar_.find( lskey );
    if ( !lskey ) lskey = iopar_.find( "Attributes.0.Definition" );
    IOObj* ioobj = IOM().get( lskey );
    if ( ioobj && SeisTrcTranslator::is2D(*ioobj) )
    {
	Seis2DLineSet ls( ioobj->fullUserExpr(true) );
	for ( int idx=0; idx<ls.nrLines(); idx++ )
	    nms.addIfNew( ls.lineName(idx) );
    }
    return new KeyReplaceJobDescProv( iopar_, "Output.1.Line key", nms );
}


bool SeisJobExecProv::isRestart() const
{
    const char* res = iopar_.find( sKeyTmpStor );
    return res && File_isDirectory(res);
}


JobDescProv* SeisJobExecProv::mk3DJobProv()
{
    const char* res = iopar_.find( sKeyTmpStor );
    if ( !res )
    {
	iopar_.set( sKeyTmpStor, getDefTempStorDir() );
	res = iopar_.find( sKeyTmpStor );
    }
    const bool havetempdir = File_isDirectory(res);

    TypeSet<int> inlnrs;
    TypeSet<int>* ptrnrs = 0;
    const char* rgkey = iopar_.find( "Inline Range Key" );
    if ( !rgkey ) rgkey = "Output.1.In-line range";
    if ( havetempdir )
    {
	getMissingLines( inlnrs, rgkey );
	ptrnrs = &inlnrs;
    }
    else if ( !File_createDir(res,0) )
    {
	errmsg_ = "Cannot create data directory in Temporary storage dir";
	return 0;
    }

    MultiID key = tempStorID( true );
    if ( key == "" )
	return 0;
    iopar_.set( seisoutkey_, key );

    return ptrnrs ? new InlineSplitJobDescProv( iopar_, *ptrnrs, rgkey )
		  : new InlineSplitJobDescProv( iopar_, rgkey );
}


JobRunner* SeisJobExecProv::getRunner()
{
    if ( is2d_ && nrrunners_ > 0 ) return 0;

    JobDescProv* jdp = is2d_ ? mk2DJobProv() : mk3DJobProv();
    if ( jdp && jdp->nrJobs() == 0 )
    {
	delete jdp; jdp = 0;
	errmsg_ = "No lines to process";
    }

    if ( jdp )
    {
	nrrunners_++;
	return new JobRunner( jdp, progname_ );
    }

    return 0;
}


BufferString SeisJobExecProv::getDefTempStorDir( const char* pth )
{
    const bool havepth = pth && *pth;
    FilePath fp( havepth ? pth : GetDataDir() );
    if ( !havepth )
	fp.add( "Seismics" );

    BufferString stordir = "Proc_";
    stordir += HostData::localHostName();
    stordir += "_";
    stordir += getPID();

    fp.add( stordir );
    return fp.fullPath();
}


void SeisJobExecProv::getMissingLines( TypeSet<int>& inlnrs,
					 const char* rgkey ) const
{
    FilePath basefp( iopar_.find(sKeyTmpStor) );

    InlineSplitJobDescProv jdp( iopar_, rgkey );
    StepInterval<int> inls; jdp.getRange( inls );
    int lastgood = inls.start - inls.step;
    for ( int inl=inls.start; inl<=inls.stop; inl+=inls.step )
    {
	FilePath fp( basefp );
	BufferString fnm( "i." ); fnm += inl;
	fp.add( fnm ); fnm = fp.fullPath();
	StreamData sd = StreamProvider( fnm ).makeIStream();
	bool isok = sd.usable();
	if ( isok )
	{
	    CBVSReader rdr( sd.istrm, true ); // stream closed by reader
	    isok = !rdr.errMsg() || !*rdr.errMsg();
	    if ( isok )
		isok = rdr.info().geom.start.crl || rdr.info().geom.start.crl;
	}
	if ( !isok )
	    inlnrs += inl;
    }
}


MultiID SeisJobExecProv::tempStorID( bool create ) const
{
    FilePath fp( iopar_.find(sKeyTmpStor) );
    fp.add( "i.*" );

    // Is there already an entry?
    IOM().to( ctio_.ctxt.stdSelKey() );
    IODirEntryList el( IOM().dirPtr(), ctio_.ctxt );
    const BufferString fnm( fp.fullPath() );
    for ( int idx=0; idx<el.size(); idx++ )
    {
	const IOObj* ioobj = el[idx]->ioobj;
	if ( !ioobj ) continue;
	mDynamicCastGet(const IOStream*,iostrm,ioobj)
	if ( !iostrm || !iostrm->isMulti() ) continue;

	if ( fnm == iostrm->fileName() )
	    return iostrm->key();
    }

    MultiID ret;
    if ( !create )
	return ret;

    fp.setFileName( 0 );
    BufferString objnm( "~" );
    objnm += fp.fileName();
    ctio_.setName( objnm );
    IOM().getEntry( ctio_ );
    if ( !ctio_.ioobj )
	errmsg_ = "Cannot create temporary object for seismics";
    else
    {
	ret = ctio_.ioobj->key();
	ctio_.ioobj->pars() = outioobjpars_;
	IOM().commitChanges( *ctio_.ioobj );
	ctio_.setObj(0);
    }

    return ret;
}


Executor* SeisJobExecProv::getPostProcessor()
{
    if ( is2d_ ) return 0;

    MultiID tmpkey( iopar_.find(seisoutkey_) );
    PtrMan<IOObj> inioobj = IOM().get( tmpkey );
    PtrMan<IOObj> outioobj = IOM().get( seisoutid_ );
    return new SeisSingleTraceProc( inioobj, outioobj,
				    "Data transfer", &iopar_,
				    "Writing results to output cube" );
}


bool SeisJobExecProv::removeTempSeis()
{
    if ( is2d_ ) return true;

    MultiID tmpkey( iopar_.find(seisoutkey_) );
    PtrMan<IOObj> ioobj = IOM().get( tmpkey );
    if ( !ioobj ) return true;

    FilePath fp( ioobj->fullUserExpr(true) );
    IOM().permRemove( tmpkey );

    fp.setFileName(0);
    return File_remove(fp.fullPath().buf(),YES);
}

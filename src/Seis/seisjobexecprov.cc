/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seisjobexecprov.h"
#include "seiscbvs.h"
#include "seis2dline.h"
#include "seissingtrcproc.h"
#include "posinfo2d.h"
#include "jobdescprov.h"
#include "jobrunner.h"
#include "ctxtioobj.h"
#include "cbvsreader.h"
#include "ioman.h"
#include "iostrm.h"
#include "iopar.h"
#include "iodirentry.h"
#include "oddirs.h"
#include "hostdata.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "strmprov.h"
#include "ptrman.h"
#include "survinfo.h"
#include "surv2dgeom.h"
#include "cubesampling.h"
#include "separstr.h"
#include <iostream>
#include <sstream>

const char* SeisJobExecProv::sKeySeisOutIDKey()	    { return "Output Seismics Key"; }
const char* SeisJobExecProv::sKeyOutputLS()	    { return "Output Line Set"; }
const char* SeisJobExecProv::sKeyWorkLS()	    { return "Work Line Set"; }

static const char* sKeyProcIs2D = "Processing is 2D";
#define mOutKey(s) IOPar::compKey("Output.0",s)

const BufferString& getOutSubSelKey()
{
    static const BufferString outsubselkey(
				IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    return outsubselkey;
}

#define mOutSubKey(s) IOPar::compKey(getOutSubSelKey().buf(),s)


SeisJobExecProv::SeisJobExecProv( const char* prognm, const IOPar& iniop )
	: progname_(prognm)
    	, iopar_(*new IOPar(iniop))
    	, outioobjpars_(*new IOPar)
    	, ctio_(*new CtxtIOObj(SeisTrcTranslatorGroup::ioContext()) )
    	, nrrunners_(0)
    	, is2d_(false)
    	, outls_(0)
{
    ctio_.ctxt.toselect.allowtransls_ = CBVSSeisTrcTranslator::translKey();
    seisoutkey_ = outputKey( iopar_ );

    FixedString res = iopar_.find( seisoutkey_ );
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


SeisJobExecProv::~SeisJobExecProv()
{
    delete outls_;
    delete &iopar_;
    delete &outioobjpars_;
    delete &ctio_;
}


const char* SeisJobExecProv::outputKey( const IOPar& iopar )
{
    static BufferString res;
    res = iopar.find( sKeySeisOutIDKey() ).str();
    if ( res.isEmpty() ) res = mOutKey("Seismic.ID");
    return res.buf();
}


JobDescProv* SeisJobExecProv::mk2DJobProv()
{
    FixedString restkey = iopar_.find( sKeyProcIs2D );
    const bool isrestart = restkey && *restkey == 'Y';
    iopar_.set( sKeyProcIs2D, "Yes" );

    // Allow alternative keying via input key
    const char* inpkeyword = iopar_.find( "Input Key" );
    if ( !inpkeyword ) inpkeyword = "Input Line Set";
    const char* ioobjkey = iopar_.find( inpkeyword );
    if ( !ioobjkey ) ioobjkey = iopar_.find( "Input Seismics.ID" );

    BufferStringSet nms;
    IOObj* ioobj = IOM().get( ioobjkey );
    if ( ioobj && SeisTrcTranslator::is2D(*ioobj) )
    {
	Seis2DLineSet* inpls = new Seis2DLineSet( ioobj->fullUserExpr(true) );
	for ( int idx=0; idx<inpls->nrLines(); idx++ )
	    nms.addIfNew( inpls->lineName(idx) );
	FixedString attrnm = iopar_.find( sKey::Target() );

	if ( isrestart )
	{
	    S2DPOS().setCurLineSet( inpls->name() );
	    for ( int idx=0; idx<nms.size(); idx++ )
	    {
		LineKey lk( nms.get(idx) );
		lk.setAttrName( attrnm );
		const int lidx = inpls->indexOf( lk );
		if ( lidx >= 0 )
		{
		    PosInfo::Line2DData geom( lk.lineName() );
		    if ( S2DPOS().getGeometry(geom)
			 && !geom.isEmpty() )
		    {
			nms.removeSingle( idx );
			idx--;
		    }
		}
	    }
	    if ( nms.size() < 1 )
	    {
		// Hmm - all already done. Then probably (s)he wants to
		// re-process, possibly with new attrib definition
		for ( int idx=0; idx<inpls->nrLines(); idx++ )
		    nms.addIfNew( inpls->lineName(idx) );
	    }
	}

	// Because we may be going drastically concurrent, we'd better
	// ensure we have the line set ready.
	// This is crucial in the war against NFS attribute caching
	const char* outkeyword = iopar_.find( "Output Key" );
	if ( !outkeyword ) outkeyword = "Output.0.Seismic.ID";
	ioobjkey = iopar_.find( outkeyword );
	delete outls_; outls_ = inpls;
	BufferString datatype;
	if ( ioobjkey )
	{
	    IOObj* outioobj = IOM().get( ioobjkey );
	    if ( outioobj && outioobj->key() != ioobj->key() )
	    {
		outls_ = new Seis2DLineSet( outioobj->fullUserExpr(true) );
		delete outioobj;
	    }
	    //Look for DataType specification
	    SeparString sepstr( ioobjkey, '|' );
	    if ( sepstr[2] && *sepstr[2] )
		datatype += sepstr[2];
	}
	inpls->addLineKeys( *outls_, attrnm, 0, datatype.buf() );
	if ( inpls != outls_ )
	    delete inpls;
    }
    delete ioobj;

    BufferString parkey( mOutKey(sKey::LineKey()) );
    KeyReplaceJobDescProv* ret
	= new KeyReplaceJobDescProv( iopar_, parkey, nms );
    ret->objtyp_ = "Line";
    return ret;
}


bool SeisJobExecProv::emitLSFile( const char* fnm ) const
{
    if ( !outls_ ) return false;

    StreamData sd = StreamProvider(fnm).makeOStream();
    if ( !sd.usable() )
	return false;

    outls_->putTo( *sd.ostrm );
    sd.close();
    return !File::isEmpty( fnm );
}


void SeisJobExecProv::preparePreSet( IOPar& iop, const char* reallskey ) const
{
    if ( outls_ )
	outls_->preparePreSet( iop, reallskey );
}


bool SeisJobExecProv::isRestart() const
{
    const char* res = iopar_.find( sKey::TmpStor() );
    if ( !res )
	return iopar_.find( sKeyProcIs2D );

    return File::isDirectory(res);
}


#define mSetInlsPerJob(n) iopar_.set( "Nr of Inlines per Job", n )

JobDescProv* SeisJobExecProv::mk3DJobProv( int nrinlperjob )
{
    const char* tmpstordir = iopar_.find( sKey::TmpStor() );
    if ( !tmpstordir )
    {
	iopar_.set( sKey::TmpStor(), getDefTempStorDir() );
	tmpstordir = iopar_.find( sKey::TmpStor() );
    }
    const bool havetempdir = File::isDirectory(tmpstordir);

    TypeSet<int> inlnrs;
    TypeSet<int>* ptrnrs = 0;

    mSetInlsPerJob( nrinlperjob );
    InlineSplitJobDescProv jdp( iopar_, 0 );
    jdp.getRange( todoinls_ );

    if ( havetempdir )
    {
	getMissingLines( inlnrs );
	ptrnrs = &inlnrs;
	mSetInlsPerJob( 1 );
    }
    else if ( !File::createDir(tmpstordir) )
    {
	errmsg_ = "Cannot create data directory in Temporary storage dir";
	return 0;
    }

    tmpstorid_ = tempStorID();
    if ( tmpstorid_.isEmpty() )
	return 0;

    IOPar jpiopar( iopar_ );
    jpiopar.set( seisoutkey_, tmpstorid_ );
    jpiopar.set( mOutSubKey(sKey::Type()), sKey::Range() );

    return ptrnrs ? new InlineSplitJobDescProv( jpiopar, *ptrnrs, 0 )
		  : new InlineSplitJobDescProv( jpiopar, 0 );
}


JobRunner* SeisJobExecProv::getRunner( int nrinlperjob )
{
    if ( is2d_ && nrrunners_ > 0 ) return 0;

    JobDescProv* jdp = is2d_ ? mk2DJobProv() : mk3DJobProv( nrinlperjob );

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
    stordir += GetPID();

    fp.add( stordir );
    return fp.fullPath();
}


void SeisJobExecProv::getMissingLines( TypeSet<int>& inlnrs ) const
{
    FilePath basefp( iopar_.find(sKey::TmpStor()) );

    for ( int inl=todoinls_.start; inl<=todoinls_.stop; inl+=todoinls_.step )
    {
	BufferString fnm( "i." ); fnm += inl;
	FilePath fp( basefp, fnm );
	fnm = fp.fullPath();
	StreamData sd = StreamProvider( fnm ).makeIStream();
	bool isok = sd.usable();
	if ( isok )
	{
	    CBVSReader rdr( sd.istrm, false ); // stream closed by reader
	    isok = !rdr.errMsg();
	    if ( isok )
		isok = rdr.info().geom_.start.crl || rdr.info().geom_.start.crl;
	}
	if ( !isok )
	    inlnrs += inl;
    }
}


MultiID SeisJobExecProv::tempStorID() const
{
    FilePath fp( iopar_.find(sKey::TmpStor()) );

    // Is there already an entry?
    IOM().to( ctio_.ctxt.getSelKey() );
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
	mDynamicCastGet(IOStream*,iostrm,ctio_.ioobj)
	fp.add( "i.*" );
	if ( todoinls_.start != todoinls_.stop || todoinls_.start != 0 )
	    iostrm->fileNumbers() = todoinls_;
	else
	{
	    // That cannot be right.
	    StepInterval<int> fnrs;
	    fnrs.start = SI().sampling(false).hrg.start.inl;
	    fnrs.stop = SI().sampling(false).hrg.stop.inl;
	    fnrs.step = SI().sampling(false).hrg.step.inl;
	    iostrm->fileNumbers() = fnrs;
	}
	iostrm->setFileName( fp.fullPath() );
	IOM().commitChanges( *iostrm );
	ctio_.setObj(0);
    }

    return ret;
}


Executor* SeisJobExecProv::getPostProcessor()
{
    if ( is2d_ ) return 0;

    PtrMan<IOObj> inioobj = IOM().get( tmpstorid_ );
    PtrMan<IOObj> outioobj = IOM().get( seisoutid_ );
    return new SeisSingleTraceProc( inioobj, outioobj,
				    "Data transfer", &iopar_,
				    "Writing results to output cube" );
}


bool SeisJobExecProv::removeTempSeis()
{
    if ( is2d_ ) return true;

    PtrMan<IOObj> ioobj = IOM().get( tmpstorid_ );
    if ( !ioobj ) return true;

    FilePath fp( ioobj->fullUserExpr(true) );
    IOM().permRemove( tmpstorid_ );

    if ( fp.fileName() == "i.*" )
	fp.setFileName(0);
    return File::remove( fp.fullPath().buf() );
}

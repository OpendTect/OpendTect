/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisjobexecprov.h"

#include "batchjobdispatch.h"
#include "cbvsreader.h"
#include "ctxtioobj.h"
#include "genc.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "iostrm.h"
#include "iopar.h"
#include "jobdescprov.h"
#include "jobrunner.h"
#include "hostdata.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "posinfo2d.h"
#include "posinfo2dsurv.h"
#include "ptrman.h"
#include "seiscbvs.h"
#include "seis2ddata.h"
#include "seissingtrcproc.h"
#include "strmdata.h"
#include "separstr.h"
#include "survinfo.h"
#include "systeminfo.h"
#include "trckeyzsampling.h"

const char* SeisJobExecProv::sKeySeisOutIDKey()	{ return "Output Seismics Key";}
const char* SeisJobExecProv::sKeyOutputLS()	{ return "Output Line Set"; }
const char* SeisJobExecProv::sKeyWorkLS()	{ return "Work Line Set"; }

static const char* sKeyProcIs2D = "Processing is 2D";
#define mOutKey(s) IOPar::compKey("Output.0",s)

const OD::String& getOutSubSelKey()
{
    mDefineStaticLocalObject( const BufferString, outsubselkey,
			(IOPar::compKey(sKey::Output(),sKey::Subsel())) );
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
	, outds_(0)
{
    seisoutkey_ = outputKey( iopar_ );

    MultiID mid;
    iopar_.get( seisoutkey_, mid );
    IOObj* outioobj = IOM().get( mid );
    if ( !outioobj )
	errmsg_ = tr("Cannot find specified output seismic ID");
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
    delete outds_;
    delete &iopar_;
    delete &outioobjpars_;
    delete &ctio_;
}


const char* SeisJobExecProv::outputKey( const IOPar& iopar )
{
    mDeclStaticString( res );
    if ( iopar.hasKey(sKeySeisOutIDKey()) )
	res = sKeySeisOutIDKey();
    else
	res = mOutKey("Seismic.ID");
    return res.buf();
}


JobDescProv* SeisJobExecProv::mk2DJobProv()
{
    iopar_.set( sKeyProcIs2D, "Yes" );
    Line2DSubselJobDescProv* ret = new Line2DSubselJobDescProv( iopar_ );
    return ret;
}


bool SeisJobExecProv::emitLSFile( const char* fnm ) const
{
    if ( !outds_ ) return false;

    od_ostream strm( fnm );
    if ( !strm.isOK() )
	return false;

    return !File::isEmpty( fnm );
}


void SeisJobExecProv::preparePreSet( IOPar& iop, const char* reallskey ) const
{}


bool SeisJobExecProv::isRestart( const IOPar& iop )
{
    const BufferString res = iop.find( sKey::TmpStor() );
    if ( res.isEmpty() )
	return iop.hasKey( sKeyProcIs2D );

    return File::isDirectory( res );
}


#define mSetInlsPerJob(n) iopar_.set( "Nr of Inlines per Job", n )

JobDescProv* SeisJobExecProv::mk3DJobProv( int nrinlperjob )
{
    BufferString tmpstordir = iopar_.find( sKey::TmpStor() );
    if ( tmpstordir.isEmpty() )
    {
	iopar_.set( sKey::TmpStor(), getDefTempStorDir() );
	tmpstordir = iopar_.find( sKey::TmpStor() );
    }
    const bool havetempdir = File::isDirectory(tmpstordir);

    TypeSet<int> inlnrs;
    TypeSet<int>* ptrnrs = 0;

    mSetInlsPerJob( nrinlperjob );
    InlineSplitJobDescProv jdp( iopar_ );
    jdp.getRange( todoinls_ );

    if ( havetempdir )
    {
	getMissingLines( inlnrs );
	ptrnrs = &inlnrs;
    }
    else if ( !File::createDir(tmpstordir) )
    {
	errmsg_ = uiStrings::phrCannotCreateDirectory(toUiString(tmpstordir));
	return 0;
    }

    tmpstorid_ = tempStorID();
    if ( tmpstorid_.isUdf() )
	return 0;

    IOPar jpiopar( iopar_ );
    jpiopar.set( seisoutkey_, tmpstorid_ );
    jpiopar.set( mOutSubKey(sKey::Type()), sKey::Range() );

    return ptrnrs ? new InlineSplitJobDescProv( jpiopar, *ptrnrs )
		  : new InlineSplitJobDescProv( jpiopar );
}


JobRunner* SeisJobExecProv::getRunner( int nrinlperjob )
{
    if ( is2d_ && nrrunners_ > 0 ) return 0;

    JobDescProv* jdp = is2d_ ? mk2DJobProv() : mk3DJobProv( nrinlperjob );

    if ( jdp && jdp->nrJobs() == 0 )
    {
	delete jdp; jdp = 0;
	errmsg_ = tr("No lines to process");
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

    BufferString stordir( "Proc_" );
    stordir.add( System::localHostNameWoDomain() )
	   .add( "_" ).add( GetPID() );

    fp.add( stordir );
    return fp.fullPath();
}


void SeisJobExecProv::getMissingLines( TypeSet<int>& inlnrs ) const
{
    FilePath basefp( iopar_.find(sKey::TmpStor()) );

    for ( int inl=todoinls_.start_; inl<=todoinls_.stop_; inl+=todoinls_.step_ )
    {
	BufferString fnm( "i." ); fnm += inl;
	FilePath fp( basefp, fnm );
	fnm = fp.fullPath();
	od_istream* strm = new od_istream( fnm );
	bool isok = strm->isOK();
	if ( isok )
	{
	    CBVSReader rdr( strm, false ); // stream closed by reader
	    isok = !rdr.errMsg();
	    if ( isok )
		isok = rdr.info().geom_.start.crl() ||
		       rdr.info().geom_.stop.crl();

	    if ( isok )
		inl = rdr.info().geom_.stop.inl();
	}
	else
	{
	    delete strm;
	}

	if ( !isok )
	    inlnrs += inl;
    }
}


MultiID SeisJobExecProv::tempStorID() const
{
    FilePath fp( iopar_.find(sKey::TmpStor()) );

    // Is there already an entry?
    const IODir iodir( ctio_.ctxt_.getSelKey() );
    const IODirEntryList el( iodir, ctio_.ctxt_ );
    const BufferString fnm( fp.fullPath() );
    for ( int idx=0; idx<el.size(); idx++ )
    {
	const IOObj* ioobj = el[idx]->ioobj_;
	if ( !ioobj ) continue;
	mDynamicCastGet(const IOStream*,iostrm,ioobj)
	if ( !iostrm || !iostrm->isMulti() ) continue;

	if ( fnm == iostrm->fileSpec().fileName() )
	    return iostrm->key();
    }

    MultiID ret;
    BufferString objnm( "~" );
    objnm += fp.fileName();
    ctio_.setName( objnm );
    IOM().getEntry( ctio_ );
    if ( !ctio_.ioobj_ )
	errmsg_ = uiStrings::phrCannotCreateDBEntryFor(tr("temporary storage"));
    else
    {
	ret = ctio_.ioobj_->key();
	ctio_.ioobj_->pars() = outioobjpars_;
	mDynamicCastGet(IOStream*,iostrm,ctio_.ioobj_)
	fp.add( "i.*" );
	StepInterval<int> inls( todoinls_ );
	if ( inls.start_ == 0 && inls.stop_ == 0 )
	{
	    inls.start_ = SI().sampling(false).hsamp_.start_.inl();
	    inls.stop_ = SI().sampling(false).hsamp_.stop_.inl();
	    inls.step_ = SI().sampling(false).hsamp_.step_.inl();
	}

	iostrm->fileSpec().setFileName( fp.fullPath() );
	iostrm->fileSpec().nrs_ = inls;

	IOM().commitChanges( *iostrm );
	ctio_.setObj(0);
    }

    return ret;
}


Executor* SeisJobExecProv::getPostProcessor()
{
    if ( is2d_ )
	return nullptr;

    PtrMan<IOObj> inioobj = IOM().get( tmpstorid_ );
    PtrMan<IOObj> outioobj = IOM().get( seisoutid_ );
    if ( !inioobj || !outioobj )
	return nullptr;

    return new SeisSingleTraceProc( *inioobj, *outioobj,
				    "Data transfer", &iopar_,
				    tr("Writing results to output cube") );
}


bool SeisJobExecProv::removeTempSeis()
{
    if ( is2d_ ) return true;

    PtrMan<IOObj> ioobj = IOM().get( tmpstorid_ );
    if ( !ioobj ) return true;

    FilePath fp( ioobj->fullUserExpr(true) );
    IOM().permRemove( tmpstorid_ );

    if ( fp.fileName().startsWith("i.") )
	fp.setFileName(0);
    return File::remove( fp.fullPath().buf() );
}

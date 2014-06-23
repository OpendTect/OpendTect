/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2002 / Mar 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseismmproc.h"
#include "uiseismmjobdispatch.h"
#include "uiseisioobjinfo.h"
#include "seisjobexecprov.h"
#include "jobrunner.h"
#include "jobdescprov.h"
#include "iopar.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "settings.h"
#include "genc.h"

#include "uilabel.h"
#include "uiiosel.h"
#include "uimsg.h"
#include "uigeninput.h"
#include "od_helpids.h"


bool Batch::SeisMMProgDef::isSuitedFor( const char* pnm ) const
{
    FixedString prognm = pnm;
    return prognm == Batch::JobSpec::progNameFor( Batch::JobSpec::Attrib );
}

bool Batch::SeisMMProgDef::canHandle( const Batch::JobSpec& js ) const
{
    return isSuitedFor( js.prognm_ );
}

bool Batch::SeisMMProgDef::canResume( const Batch::JobSpec& js ) const
{
    return canHandle(js) && SeisJobExecProv::isRestart(js.pars_);
}

static const char* outlsfilename = "outls.2ds";

#define mMMKey			"MultiMachine"
#define mNrInlPerJobSettKey	"Nr inline per job"
#define mNrInlPerJobProcKey	"Nr of Inlines per Job"

static int defltNrInlPerJob( const IOPar& inputpar )
{
    int nr_inl_job = -1;
    inputpar.get( mNrInlPerJobProcKey, nr_inl_job );
    if ( nr_inl_job <= 0 )
    {
	IOPar* iopar = Settings::common().subselect( mMMKey );
	if ( !iopar ) iopar = new IOPar;

	bool insettings = iopar->get( mNrInlPerJobSettKey, nr_inl_job );

	if ( !insettings )
	{
	    nr_inl_job = 3;
	    iopar->set( mNrInlPerJobSettKey, nr_inl_job );

	    Settings::common().mergeComp( *iopar, mMMKey );
	    Settings::common().write();
	}
    }

    return nr_inl_job;
}



#define mRetInvJobSpec(s) \
    { \
	delete outioobjinfo_; outioobjinfo_ = 0; \
	new uiLabel( this, s ); \
	setOkText( tr("Dismiss") ); setCancelText( uiStrings::sEmptyString() );\
        return; \
    }


uiSeisMMProc::uiSeisMMProc( uiParent* p, const IOPar& iop )
    : uiMMBatchJobDispatcher(p,iop, mODHelpKey(mSeisMMProcHelpID) )
    , parfnm_(iop.find(sKey::FileName()))
    , tmpstordirfld_(0), inlperjobfld_(0)
    , jobprov_(0)
    , outioobjinfo_(0)
    , lsfileemitted_(false)
    , is2d_(false)
{
    if ( parfnm_.isEmpty() )
	mRetInvJobSpec( tr("Invalid job specification file pass."
		"\nMissing 'File name' key.") )

    const char* idres = jobpars_.find( SeisJobExecProv::outputKey(jobpars_) );
    if ( !idres )
	mRetInvJobSpec( "Cannot find the output ID in the job specification."
	"\nThis may mean the job is not fit for Multi-Job/Machine execution" )

    const MultiID outid( idres );
    outioobjinfo_ = new uiSeisIOObjInfo( outid );
    if ( !outioobjinfo_->isOK() )
	mRetInvJobSpec( BufferString("Cannot find output cube (", idres,
			") in object management." ) );

    nrinlperjob_ = InlineSplitJobDescProv::defaultNrInlPerJob();
    jobpars_.get( "Nr of Inlines per Job", nrinlperjob_ );

    const_cast<bool&>(is2d_) = outioobjinfo_->is2D();
    const bool doresume = Batch::JobDispatcher::userWantsResume(iop)
			&& SeisJobExecProv::isRestart(iop);

    setOkText( tr("  Dismiss  ") );
    setTitleText( isMultiHost()  ? tr("Multi-Machine Processing")
			: (is2d_ ? tr("Multi-line processing")
				 : tr("Line-split processing")) );
    FixedString res = jobpars_.find( sKey::Target() );
    BufferString captn = "Processing";
    if ( !res.isEmpty() )
	captn.add( " '" ).add( res ).add( "'" );
    setCaption( captn );

    if ( !is2d_ )
    {
	BufferString tmpstordir = jobpars_.find( sKey::TmpStor() );
	if ( !doresume )
	{
	    if ( File::exists(tmpstordir) )
		File::remove(tmpstordir);
	    tmpstordir = SeisJobExecProv::getDefTempStorDir();
	    FilePath fp( tmpstordir ); fp.setFileName( 0 );
	    tmpstordir = fp.fullPath();
	}

	uiObject* inlperjobattach = 0;
	if ( doresume )
	{
	    BufferString msg( sKey::TmpStor() ); msg += ": ";
	    uiLabel* tmpstorloc = new uiLabel( specparsgroup_, msg );

	    inlperjobattach = new uiLabel( specparsgroup_, tmpstordir );
	    inlperjobattach->attach( rightOf, tmpstorloc );
	}
	else
	{
	    tmpstordirfld_ = new uiIOFileSelect( specparsgroup_,
			    sKey::TmpStor(), false, tmpstordir );
	    tmpstordirfld_->usePar( uiIOFileSelect::tmpstoragehistory() );
	    if ( !tmpstordir.isEmpty() && File::isDirectory(tmpstordir) )
		tmpstordirfld_->setInput( tmpstordir );
	    tmpstordirfld_->selectDirectory( true );
	    tmpstordirfld_->setHSzPol( uiObject::MedMax );
	    inlperjobattach = tmpstordirfld_->mainObject();
	}

	inlperjobfld_ = new uiGenInput( specparsgroup_, 
                        tr("Nr of inlines per job"),
			IntInpSpec( defltNrInlPerJob(jobpars_) ) );

	inlperjobfld_->attach( alignedBelow, inlperjobattach );
    }
}


uiSeisMMProc::~uiSeisMMProc()
{
    delete jobprov_;
    delete outioobjinfo_;
}


#define mErrRet(s) { uiMSG().error(s); return 0; }

bool uiSeisMMProc::initWork( bool retry )
{
    if ( !outioobjinfo_ )
	return false;

    errmsg_.setEmpty();
    BufferString tmpstordir;

    if ( !retry )
    {
	if ( !tmpstordirfld_ )
	    jobpars_.get( sKey::TmpStor(), tmpstordir );
	else
	{
	    tmpstordir = tmpstordirfld_->getInput();
	    if ( !File::isWritable(tmpstordir) )
		mErrRet(tr("The temporary storage directory is not writable"))
	    tmpstordir = SeisJobExecProv::getDefTempStorDir( tmpstordir );
	    jobpars_.set( sKey::TmpStor(), tmpstordir );
	}

	jobprov_ = new SeisJobExecProv(
		Batch::JobSpec::progNameFor(Batch::JobSpec::Attrib), jobpars_ );
	if ( jobprov_->errMsg() )
	    { errmsg_ = jobprov_->errMsg(); return false; }

	nrinlperjob_ = 1;
	if ( inlperjobfld_ )
	{
	    nrinlperjob_ = inlperjobfld_->getIntValue();
	    if ( nrinlperjob_ < 1 ) nrinlperjob_ = 1;
	    if ( nrinlperjob_ > 100 ) nrinlperjob_ = 100;
	    inlperjobfld_->setValue( nrinlperjob_ );
	    inlperjobfld_->setSensitive( false );
	}
    }

    delete jobrunner_;
    jobrunner_ = jobprov_->getRunner( retry ? 1 : nrinlperjob_ );
    if ( jobprov_->errMsg() && *jobprov_->errMsg() )
    {
	delete jobrunner_; jobrunner_ = 0;
	errmsg_ = jobprov_->errMsg();
	return false;
    }

    if ( !retry )
    {
	if ( !is2d_ )
	{
	    jobpars_.get( sKey::TmpStor(), tmpstordir );
	    if ( !File::isDirectory(tmpstordir) )
	    {
		if ( File::exists(tmpstordir) )
		    File::remove( tmpstordir );
		File::createDir( tmpstordir );
	    }
	    if ( !File::isDirectory(tmpstordir) )
		mErrRet(tr("Cannot create temporary storage directory"))
	}

	jobprov_->pars().write( parfnm_, sKey::Pars() );
    }

    return true;
}


bool uiSeisMMProc::prepareCurrentJob()
{
    if ( !is2d_ )
	return true;

    // Put a copy of the .2ds file in the proc directory
    // Makes sure 2D changes are only done on master
    if ( !lsfileemitted_ )
    {
	const BufferString lsfnm =
		FilePath(jobrunner_->procDir(),outlsfilename).fullPath();
	lsfileemitted_ = jobprov_->emitLSFile( lsfnm );
    }
    if ( lsfileemitted_ )
    {
	FilePath fp( jobrunner_->curJobFilePath() );
	fp.setFileName( outlsfilename );
	const BufferString lsfnm( fp.fullPath() );
		// This lsfnm may differ from above - remote directories!
	jobprov_->preparePreSet( jobrunner_->curJobIOPar(),
				SeisJobExecProv::sKeyOutputLS() );
	jobrunner_->curJobIOPar().set(
		IOPar::compKey(SeisJobExecProv::sKeyWorkLS(),sKey::FileName()),
		lsfnm );
    }

    return true;
}


Executor* uiSeisMMProc::getPostProcessor() const
{
    return jobprov_ ? jobprov_->getPostProcessor() : 0;
}


bool uiSeisMMProc::haveTmpProcFiles() const
{
    return !is2d_;
}


bool uiSeisMMProc::removeTmpProcFiles()
{
    if ( !jobprov_ )
	return true;

    bool removed = jobprov_->removeTempSeis();
    int count = 3;

    while ( !removed && count-- > 0 )
    {
	sleepSeconds( 2 );
	removed = jobprov_->removeTempSeis();
    }

    if ( !removed )
	errmsg_.set( "Could not remove all temporary seismics" );

    return removed;
}


bool uiSeisMMProc::needConfirmEarlyStop() const
{
    return outioobjinfo_ && jobrunner_;
}
